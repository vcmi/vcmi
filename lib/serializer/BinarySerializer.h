/*
 * BinarySerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSerializer.h"
#include "CTypeList.h"
#include "ESerializationVersion.h"
#include "Serializeable.h"
#include "../mapObjects/CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CSaverBase
{
protected:
	IBinaryWriter * writer;
public:
	CSaverBase(IBinaryWriter * w): writer(w){};

	void write(const void * data, unsigned size)
	{
		writer->write(reinterpret_cast<const std::byte*>(data), size);
	};
};

/// Main class for serialization of classes into binary form
/// Behaviour for various classes is following:
/// Primitives:    copy memory into underlying stream (defined in CSaverBase)
/// Containers:    custom overloaded method that decouples class into primitives
/// VCMI Classes:  recursively serialize them via ClassName::serialize( BinarySerializer &, int version) call
class BinarySerializer : public CSaverBase
{
	template<typename Handler>
	struct VariantVisitorSaver
	{
		Handler &h;
		VariantVisitorSaver(Handler &H):h(H)
		{
		}

		template <typename T>
		void operator()(const T &t)
		{
			h & t;
		}
	};

	template<typename Fake, typename T>
	bool saveIfStackInstance(const T &data)
	{
		return false;
	}

	template<typename Fake>
	bool saveIfStackInstance(const CStackInstance* const &data)
	{
		assert(data->armyObj);

		SlotID slot;

		if(data->getNodeType() == CBonusSystemNode::COMMANDER)
			slot = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		else
			slot = data->armyObj->findStack(data);

		assert(slot != SlotID());
		save(data->armyObj->id);
		save(slot);

		if (data->armyObj->id != ObjectInstanceID::NONE)
			return true;
		else
			return false;
	}

	template <typename T> class CPointerSaver;

	class CBasicPointerSaver
	{
	public:
		virtual void savePtr(CSaverBase &ar, const Serializeable *data) const =0;
		virtual ~CBasicPointerSaver() = default;

		template<typename T> static CBasicPointerSaver *getApplier(const T * t=nullptr)
		{
			return new CPointerSaver<T>();
		}
	};


	template <typename T>
	class CPointerSaver : public CBasicPointerSaver
	{
	public:
		void savePtr(CSaverBase &ar, const Serializeable *data) const override
		{
			auto & s = static_cast<BinarySerializer &>(ar);
			const T *ptr = dynamic_cast<const T*>(data);

			//T is most derived known type, it's time to call actual serialize
			const_cast<T*>(ptr)->serialize(s);
		}
	};

	CApplier<CBasicPointerSaver> applier;

public:
	using Version = ESerializationVersion;

	std::map<std::string, uint32_t> savedStrings;
	std::map<const Serializeable*, uint32_t> savedPointers;

	Version version = Version::CURRENT;
	static constexpr bool trackSerializedPointers = true;
	static constexpr bool saving = true;
	bool loadingGamestate = false;

	bool hasFeature(Version what) const
	{
		return version >= what;
	};

	DLL_LINKAGE BinarySerializer(IBinaryWriter * w);

	template<typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		applier.registerType(b, d);
	}

	template<class T>
	BinarySerializer & operator&(const T & t)
	{
		this->save(t);
		return * this;
	}

	void saveEncodedInteger(int64_t value)
	{
		uint64_t valueUnsigned = std::abs(value);

		while (valueUnsigned > 0x3f)
		{
			uint8_t byteValue = (valueUnsigned & 0x7f) | 0x80;
			valueUnsigned = valueUnsigned >> 7;
			save(byteValue);
		}

		uint8_t lastByteValue = valueUnsigned & 0x3f;
		if (value < 0)
			lastByteValue |= 0x40;

		save(lastByteValue);
	}

	template < typename T, typename std::enable_if_t < std::is_same_v<T, bool>, int > = 0 >
	void save(const T &data)
	{
		uint8_t writ = static_cast<uint8_t>(data);
		save(writ);
	}

	template < class T, typename std::enable_if_t < std::is_floating_point_v<T>, int  > = 0 >
	void save(const T &data)
	{
		// save primitive - simply dump binary data to output
		this->write(static_cast<const void *>(&data), sizeof(data));
	}

	template < class T, typename std::enable_if_t < std::is_integral_v<T> && !std::is_same_v<T, bool>, int  > = 0 >
	void save(const T &data)
	{
		if constexpr (sizeof(T) == 1)
		{
			// save primitive - simply dump binary data to output
			this->write(static_cast<const void *>(&data), sizeof(data));
		}
		else
		{
			if (hasFeature(Version::COMPACT_INTEGER_SERIALIZATION))
				saveEncodedInteger(data);
			else
				this->write(static_cast<const void *>(&data), sizeof(data));
		}
	}

	void save(const Version &data)
	{
		this->write(static_cast<const void *>(&data), sizeof(data));
	}

	template < typename T, typename std::enable_if_t < std::is_enum_v<T>, int  > = 0 >
	void save(const T &data)
	{
		int32_t writ = static_cast<int32_t>(data);
		*this & writ;
	}

	template < typename T, typename std::enable_if_t < std::is_array_v<T>, int  > = 0 >
	void save(const T &data)
	{
		uint32_t size = std::size(data);
		for(uint32_t i=0; i < size; i++)
			*this & data[i];
	}

	template < typename T, typename std::enable_if_t < std::is_pointer_v<T>, int  > = 0 >
	void save(const T &data)
	{
		//write if pointer is not nullptr
		bool isNull = (data == nullptr);
		save(isNull);

		//if pointer is nullptr then we don't need anything more...
		if(data == nullptr)
			return;

		savePointerImpl(data);
	}

	template < typename T, typename std::enable_if_t < std::is_base_of_v<Entity, std::remove_pointer_t<T>>, int  > = 0 >
	void savePointerImpl(const T &data)
	{
		auto index = data->getId();
		save(index);
	}

	template < typename T, typename std::enable_if_t < !std::is_base_of_v<Entity, std::remove_pointer_t<T>>, int  > = 0 >
	void savePointerImpl(const T &data)
	{
		typedef typename std::remove_const_t<typename std::remove_pointer_t<T>> TObjectType;

		if(writer->smartVectorMembersSerialization)
		{
			typedef typename VectorizedTypeFor<TObjectType>::type VType;
			typedef typename VectorizedIDType<TObjectType>::type IDType;

			if(const auto *info = writer->getVectorizedTypeInfo<VType, IDType>())
			{
				IDType id = writer->getIdFromVectorItem<VType>(*info, data);
				save(id);
				if(id != IDType(-1)) //vector id is enough
					return;
			}
		}

		if(writer->sendStackInstanceByIds)
		{
			const bool gotSaved = saveIfStackInstance<void>(data);
			if(gotSaved)
				return;
		}

		if(trackSerializedPointers)
		{
			// We might have an object that has multiple inheritance and store it via the non-first base pointer.
			// Therefore, all pointers need to be normalized to the actual object address.
			const auto * actualPointer = static_cast<const Serializeable*>(data);
			auto i = savedPointers.find(actualPointer);
			if(i != savedPointers.end())
			{
				//this pointer has been already serialized - write only it's id
				save(i->second);
				return;
			}

			//give id to this pointer
			uint32_t pid = savedPointers.size();
			savedPointers[actualPointer] = pid;
			save(pid);
		}

		//write type identifier
		uint16_t tid = CTypeList::getInstance().getTypeID(data);
		save(tid);

		if(!tid)
			save(*data); //if type is unregistered simply write all data in a standard way
		else
			applier.getApplier(tid)->savePtr(*this, static_cast<const Serializeable*>(data));  //call serializer specific for our real type
	}

	template < typename T, typename std::enable_if_t < is_serializeable<BinarySerializer, T>::value, int  > = 0 >
	void save(const T &data)
	{
		const_cast<T&>(data).serialize(*this);
	}

	void save(const std::monostate & data)
	{
		// no-op
	}

	template <typename T>
	void save(const std::shared_ptr<T> &data)
	{
		T *internalPtr = data.get();
		save(internalPtr);
	}
	template <typename T>
	void save(const std::shared_ptr<const T> &data)
	{
		const T *internalPtr = data.get();
		save(internalPtr);
	}
	template <typename T>
	void save(const std::unique_ptr<T> &data)
	{
		T *internalPtr = data.get();
		save(internalPtr);
	}
	template <typename T, typename std::enable_if_t < !std::is_same_v<T, bool >, int  > = 0>
	void save(const std::vector<T> &data)
	{
		uint32_t length = data.size();
		*this & length;
		for(uint32_t i=0;i<length;i++)
			save(data[i]);
	}
	template <typename T, typename std::enable_if_t < !std::is_same_v<T, bool >, int  > = 0>
	void save(const std::deque<T> & data)
	{
		uint32_t length = data.size();
		*this & length;
		for(uint32_t i = 0; i < length; i++)
			save(data[i]);
	}
	template <typename T, size_t N>
	void save(const std::array<T, N> &data)
	{
		for(uint32_t i=0; i < N; i++)
			save(data[i]);
	}
	template <typename T>
	void save(const std::set<T> &data)
	{
		auto & d = const_cast<std::set<T> &>(data);
		uint32_t length = d.size();
		save(length);
		for(auto i = d.begin(); i != d.end(); i++)
			save(*i);
	}
	template <typename T, typename U>
	void save(const std::unordered_set<T, U> &data)
	{
		auto & d = const_cast<std::unordered_set<T, U> &>(data);
		uint32_t length = d.size();
		*this & length;
		for(auto i = d.begin(); i != d.end(); i++)
			save(*i);
	}
	template <typename T>
	void save(const std::list<T> &data)
	{
		auto & d = const_cast<std::list<T> &>(data);
		uint32_t length = d.size();
		*this & length;
		for(auto i = d.begin(); i != d.end(); i++)
			save(*i);
	}

	void save(const std::string &data)
	{
		if (hasFeature(Version::COMPACT_STRING_SERIALIZATION))
		{
			if (data.empty())
			{
				save(static_cast<uint32_t>(0));
				return;
			}

			auto it = savedStrings.find(data);

			if (it == savedStrings.end())
			{
				save(static_cast<uint32_t>(data.length()));
				this->write(static_cast<const void *>(data.data()), data.size());

				// -1, -2...
				int32_t newStringID = -1 - savedStrings.size();

				savedStrings[data] = newStringID;
			}
			else
			{
				int32_t index = it->second;
				save(index);
			}
		}
		else
		{
			save(static_cast<uint32_t>(data.length()));
			this->write(static_cast<const void *>(data.data()), data.size());
		}
	}

	template <typename T1, typename T2>
	void save(const std::pair<T1,T2> &data)
	{
		save(data.first);
		save(data.second);
	}
	template <typename T1, typename T2>
	void save(const std::unordered_map<T1,T2> &data)
	{
		*this & static_cast<uint32_t>(data.size());
		for(auto i = data.begin(); i != data.end(); i++)
		{
			save(i->first);
			save(i->second);
		}
	}
	template <typename T1, typename T2>
	void save(const std::map<T1,T2> &data)
	{
		*this & static_cast<uint32_t>(data.size());
		for(auto i = data.begin(); i != data.end(); i++)
		{
			save(i->first);
			save(i->second);
		}
	}
	template <typename T1, typename T2>
	void save(const std::multimap<T1, T2> &data)
	{
		*this & static_cast<uint32_t>(data.size());
		for(auto i = data.begin(); i != data.end(); i++)
		{
			save(i->first);
			save(i->second);
		}
	}
	template<typename T0, typename... TN>
	void save(const std::variant<T0, TN...> & data)
	{
		int32_t which = data.index();
		save(which);

		VariantVisitorSaver<BinarySerializer> visitor(*this);
		std::visit(visitor, data);
	}
	template<typename T>
	void save(const std::optional<T> & data)
	{
		if(data)
		{
			save(static_cast<uint8_t>(1));
			save(*data);
		}
		else
		{
			save(static_cast<uint32_t>(0));
		}
	}

	template <typename T>
	void save(const boost::multi_array<T, 3> &data)
	{
		uint32_t length = data.num_elements();
		*this & length;
		auto shape = data.shape();
		uint32_t x = shape[0];
		uint32_t y = shape[1];
		uint32_t z = shape[2];
		*this & x & y & z;
		for(uint32_t i = 0; i < length; i++)
			save(data.data()[i]);
	}
	template <std::size_t T>
	void save(const std::bitset<T> &data)
	{
		static_assert(T <= 64);
		if constexpr (T <= 16)
		{
			auto writ = static_cast<uint16_t>(data.to_ulong());
			save(writ);
		}
		else if constexpr (T <= 32)
		{
			auto writ = static_cast<uint32_t>(data.to_ulong());
			save(writ);
		}
		else if constexpr (T <= 64)
		{
			auto writ = static_cast<uint64_t>(data.to_ulong());
			save(writ);
		}
	}
};

VCMI_LIB_NAMESPACE_END
