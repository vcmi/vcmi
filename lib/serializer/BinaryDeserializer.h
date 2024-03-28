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
#include "CTypeList.h"
#include "ESerializationVersion.h"
#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CLoaderBase
{
protected:
	IBinaryReader * reader;
public:
	CLoaderBase(IBinaryReader * r): reader(r){};

	inline void read(void * data, unsigned size, bool reverseEndianness)
	{
		auto bytePtr = reinterpret_cast<std::byte*>(data);

		reader->read(bytePtr, size);
		if(reverseEndianness)
			std::reverse(bytePtr, bytePtr + size);
	};
};

/// Main class for deserialization of classes from binary form
/// Effectively revesed version of BinarySerializer
class DLL_LINKAGE BinaryDeserializer : public CLoaderBase
{
	template<typename Ser,typename T>
	struct LoadIfStackInstance
	{
		static bool invoke(Ser &s, T &data)
		{
			return false;
		}
	};

	template<typename Ser>
	struct LoadIfStackInstance<Ser, CStackInstance *>
	{
		static bool invoke(Ser &s, CStackInstance* &data)
		{
			CArmedInstance *armedObj;
			SlotID slot;
			s.load(armedObj);
			s.load(slot);
			if(slot != SlotID::COMMANDER_SLOT_PLACEHOLDER)
			{
				assert(armedObj->hasStackAtSlot(slot));
				data = armedObj->stacks[slot];
			}
			else
			{
				auto * hero = dynamic_cast<CGHeroInstance *>(armedObj);
				assert(hero);
				assert(hero->commander);
				data = hero->commander;
			}
			return true;
		}
	};

	template <typename T, typename Enable = void>
	struct ClassObjectCreator
	{
		static T *invoke(IGameCallback *cb)
		{
			static_assert(!std::is_base_of_v<GameCallbackHolder, T>, "Cannot call new upon map objects!");
			static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
			return new T();
		}
	};

	template<typename T>
	struct ClassObjectCreator<T, typename std::enable_if_t<std::is_abstract_v<T>>>
	{
		static T *invoke(IGameCallback *cb)
		{
			throw std::runtime_error("Something went really wrong during deserialization. Attempted creating an object of an abstract class " + std::string(typeid(T).name()));
		}
	};

	template<typename T>
	struct ClassObjectCreator<T, typename std::enable_if_t<std::is_base_of_v<GameCallbackHolder, T> && !std::is_abstract_v<T>>>
	{
		static T *invoke(IGameCallback *cb)
		{
			static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
			return new T(cb);
		}
	};

	STRONG_INLINE ui32 readAndCheckLength()
	{
		ui32 length;
		load(length);
		//NOTE: also used for h3m's embedded in campaigns, so it may be quite large in some cases (e.g. XXL maps with multiple objects)
		if(length > 1000000)
		{
			logGlobal->warn("Warning: very big length: %d", length);
			reader->reportState(logGlobal);
		};
		return length;
	}

	template <typename Type> class CPointerLoader;

	class IPointerLoader
	{
	public:
		virtual void * loadPtr(CLoaderBase &ar, IGameCallback * cb, ui32 pid) const =0; //data is pointer to the ACTUAL POINTER
		virtual ~IPointerLoader() = default;

		template<typename Type> static IPointerLoader *getApplier(const Type * t = nullptr)
		{
			return new CPointerLoader<Type>();
		}
	};

	template <typename Type>
	class CPointerLoader : public IPointerLoader
	{
	public:
		void * loadPtr(CLoaderBase &ar, IGameCallback * cb, ui32 pid) const override //data is pointer to the ACTUAL POINTER
		{
			auto & s = static_cast<BinaryDeserializer &>(ar);

			//create new object under pointer
			Type * ptr = ClassObjectCreator<Type>::invoke(cb); //does new npT or throws for abstract classes
			s.ptrAllocated(ptr, pid);

			ptr->serialize(s);

			return static_cast<void*>(ptr);
		}
	};

	CApplier<IPointerLoader> applier;

	int write(const void * data, unsigned size);

public:
	using Version = ESerializationVersion;

	bool reverseEndianness; //if source has different endianness than us, we reverse bytes
	Version version;

	std::map<ui32, void*> loadedPointers;
	std::map<const void*, std::shared_ptr<void>> loadedSharedPointers;
	IGameCallback * cb = nullptr;
	bool smartPointerSerialization;
	bool saving;

	BinaryDeserializer(IBinaryReader * r);

	template<class T>
	BinaryDeserializer & operator&(T & t)
	{
		this->load(t);
		return * this;
	}

	template < class T, typename std::enable_if_t < std::is_fundamental_v<T> && !std::is_same_v<T, bool>, int  > = 0 >
	void load(T &data)
	{
		this->read(static_cast<void *>(&data), sizeof(data), reverseEndianness);
	}

	template < typename T, typename std::enable_if_t < is_serializeable<BinaryDeserializer, T>::value, int  > = 0 >
	void load(T &data)
	{
		////that const cast is evil because it allows to implicitly overwrite const objects when deserializing
		typedef typename std::remove_const_t<T> nonConstT;
		auto & hlp = const_cast<nonConstT &>(data);
		hlp.serialize(*this);
	}
	template < typename T, typename std::enable_if_t < std::is_array_v<T>, int  > = 0 >
	void load(T &data)
	{
		ui32 size = std::size(data);
		for(ui32 i = 0; i < size; i++)
			load(data[i]);
	}

	template < typename T, typename std::enable_if_t < std::is_enum_v<T>, int  > = 0 >
	void load(T &data)
	{
		si32 read;
		load( read );
		data = static_cast<T>(read);
	}

	template < typename T, typename std::enable_if_t < std::is_same_v<T, bool>, int > = 0 >
	void load(T &data)
	{
		ui8 read;
		load( read );
		data = static_cast<bool>(read);
	}

	template <typename T, typename std::enable_if_t < !std::is_same_v<T, bool >, int  > = 0>
	void load(std::vector<T> &data)
	{
		ui32 length = readAndCheckLength();
		data.resize(length);
		for(ui32 i=0;i<length;i++)
			load( data[i]);
	}

	template <typename T, typename std::enable_if_t < !std::is_same_v<T, bool >, int  > = 0>
	void load(std::deque<T> & data)
	{
		ui32 length = readAndCheckLength();
		data.resize(length);
		for(ui32 i = 0; i < length; i++)
			load(data[i]);
	}

	template < typename T, typename std::enable_if_t < std::is_pointer_v<T>, int  > = 0 >
	void load(T &data)
	{
		bool isNull;
		load( isNull );
		if(isNull)
		{
			data = nullptr;
			return;
		}

		loadPointerImpl(data);
	}

	template < typename T, typename std::enable_if_t < std::is_base_of_v<Entity, std::remove_pointer_t<T>>, int  > = 0 >
	void loadPointerImpl(T &data)
	{
		using DataType = std::remove_pointer_t<T>;

		typename DataType::IdentifierType index;
		load(index);

		auto * constEntity = index.toEntity(VLC);
		auto * constData = dynamic_cast<const DataType *>(constEntity);
		data = const_cast<DataType *>(constData);
	}

	template < typename T, typename std::enable_if_t < !std::is_base_of_v<Entity, std::remove_pointer_t<T>>, int  > = 0 >
	void loadPointerImpl(T &data)
	{
		if(reader->smartVectorMembersSerialization)
		{
			typedef typename std::remove_const_t<typename std::remove_pointer_t<T>> TObjectType; //eg: const CGHeroInstance * => CGHeroInstance
			typedef typename VectorizedTypeFor<TObjectType>::type VType;									 //eg: CGHeroInstance -> CGobjectInstance
			typedef typename VectorizedIDType<TObjectType>::type IDType;
			if(const auto *info = reader->getVectorizedTypeInfo<VType, IDType>())
			{
				IDType id;
				load(id);
				if(id != IDType(-1))
				{
					data = static_cast<T>(reader->getVectorItemFromId<VType, IDType>(*info, id));
					return;
				}
			}
		}

		if(reader->sendStackInstanceByIds)
		{
			bool gotLoaded = LoadIfStackInstance<BinaryDeserializer,T>::invoke(* this, data);
			if(gotLoaded)
				return;
		}

		ui32 pid = 0xffffffff; //pointer id (or maybe rather pointee id)
		if(smartPointerSerialization)
		{
			load( pid ); //get the id
			auto i = loadedPointers.find(pid); //lookup

			if(i != loadedPointers.end())
			{
				// We already got this pointer
				// Cast it in case we are loading it to a non-first base pointer
				data = static_cast<T>(i->second);
				return;
			}
		}
		//get type id
		ui16 tid;
		load( tid );

		if(!tid)
		{
			typedef typename std::remove_pointer_t<T> npT;
			typedef typename std::remove_const_t<npT> ncpT;
			data = ClassObjectCreator<ncpT>::invoke(cb);
			ptrAllocated(data, pid);
			load(*data);
		}
		else
		{
			auto * app = applier.getApplier(tid);
			if(app == nullptr)
			{
				logGlobal->error("load %d %d - no loader exists", tid, pid);
				data = nullptr;
				return;
			}
			data = static_cast<T>(app->loadPtr(*this, cb, pid));
		}
	}

	template <typename T>
	void ptrAllocated(const T *ptr, ui32 pid)
	{
		if(smartPointerSerialization && pid != 0xffffffff)
			loadedPointers[pid] = (void*)ptr; //add loaded pointer to our lookup map; cast is to avoid errors with const T* pt
	}

	template<typename Base, typename Derived> void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		applier.registerType(b, d);
	}

	template <typename T>
	void load(std::shared_ptr<T> &data)
	{
		typedef typename std::remove_const_t<T> NonConstT;
		NonConstT *internalPtr;
		load(internalPtr);

		void * internalPtrDerived = static_cast<void*>(internalPtr);

		if(internalPtr)
		{
			auto itr = loadedSharedPointers.find(internalPtrDerived);
			if(itr != loadedSharedPointers.end())
			{
				// This pointers is already loaded. The "data" needs to be pointed to it,
				// so their shared state is actually shared.
				data = std::static_pointer_cast<T>(itr->second);
			}
			else
			{
				auto hlp = std::shared_ptr<NonConstT>(internalPtr);
				data = hlp;
				loadedSharedPointers[internalPtrDerived] = std::static_pointer_cast<void>(hlp);
			}
		}
		else
			data.reset();
	}

	void load(std::monostate & data)
	{
		// no-op
	}

	template <typename T>
	void load(std::shared_ptr<const T> & data)
	{
		std::shared_ptr<T> nonConstData;

		load(nonConstData);

		data = nonConstData;
	}

	template <typename T>
	void load(std::unique_ptr<T> &data)
	{
		T *internalPtr;
		load( internalPtr );
		data.reset(internalPtr);
	}
	template <typename T, size_t N>
	void load(std::array<T, N> &data)
	{
		for(ui32 i = 0; i < N; i++)
			load( data[i] );
	}
	template <typename T>
	void load(std::set<T> &data)
	{
		ui32 length = readAndCheckLength();
		data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			load( ins );
			data.insert(ins);
		}
	}
	template <typename T, typename U>
	void load(std::unordered_set<T, U> &data)
	{
		ui32 length = readAndCheckLength();
		data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			load(ins);
			data.insert(ins);
		}
	}
	template <typename T>
	void load(std::list<T> &data)
	{
		ui32 length = readAndCheckLength();
		data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			load(ins);
			data.push_back(ins);
		}
	}
	template <typename T1, typename T2>
	void load(std::pair<T1,T2> &data)
	{
		load(data.first);
		load(data.second);
	}

	template <typename T1, typename T2>
	void load(std::map<T1,T2> &data)
	{
		ui32 length = readAndCheckLength();
		data.clear();
		T1 key;
		for(ui32 i=0;i<length;i++)
		{
			load(key);
			load(data[key]);
		}
	}
	void load(std::string &data)
	{
		ui32 length = readAndCheckLength();
		data.resize(length);
		this->read(static_cast<void *>(data.data()), length, false);
	}

	template<typename... TN>
	void load(std::variant<TN...> & data)
	{
		si32 which;
		load( which );
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
		ui8 present;
		load( present );
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

	template <typename T>
	void load(boost::multi_array<T, 3> & data)
	{
		ui32 length = readAndCheckLength();
		ui32 x;
		ui32 y;
		ui32 z;
		load(x);
		load(y);
		load(z);
		data.resize(boost::extents[x][y][z]);
		assert(length == data.num_elements()); //x*y*z should be equal to number of elements
		for(ui32 i = 0; i < length; i++)
			load(data.data()[i]);
	}
	template <std::size_t T>
	void load(std::bitset<T> &data)
	{
		static_assert(T <= 64);
		if constexpr (T <= 16)
		{
			uint16_t read;
			load(read);
			data = read;
		}
		else if constexpr (T <= 32)
		{
			uint32_t read;
			load(read);
			data = read;
		}
		else if constexpr (T <= 64)
		{
			uint64_t read;
			load(read);
			data = read;
		}
	}
};

VCMI_LIB_NAMESPACE_END
