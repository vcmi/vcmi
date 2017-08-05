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

#include <boost/mpl/for_each.hpp>

#include "CTypeList.h"
#include "../mapObjects/CGHeroInstance.h"

class CStackInstance;

class DLL_LINKAGE CLoaderBase
{
protected:
	IBinaryReader * reader;
public:
	CLoaderBase(IBinaryReader * r): reader(r){};

	inline int read(void * data, unsigned size)
	{
		return reader->read(data, size);
	};
};

/// Main class for deserialization of classes from binary form
/// Effectively revesed version of BinarySerializer
class DLL_LINKAGE BinaryDeserializer : public CLoaderBase
{
	template<typename Variant, typename Source>
	struct VariantLoaderHelper
	{
		Source & source;
		std::vector<std::function<Variant()>> funcs;

		VariantLoaderHelper(Source & source):
			source(source)
		{
			boost::mpl::for_each<typename Variant::types>(std::ref(*this));
		}

		template<typename Type>
		void operator()(Type)
		{
			funcs.push_back([&]() -> Variant
			{
				Type obj;
				source.load(obj);
				return Variant(obj);
			});
		}
	};

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
				auto hero = dynamic_cast<CGHeroInstance *>(armedObj);
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
		static T *invoke()
		{
			static_assert(!std::is_abstract<T>::value, "Cannot call new upon abstract classes!");
			return new T();
		}
	};

	template<typename T>
	struct ClassObjectCreator<T, typename std::enable_if<std::is_abstract<T>::value>::type>
	{
		static T *invoke()
		{
			throw std::runtime_error("Something went really wrong during deserialization. Attempted creating an object of an abstract class " + std::string(typeid(T).name()));
		}
	};

#define READ_CHECK_U32(x)			\
	ui32 length;			\
	load(length);				\
	if(length > 500000)				\
	{								\
		logGlobal->warnStream() << "Warning: very big length: " << length;\
		reader->reportState(logGlobal);			\
	};

	template <typename T> class CPointerLoader;

	class CBasicPointerLoader
	{
	public:
		virtual const std::type_info * loadPtr(CLoaderBase &ar, void *data, ui32 pid) const =0; //data is pointer to the ACTUAL POINTER
		virtual ~CBasicPointerLoader(){}

		template<typename T> static CBasicPointerLoader *getApplier(const T * t=nullptr)
		{
			return new CPointerLoader<T>();
		}
	};

	template <typename T> class CPointerLoader : public CBasicPointerLoader
	{
	public:
		const std::type_info * loadPtr(CLoaderBase &ar, void *data, ui32 pid) const override //data is pointer to the ACTUAL POINTER
		{
			BinaryDeserializer &s = static_cast<BinaryDeserializer&>(ar);
			T *&ptr = *static_cast<T**>(data);

			//create new object under pointer
			typedef typename std::remove_pointer<T>::type npT;
			ptr = ClassObjectCreator<npT>::invoke(); //does new npT or throws for abstract classes
			s.ptrAllocated(ptr, pid);
			//T is most derived known type, it's time to call actual serialize
			assert(s.fileVersion != 0);
			ptr->serialize(s,s.fileVersion);
			return &typeid(T);
		}
	};

	CApplier<CBasicPointerLoader> applier;

	int write(const void * data, unsigned size);

public:
	bool reverseEndianess; //if source has different endianness than us, we reverse bytes
	si32 fileVersion;

	std::map<ui32, void*> loadedPointers;
	std::map<ui32, const std::type_info*> loadedPointersTypes;
	std::map<const void*, boost::any> loadedSharedPointers;
	bool smartPointerSerialization;
	bool saving;

	BinaryDeserializer(IBinaryReader * r): CLoaderBase(r)
	{
		saving = false;
		fileVersion = 0;
		smartPointerSerialization = true;
		reverseEndianess = false;
	}

	template<class T>
	BinaryDeserializer & operator&(T & t)
	{
		this->load(t);
		return * this;
	}

	template < class T, typename std::enable_if < std::is_fundamental<T>::value && !std::is_same<T, bool>::value, int  >::type = 0 >
	void load(T &data)
	{
		unsigned length = sizeof(data);
		char* dataPtr = (char*)&data;
		this->read(dataPtr,length);
		if(reverseEndianess)
			std::reverse(dataPtr, dataPtr + length);
	}

	template < typename T, typename std::enable_if < is_serializeable<BinaryDeserializer, T>::value, int  >::type = 0 >
	void load(T &data)
	{
		assert( fileVersion != 0 );
		////that const cast is evil because it allows to implicitly overwrite const objects when deserializing
		typedef typename std::remove_const<T>::type nonConstT;
		nonConstT &hlp = const_cast<nonConstT&>(data);
		hlp.serialize(*this,fileVersion);
	}
	template < typename T, typename std::enable_if < std::is_array<T>::value, int  >::type = 0 >
	void load(T &data)
	{
		ui32 size = ARRAY_COUNT(data);
		for(ui32 i = 0; i < size; i++)
			load(data[i]);
	}

	template < typename T, typename std::enable_if < std::is_enum<T>::value, int  >::type = 0 >
	void load(T &data)
	{
		si32 read;
		load( read );
		data = static_cast<T>(read);
	}

	template < typename T, typename std::enable_if < std::is_same<T, bool>::value, int >::type = 0 >
	void load(T &data)
	{
		ui8 read;
		load( read );
		data = static_cast<bool>(read);
	}

	template < typename T, typename std::enable_if < std::is_same<T, std::vector<bool> >::value, int  >::type = 0 >
	void load(T & data)
	{
		std::vector<ui8> convData;
		load(convData);
		convData.resize(data.size());
		range::copy(convData, data.begin());
	}

	template <typename T, typename std::enable_if < !std::is_same<T, bool >::value, int  >::type = 0>
	void load(std::vector<T> &data)
	{
		READ_CHECK_U32(length);
		data.resize(length);
		for(ui32 i=0;i<length;i++)
			load( data[i]);
	}

	template < typename T, typename std::enable_if < std::is_pointer<T>::value, int  >::type = 0 >
	void load(T &data)
	{
		ui8 hlp;
		load( hlp );
		if(!hlp)
		{
			data = nullptr;
			return;
		}

		if(reader->smartVectorMembersSerialization)
		{
			typedef typename std::remove_const<typename std::remove_pointer<T>::type>::type TObjectType; //eg: const CGHeroInstance * => CGHeroInstance
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
			std::map<ui32, void*>::iterator i = loadedPointers.find(pid); //lookup

			if(i != loadedPointers.end())
			{
				// We already got this pointer
				// Cast it in case we are loading it to a non-first base pointer
				assert(loadedPointersTypes.count(pid));
				data = reinterpret_cast<T>(typeList.castRaw(i->second, loadedPointersTypes.at(pid), &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type)));
				return;
			}
		}

		//get type id
		ui16 tid;
		load( tid );

		if(!tid)
		{
			typedef typename std::remove_pointer<T>::type npT;
			typedef typename std::remove_const<npT>::type ncpT;
			data = ClassObjectCreator<ncpT>::invoke();
			ptrAllocated(data, pid);
			load(*data);
		}
		else
		{
			auto app = applier.getApplier(tid);
			if(app == nullptr)
			{
				logGlobal->error("load %d %d - no loader exists", tid, pid);
				data = nullptr;
				return;
			}
			auto typeInfo = app->loadPtr(*this,&data, pid);
			data = reinterpret_cast<T>(typeList.castRaw((void*)data, typeInfo, &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type)));
		}
	}

	template <typename T>
	void ptrAllocated(const T *ptr, ui32 pid)
	{
		if(smartPointerSerialization && pid != 0xffffffff)
		{
			loadedPointersTypes[pid] = &typeid(T);
			loadedPointers[pid] = (void*)ptr; //add loaded pointer to our lookup map; cast is to avoid errors with const T* pt
		}
	}

	template<typename Base, typename Derived> void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		applier.registerType(b, d);
	}

	template <typename T>
	void load(std::shared_ptr<T> &data)
	{
		typedef typename std::remove_const<T>::type NonConstT;
		NonConstT *internalPtr;
		load(internalPtr);

		void *internalPtrDerived = typeList.castToMostDerived(internalPtr);

		if(internalPtr)
		{
			auto itr = loadedSharedPointers.find(internalPtrDerived);
			if(itr != loadedSharedPointers.end())
			{
				// This pointers is already loaded. The "data" needs to be pointed to it,
				// so their shared state is actually shared.
				try
				{
					auto actualType = typeList.getTypeInfo(internalPtr);
					auto typeWeNeedToReturn = typeList.getTypeInfo<T>();
					if(*actualType == *typeWeNeedToReturn)
					{
						// No casting needed, just unpack already stored shared_ptr and return it
						data = boost::any_cast<std::shared_ptr<T>>(itr->second);
					}
					else
					{
						// We need to perform series of casts
						auto ret = typeList.castShared(itr->second, actualType, typeWeNeedToReturn);
						data = boost::any_cast<std::shared_ptr<T>>(ret);
					}
				}
				catch(std::exception &e)
				{
					logGlobal->errorStream() << e.what();
					logGlobal->errorStream() << boost::format("Failed to cast stored shared ptr. Real type: %s. Needed type %s. FIXME FIXME FIXME")
						% itr->second.type().name() % typeid(std::shared_ptr<T>).name();
					//TODO scenario with inheritance -> we can have stored ptr to base and load ptr to derived (or vice versa)
					assert(0);
				}
			}
			else
			{
				auto hlp = std::shared_ptr<NonConstT>(internalPtr);
				data = hlp; //possibly adds const
				loadedSharedPointers[internalPtrDerived] = typeList.castSharedToMostDerived(hlp);
			}
		}
		else
			data.reset();
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
		READ_CHECK_U32(length);
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
		READ_CHECK_U32(length);
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
		READ_CHECK_U32(length);
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
		READ_CHECK_U32(length);
		data.clear();
		T1 key;
		T2 value;
		for(ui32 i=0;i<length;i++)
		{
			load(key);
			load(value);
			data.insert(std::pair<T1, T2>(std::move(key), std::move(value)));
		}
	}
	template <typename T1, typename T2>
	void load(std::multimap<T1, T2> &data)
	{
		READ_CHECK_U32(length);
		data.clear();
		T1 key;
		T2 value;
		for(ui32 i = 0; i < length; i++)
		{
			load(key);
			load(value);
			data.insert(std::pair<T1, T2>(std::move(key), std::move(value)));
		}
	}
	void load(std::string &data)
	{
		READ_CHECK_U32(length);
		data.resize(length);
		this->read((void*)data.c_str(),length);
	}

	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void load(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &data)
	{
		typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> TVariant;

		VariantLoaderHelper<TVariant, BinaryDeserializer> loader(*this);

		si32 which;
		load( which );
		assert(which < loader.funcs.size());
		data = loader.funcs.at(which)();
	}

	template <typename T>
	void load(boost::optional<T> & data)
	{
		ui8 present;
		load( present );
		if(present)
		{
			//TODO: replace with emplace once we start request Boost 1.56+, see PR360
			T t;
			load(t);
			data = boost::make_optional(std::move(t));
		}
		else
		{
			data = boost::optional<T>();
		}
	}
};

class DLL_LINKAGE CLoadFile : public IBinaryReader
{
public:
	BinaryDeserializer serializer;

	std::string fName;
	std::unique_ptr<FileStream> sfile;

	CLoadFile(const boost::filesystem::path & fname, int minimalVersion = SERIALIZATION_VERSION); //throws!
	~CLoadFile();
	int read(void * data, unsigned size) override; //throws!

	void openNextFile(const boost::filesystem::path & fname, int minimalVersion); //throws!
	void clear();
	void reportState(CLogger * out) override;

	void checkMagicBytes(const std::string & text);

	template<class T>
	CLoadFile & operator>>(T &t)
	{
		serializer & t;
		return * this;
	}
};
