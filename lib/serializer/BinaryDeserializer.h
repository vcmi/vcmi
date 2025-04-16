/*
 * BinaryDeserializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSerializer.h"
#include "ESerializationVersion.h"
#include "SerializerReflection.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Main class for deserialization of classes from binary form
/// Effectively revesed version of BinarySerializer
class BinaryDeserializer
{
public:
	using Version = ESerializationVersion;
	static constexpr bool saving = false;

	IGameCallback * cb = nullptr;
	Version version = Version::NONE;
	bool loadingGamestate = false;
	bool reverseEndianness = false; //if source has different endianness than us, we reverse bytes

	BinaryDeserializer(IBinaryReader * r)
		: reader(r)
	{
	}

	template<class T>
	BinaryDeserializer & operator&(T & t)
	{
		this->load(t);
		return *this;
	}

	void clear()
	{
		loadedPointers.clear();
		loadedSharedPointers.clear();
		loadedUniquePointers.clear();
	}

	bool hasFeature(Version v) const
	{
		return version >= v;
	}

private:
	static constexpr bool trackSerializedPointers = true;

	std::vector<std::string> loadedStrings;
	std::map<uint32_t, Serializeable *> loadedPointers;
	std::set<Serializeable *> loadedUniquePointers;
	std::map<const Serializeable *, std::shared_ptr<Serializeable>> loadedSharedPointers;
	IBinaryReader * reader;

	uint32_t readAndCheckLength()
	{
		uint32_t length;
		load(length);
		//NOTE: also used for h3m's embedded in campaigns, so it may be quite large in some cases (e.g. XXL maps with multiple objects)
		if(length > 1000000)
		{
			logGlobal->warn("Warning: very big length: %d", length);
		};
		return length;
	}

	void read(void * data, unsigned size)
	{
		auto bytePtr = reinterpret_cast<std::byte *>(data);

		reader->read(bytePtr, size);
		if(reverseEndianness)
			std::reverse(bytePtr, bytePtr + size);
	};

	int64_t loadEncodedInteger()
	{
		uint64_t valueUnsigned = 0;
		uint_fast8_t offset = 0;

		for(;;)
		{
			uint8_t byteValue;
			load(byteValue);

			if((byteValue & 0x80) != 0)
			{
				valueUnsigned |= static_cast<uint64_t>(byteValue & 0x7f) << offset;
				offset += 7;
			}
			else
			{
				valueUnsigned |= static_cast<uint64_t>(byteValue & 0x3f) << offset;
				bool isNegative = (byteValue & 0x40) != 0;
				if(isNegative)
					return -static_cast<int64_t>(valueUnsigned);
				else
					return valueUnsigned;
			}
		}
	}

	template<class T, typename std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	void load(T & data)
	{
		this->read(static_cast<void *>(&data), sizeof(data));
	}

	template<class T, typename std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
	void load(T & data)
	{
		if constexpr(sizeof(T) == 1)
		{
			this->read(static_cast<void *>(&data), sizeof(data));
		}
		else
		{
			static_assert(!std::is_same_v<uint64_t, T>, "Serialization of unsigned 64-bit value may not work in some cases");
			data = loadEncodedInteger();
		}
	}

	template<typename T, typename std::enable_if_t<is_serializeable<BinaryDeserializer, T>::value, int> = 0>
	void load(T & data)
	{
		////that const cast is evil because it allows to implicitly overwrite const objects when deserializing
		typedef typename std::remove_const_t<T> nonConstT;
		auto & hlp = const_cast<nonConstT &>(data);
		hlp.serialize(*this);
	}
	template<typename T, typename std::enable_if_t<std::is_array_v<T>, int> = 0>
	void load(T & data)
	{
		uint32_t size = std::size(data);
		for(uint32_t i = 0; i < size; i++)
			load(data[i]);
	}

	void load(Version & data)
	{
		this->read(static_cast<void *>(&data), sizeof(data));
	}

	template<typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
	void load(T & data)
	{
		int32_t read;
		load(read);
		data = static_cast<T>(read);
	}

	template<typename T, typename std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
	void load(T & data)
	{
		uint8_t read;
		load(read);
		assert(read == 0 || read == 1);
		data = static_cast<bool>(read);
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<T, bool>, int> = 0>
	void load(std::vector<T> & data)
	{
		uint32_t length = readAndCheckLength();

		if constexpr(std::is_base_of_v<GameCallbackHolder, T>)
			data.resize(length, T(cb));
		else
			data.resize(length);

		for(uint32_t i=0;i<length;i++)
			load( data[i]);
	}

	template <typename T, size_t N>
	void load(boost::container::small_vector<T, N>& data)
	{
		uint32_t length = readAndCheckLength();
		data.resize(length);
		for (uint32_t i = 0; i < length; i++)
			load(data[i]);
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<T, bool>, int> = 0>
	void load(std::deque<T> & data)
	{
		uint32_t length = readAndCheckLength();
		data.resize(length);
		for(uint32_t i = 0; i < length; i++)
			load(data[i]);
	}

	template<typename T>
	void loadRawPointer(T & data)
	{
		bool isNull;
		load(isNull);
		if(isNull)
		{
			data = nullptr;
			return;
		}

		uint32_t pid = 0xffffffff; //pointer id (or maybe rather pointee id)
		if(trackSerializedPointers)
		{
			load(pid); //get the id
			auto i = loadedPointers.find(pid); //lookup

			if(i != loadedPointers.end())
			{
				// We already got this pointer
				// Cast it in case we are loading it to a non-first base pointer
				data = dynamic_cast<T>(i->second);
				if (vstd::contains(loadedUniquePointers, data))
					throw std::runtime_error("Attempt to deserialize duplicated unique_ptr!");

				return;
			}
		}
		//get type id
		uint16_t tid;
		load(tid);

		typedef typename std::remove_pointer_t<T> npT;
		typedef typename std::remove_const_t<npT> ncpT;
		if(!tid)
		{
			data = ClassObjectCreator<ncpT>::invoke(cb);
			ptrAllocated(data, pid);
			load(*data);
		}
		else
		{
			auto * app = CSerializationApplier::getInstance().getApplier(tid);
			if(app == nullptr)
			{
				logGlobal->error("load %d %d - no loader exists", tid, pid);
				data = nullptr;
				return;
			}
			auto createdPtr = app->createPtr(*this, cb);
			auto dataNonConst = dynamic_cast<ncpT *>(createdPtr);
			assert(createdPtr);
			assert(dataNonConst);
			data = dataNonConst;
			ptrAllocated(data, pid);
			app->loadPtr(*this, cb, dataNonConst);
		}
	}

	template<typename T>
	void ptrAllocated(T * ptr, uint32_t pid)
	{
		if(trackSerializedPointers && pid != 0xffffffff)
			loadedPointers[pid] = const_cast<Serializeable*>(dynamic_cast<const Serializeable*>(ptr)); //add loaded pointer to our lookup map; cast is to avoid errors with const T* pt
	}

	template<typename T>
	void load(std::shared_ptr<T> & data)
	{
		typedef typename std::remove_const_t<T> NonConstT;
		NonConstT * internalPtr;
		loadRawPointer(internalPtr);

		const auto * internalPtrDerived = static_cast<Serializeable *>(internalPtr);

		if(internalPtr)
		{
			auto itr = loadedSharedPointers.find(internalPtrDerived);
			if(itr != loadedSharedPointers.end())
			{
				// This pointers is already loaded. The "data" needs to be pointed to it,
				// so their shared state is actually shared.
				data = std::dynamic_pointer_cast<T>(itr->second);
			}
			else
			{
				auto hlp = std::shared_ptr<NonConstT>(internalPtr);
				data = hlp;
				loadedSharedPointers[internalPtrDerived] = std::static_pointer_cast<Serializeable>(hlp);
			}
		}
		else
			data.reset();
	}

	void load(std::monostate & data)
	{
		// no-op
	}

	template<typename T>
	void load(std::shared_ptr<const T> & data)
	{
		std::shared_ptr<T> nonConstData;

		load(nonConstData);

		data = nonConstData;
	}

	template<typename T>
	void load(std::unique_ptr<T> & data)
	{
		T * internalPtr;
		loadRawPointer(internalPtr);
		data.reset(internalPtr);
		loadedUniquePointers.insert(internalPtr);
	}

	template<typename T, size_t N>
	void load(std::array<T, N> & data)
	{
		for(uint32_t i = 0; i < N; i++)
			load(data[i]);
	}

	template<typename T>
	void load(std::set<T> & data)
	{
		uint32_t length = readAndCheckLength();
		data.clear();
		T ins;
		for(uint32_t i = 0; i < length; i++)
		{
			load(ins);
			data.insert(ins);
		}
	}
	template <typename T, typename U>
	void load(std::unordered_set<T, U> &data)
	{
		uint32_t length = readAndCheckLength();
		data.clear();
		T ins;
		for(uint32_t i = 0; i < length; i++)
		{
			load(ins);
			data.insert(ins);
		}
	}

	template<typename T>
	void load(std::list<T> & data)
	{
		uint32_t length = readAndCheckLength();
		data.clear();
		T ins;
		for(uint32_t i = 0; i < length; i++)
		{
			load(ins);
			data.push_back(ins);
		}
	}

	template<typename T1, typename T2>
	void load(std::pair<T1, T2> & data)
	{
		load(data.first);
		load(data.second);
	}

	template<typename T1, typename T2>
	void load(std::unordered_map<T1, T2> & data)
	{
		uint32_t length = readAndCheckLength();
		data.clear();
		T1 key;
		for(uint32_t i = 0; i < length; i++)
		{
			load(key);
			load(data[key]);
		}
	}

	template<typename T1, typename T2>
	void load(std::map<T1, T2> & data)
	{
		uint32_t length = readAndCheckLength();
		data.clear();
		T1 key;
		for(uint32_t i = 0; i < length; i++)
		{
			load(key);

			if constexpr(std::is_base_of_v<GameCallbackHolder, T2>)
			{
				data.try_emplace(key, cb);
				load(data.at(key));
			}
			else
				load(data[key]);
		}
	}

	void load(std::string & data)
	{
		int32_t length;
		load(length);

		if(length < 0)
		{
			int32_t stringID = -length - 1; // -1, -2 ... -> 0, 1 ...
			data = loadedStrings[stringID];
		}
		if(length == 0)
		{
			data = {};
		}
		if(length > 0)
		{
			data.resize(length);
			this->read(static_cast<void *>(data.data()), length);
			loadedStrings.push_back(data);
		}
	}

	template<typename... TN>
	void load(std::variant<TN...> & data)
	{
		int32_t which;
		load(which);
		assert(which < sizeof...(TN));

		// Create array of variants that contains all default-constructed alternatives
		const std::variant<TN...> table[] = { TN{ }... };
		// use appropriate alternative for result
		data = table[which];
		// perform actual load via std::visit dispatch
		std::visit([&](auto& o) { load(o); }, data);
	}

	template<typename T>
	void load(std::optional<T> & data)
	{
		uint8_t present;
		load(present);
		if(present)
		{
			//TODO: replace with emplace once we start request Boost 1.56+, see PR360
			T t;
			load(t);
			data = std::make_optional(std::move(t));
		}
		else
		{
			data = std::optional<T>();
		}
	}

	template<typename T>
	void load(boost::multi_array<T, 3> & data)
	{
		uint32_t length = readAndCheckLength();
		uint32_t x;
		uint32_t y;
		uint32_t z;
		load(x);
		load(y);
		load(z);
		data.resize(boost::extents[x][y][z]);
		assert(length == data.num_elements()); //x*y*z should be equal to number of elements
		for(uint32_t i = 0; i < length; i++)
			load(data.data()[i]);
	}
	template<std::size_t T>
	void load(std::bitset<T> & data)
	{
		static_assert(T <= 64);
		if constexpr(T <= 16)
		{
			uint16_t read;
			load(read);
			data = read;
		}
		else if constexpr(T <= 32)
		{
			uint32_t read;
			load(read);
			data = read;
		}
		else if constexpr(T <= 64)
		{
			uint64_t read;
			load(read);
			data = read;
		}
	}
};

VCMI_LIB_NAMESPACE_END
