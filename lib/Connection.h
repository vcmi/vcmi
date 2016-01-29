
/*
 * Connection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <typeinfo>
#include <type_traits>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/any.hpp>

#include "ConstTransitivePtr.h"
#include "CCreatureSet.h" //for CStackInstance
#include "mapObjects/CGHeroInstance.h"
#include "mapping/CCampaignHandler.h" //for CCampaignState
#include "rmg/CMapGenerator.h" // for CMapGenOptions

const ui32 version = 758;
const ui32 minSupportedVersion = 753;

class CISer;
class COSer;
class CConnection;
class CGObjectInstance;
class CStackInstance;
class CGameState;
class CCreature;
class LibClasses;
class CHero;
class FileStream;
struct CPack;
extern DLL_LINKAGE LibClasses * VLC;
namespace mpl = boost::mpl;

const std::string SAVEGAME_MAGIC = "VCMISVG";

namespace boost
{
	namespace asio
	{
		namespace ip
		{
			class tcp;
		}
		class io_service;

		template <typename Protocol> class stream_socket_service;
		template <typename Protocol,typename StreamSocketService>
		class basic_stream_socket;

		template <typename Protocol> class socket_acceptor_service;
		template <typename Protocol,typename SocketAcceptorService>
		class basic_socket_acceptor;
	}
	class mutex;
}

enum SerializationLvl
{
	Wrong=0,
	Boolean,
	Primitive,
	Array,
	Pointer,
	Enum,
	Serializable,
	BooleanVector
};


struct TypeComparer
{
    bool operator()(const std::type_info *a, const std::type_info *b) const
    {
    #ifndef __APPLE__
      return a->before(*b);
    #else
      return strcmp(a->name(), b->name()) < 0;
    #endif
    }
};

struct IPointerCaster
{
	virtual boost::any castRawPtr(const boost::any &ptr) const = 0; // takes From*, performs dynamic cast, returns To*
	virtual boost::any castSharedPtr(const boost::any &ptr) const = 0; // takes std::shared_ptr<From>, performs dynamic cast, returns std::shared_ptr<To>
	virtual boost::any castWeakPtr(const boost::any &ptr) const = 0; // takes std::weak_ptr<From>, performs dynamic cast, returns std::weak_ptr<To>. The object under poitner must live.
	//virtual boost::any castUniquePtr(const boost::any &ptr) const = 0; // takes std::unique_ptr<From>, performs dynamic cast, returns std::unique_ptr<To>
};

template <typename From, typename To>
struct PointerCaster : IPointerCaster
{
	virtual boost::any castRawPtr(const boost::any &ptr) const override // takes void* pointing to From object, performs dynamic cast, returns void* pointing to To object
	{
		From * from = (From*)boost::any_cast<void*>(ptr);
		To * ret = dynamic_cast<To*>(from);
		if (ret == nullptr)
		{
			// Last resort when RTTI goes mad
			ret = static_cast<To*>(from);
		}
		return (void*)ret;
	}

	// Helper function performing casts between smart pointers using dynamic_pointer_cast
	template<typename SmartPt>
	boost::any castSmartPtr(const boost::any &ptr) const
	{
		try
		{
			auto from = boost::any_cast<SmartPt>(ptr);
			auto ret = std::dynamic_pointer_cast<To>(from);
			if (!ret)
			{
				// Last resort when RTTI goes mad
				ret = std::static_pointer_cast<To>(from);
			}
			return ret;
		}
		catch(std::exception &e)
		{
			THROW_FORMAT("Failed cast %s -> %s. Given argument was %s. Error message: %s", typeid(From).name() % typeid(To).name() % ptr.type().name() % e.what());
		}
	}

	virtual boost::any castSharedPtr(const boost::any &ptr) const override
	{
		return castSmartPtr<std::shared_ptr<From>>(ptr);
	}
	virtual boost::any castWeakPtr(const boost::any &ptr) const override
	{
		auto from = boost::any_cast<std::weak_ptr<From>>(ptr);
		return castSmartPtr<std::shared_ptr<From>>(from.lock());
	}
// 	virtual boost::any castUniquePtr(const boost::any &ptr) const override
// 	{
// 		return castSmartPtr<std::unique_ptr<From>>(ptr);
// 	}
};

class DLL_LINKAGE CTypeList: public boost::noncopyable
{
public:
	struct TypeDescriptor;
	typedef std::shared_ptr<TypeDescriptor> TypeInfoPtr;
	struct TypeDescriptor
	{
		ui16 typeID;
		const char *name;
		std::vector<TypeInfoPtr> children, parents;
	};
	typedef boost::shared_mutex TMutex;
	typedef boost::unique_lock<TMutex> TUniqueLock;
	typedef boost::shared_lock<TMutex> TSharedLock;
private:
	mutable TMutex mx;

	std::map<const std::type_info *, TypeInfoPtr, TypeComparer> typeInfos;
	std::map<std::pair<TypeInfoPtr, TypeInfoPtr>, std::unique_ptr<const IPointerCaster>> casters; //for each pair <Base, Der> we provide a caster (each registered relations creates a single entry here)

	/// Returns sequence of types starting from "from" and ending on "to". Every next type is derived from the previous.
	/// Throws if there is no link registered.
	std::vector<TypeInfoPtr> castSequence(TypeInfoPtr from, TypeInfoPtr to) const;
	std::vector<TypeInfoPtr> castSequence(const std::type_info *from, const std::type_info *to) const;


	template<boost::any(IPointerCaster::*CastingFunction)(const boost::any &) const>
	boost::any castHelper(boost::any inputPtr, const std::type_info *fromArg, const std::type_info *toArg) const
	{
		TSharedLock lock(mx);
		auto typesSequence = castSequence(fromArg, toArg);

		boost::any ptr = inputPtr;
		for(int i = 0; i < static_cast<int>(typesSequence.size()) - 1; i++)
		{
			auto &from = typesSequence[i];
			auto &to = typesSequence[i + 1];
			auto castingPair = std::make_pair(from, to);
			if(!casters.count(castingPair))
				THROW_FORMAT("Cannot find caster for conversion %s -> %s which is needed to cast %s -> %s", from->name % to->name % fromArg->name() % toArg->name());

			auto &caster = casters.at(castingPair);
			ptr = (*caster.*CastingFunction)(ptr); //Why does std::unique_ptr not have operator->* ..?
		}

		return ptr;
	}

	TypeInfoPtr getTypeDescriptor(const std::type_info *type, bool throws = true) const; //if not throws, failure returns nullptr

	TypeInfoPtr registerType(const std::type_info *type);
public:
	CTypeList();

	template <typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		TUniqueLock lock(mx);
		static_assert(std::is_base_of<Base, Derived>::value, "First registerType template parameter needs to ba a base class of the second one.");
		static_assert(std::has_virtual_destructor<Base>::value, "Base class needs to have a virtual destructor.");
		static_assert(!std::is_same<Base, Derived>::value, "Parameters of registerTypes should be two diffrenet types.");
		auto bt = getTypeInfo(b), dt = getTypeInfo(d); //obtain std::type_info
		auto bti = registerType(bt), dti = registerType(dt); //obtain our TypeDescriptor

		// register the relation between classes
		bti->children.push_back(dti);
		dti->parents.push_back(bti);
		casters[std::make_pair(bti, dti)] = make_unique<const PointerCaster<Base, Derived>>();
		casters[std::make_pair(dti, bti)] = make_unique<const PointerCaster<Derived, Base>>();
	}

	ui16 getTypeID(const std::type_info *type, bool throws = false) const;

	template <typename T>
	ui16 getTypeID(const T * t = nullptr, bool throws = false) const
	{
		return getTypeID(getTypeInfo(t), throws);
	}

	template<typename TInput>
	void * castToMostDerived(const TInput * inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr);

		if (!strcmp(baseType.name(), derivedType->name()))
		{
			return const_cast<void*>(reinterpret_cast<const void*>(inputPtr));
		}

		return boost::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(
			const_cast<void*>(reinterpret_cast<const void*>(inputPtr)), &baseType,
			derivedType));
	}

	template<typename TInput>
	boost::any castSharedToMostDerived(const std::shared_ptr<TInput> inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr.get());

		if (!strcmp(baseType.name(), derivedType->name()))
			return inputPtr;

		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, &baseType, derivedType);
	}

	void * castRaw(void *inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return boost::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(inputPtr, from, to));
	}
	boost::any castShared(boost::any inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, from, to);
	}

	template <typename T> const std::type_info * getTypeInfo(const T * t = nullptr) const
	{
		if(t)
			return &typeid(*t);
		else
			return &typeid(T);
	}
};

extern DLL_LINKAGE CTypeList typeList;

template<typename Variant, typename Source>
struct VariantLoaderHelper
{
	Source & source;
	std::vector<std::function<Variant()>> funcs;

	VariantLoaderHelper(Source & source):
		source(source)
	{
		mpl::for_each<typename Variant::types>(std::ref(*this));
	}

	template<typename Type>
	void operator()(Type)
	{
		funcs.push_back([&]() -> Variant
		{
			Type obj;
			source >> obj;
			return Variant(obj);
		});
	}
};

template<typename T>
struct SerializationLevel
{
	typedef mpl::integral_c_tag tag;
	typedef
		typename mpl::eval_if<
			boost::is_same<T, bool>,
			mpl::int_<Boolean>,
		//else
		typename mpl::eval_if<
			boost::is_same<T, std::vector<bool> >,
			mpl::int_<BooleanVector>,
		//else
		typename mpl::eval_if<
			boost::is_fundamental<T>,
			mpl::int_<Primitive>,
		//else
		typename mpl::eval_if<
			boost::is_enum<T>,
			mpl::int_<Enum>,
		//else
		typename mpl::eval_if<
			boost::is_class<T>,
			mpl::int_<Serializable>,
		//else
		typename mpl::eval_if<
			boost::is_array<T>,
			mpl::int_<Array>,
		//else
		typename mpl::eval_if<
			boost::is_pointer<T>,
			mpl::int_<Pointer>,
		//else
		typename mpl::eval_if<
			boost::is_enum<T>,
			mpl::int_<Primitive>,
		//else
			mpl::int_<Wrong>
		>
		>
		>
		>
		>
		>
		>
		>::type type;
	static const int value = SerializationLevel::type::value;
};

template <typename ObjType, typename IdType>
struct VectorisedObjectInfo
{
	const std::vector<ConstTransitivePtr<ObjType> > *vector;	//pointer to the appropriate vector
	std::function<IdType(const ObjType &)> idRetriever;
	//const IdType ObjType::*idPtr;			//pointer to the field representing the position in the vector

	VectorisedObjectInfo(const std::vector< ConstTransitivePtr<ObjType> > *Vector, std::function<IdType(const ObjType &)> IdGetter)
		:vector(Vector), idRetriever(IdGetter)
	{
	}
};

template<typename T>
si32 idToNumber(const T &t, typename boost::enable_if<boost::is_convertible<T,si32> >::type * dummy = 0)
{
	return t;
}

template<typename T, typename NT>
NT idToNumber(const BaseForID<T, NT> &t)
{
	return t.getNum();
}

/// Class which is responsible for storing and loading data.
class DLL_LINKAGE CSerializer
{
public:
	typedef std::map<const std::type_info *, boost::any, TypeComparer> TTypeVecMap;
	TTypeVecMap vectors; //entry must be a pointer to vector containing pointers to the objects of key type

	bool smartVectorMembersSerialization;
	bool sendStackInstanceByIds;

	CSerializer();
	~CSerializer();

    virtual void reportState(CLogger * out){};

	template <typename T, typename U>
	void registerVectoredType(const std::vector<T*> *Vector, const std::function<U(const T&)> &idRetriever)
	{
		vectors[&typeid(T)] = VectorisedObjectInfo<T, U>(Vector, idRetriever);
	}
	template <typename T, typename U>
	void registerVectoredType(const std::vector<ConstTransitivePtr<T> > *Vector, const std::function<U(const T&)> &idRetriever)
	{
		vectors[&typeid(T)] = VectorisedObjectInfo<T, U>(Vector, idRetriever);
	}

	template <typename T, typename U>
	const VectorisedObjectInfo<T, U> *getVectorisedTypeInfo()
	{
		const std::type_info *myType = nullptr;
//
// 		if(boost::is_base_of<CGObjectInstance, T>::value) //ugly workaround to support also types derived from CGObjectInstance -> if we encounter one, treat it aas CGObj..
// 			myType = &typeid(CGObjectInstance);
// 		else
			 myType = &typeid(T);

		TTypeVecMap::iterator i = vectors.find(myType);
		if(i == vectors.end())
			return nullptr;
		else
		{
			assert(!i->second.empty());
			assert(i->second.type() == typeid(VectorisedObjectInfo<T, U>));
			VectorisedObjectInfo<T, U> *ret = &(boost::any_cast<VectorisedObjectInfo<T, U>&>(i->second));
			return ret;
		}
	}

	template <typename T, typename U>
	T* getVectorItemFromId(const VectorisedObjectInfo<T, U> &oInfo, U id) const
	{
	/*	if(id < 0)
			return nullptr;*/
		si32 idAsNumber = idToNumber(id);

		assert(oInfo.vector);
		assert(static_cast<si32>(oInfo.vector->size()) > idAsNumber);
		return const_cast<T*>((*oInfo.vector)[idAsNumber].get());
	}

	template <typename T, typename U>
	U getIdFromVectorItem(const VectorisedObjectInfo<T, U> &oInfo, const T* obj) const
	{
		if(!obj)
			return U(-1);

		return oInfo.idRetriever(*obj);
	}

	void addStdVecItems(CGameState *gs, LibClasses *lib = VLC);
};

class IBinaryWriter : public virtual CSerializer
{
public:
	virtual int write(const void * data, unsigned size) = 0;
};

class DLL_LINKAGE CSaverBase
{
protected:
	IBinaryWriter * writer;
public:
	CSaverBase(IBinaryWriter * w): writer(w){};

	inline int write(const void * data, unsigned size)
	{
		return writer->write(data, size);
	};
};

class CBasicPointerSaver
{
public:
	virtual void savePtr(CSaverBase &ar, const void *data) const =0;
	virtual ~CBasicPointerSaver(){}
};

template <typename T> //metafunction returning CGObjectInstance if T is its derivate or T elsewise
struct VectorisedTypeFor
{
	typedef typename
		//if
		mpl::eval_if<boost::is_same<CGHeroInstance,T>,
		mpl::identity<CGHeroInstance>,
		//else if
		mpl::eval_if<boost::is_base_of<CGObjectInstance,T>,
		mpl::identity<CGObjectInstance>,
		//else
		mpl::identity<T>
		> >::type type;
};
template <typename U>
struct VectorizedIDType
{
	typedef typename
		//if
		mpl::eval_if<boost::is_same<CArtifact,U>,
		mpl::identity<ArtifactID>,
		//else if
		mpl::eval_if<boost::is_same<CCreature,U>,
		mpl::identity<CreatureID>,
		//else if
		mpl::eval_if<boost::is_same<CHero,U>,
		mpl::identity<HeroTypeID>,
		//else if
		mpl::eval_if<boost::is_same<CArtifactInstance,U>,
		mpl::identity<ArtifactInstanceID>,
		//else if
		mpl::eval_if<boost::is_same<CGHeroInstance,U>,
		mpl::identity<HeroTypeID>,
		//else if
		mpl::eval_if<boost::is_base_of<CGObjectInstance,U>,
		mpl::identity<ObjectInstanceID>,
		//else
		mpl::identity<si32>
		> > > > > >::type type;
};

template <typename Handler>
struct VariantVisitorSaver : boost::static_visitor<>
{
	Handler &h;
	VariantVisitorSaver(Handler &H):h(H)
	{
	}

	template <typename T>
	void operator()(const T &t)
	{
		h << t;
	}
};

template<typename Ser,typename T>
struct SaveIfStackInstance
{
	static bool invoke(Ser &s, const T &data)
	{
		return false;
	}
};

template<typename Ser>
struct SaveIfStackInstance<Ser, CStackInstance *>
{
	static bool invoke(Ser &s, const CStackInstance* const &data)
	{
		assert(data->armyObj);
		SlotID slot;

		if(data->getNodeType() == CBonusSystemNode::COMMANDER)
			slot = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		else
			slot = data->armyObj->findStack(data);

		assert(slot != SlotID());
		s << data->armyObj << slot;
		return true;
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
		s >> armedObj >> slot;
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


/// The class which manages saving objects.
class DLL_LINKAGE COSer : public CSaverBase
{
public:

	struct SaveBoolean
	{
		static void invoke(COSer &s, const bool &data)
		{
			s.saveBoolean(data);
		}
	};

	struct SaveBooleanVector
	{
		static void invoke(COSer &s, const std::vector<bool> &data)
		{
			s.saveBooleanVector(data);
		}
	};

	template<typename T>
	struct SavePrimitive
	{
		static void invoke(COSer &s, const T &data)
		{
			s.savePrimitive(data);
		}
	};

	template<typename T>
	struct SaveSerializable
	{
		static void invoke(COSer &s, const T &data)
		{
			s.saveSerializable(data);
		}
	};

	template<typename T>
	struct SaveEnum
	{
		static void invoke(COSer &s, const T &data)
		{
			s.saveEnum(data);
		}
	};

	template<typename T>
	struct SavePointer
	{
		static void invoke(COSer &s, const T &data)
		{
			s.savePointer(data);
		}
	};

	template<typename T>
	struct SaveArray
	{
		static void invoke(COSer &s, const T &data)
		{
			s.saveArray(data);
		}
	};

	template<typename T>
	struct SaveWrong
	{
		static void invoke(COSer &s, const T &data)
		{
			throw std::runtime_error("Wrong save serialization call!");
		}
	};

	template <typename T>
	class CPointerSaver : public CBasicPointerSaver
	{
	public:
		void savePtr(CSaverBase &ar, const void *data) const override
		{
			COSer &s = static_cast<COSer&>(ar);
			const T *ptr = static_cast<const T*>(data);
			//T is most derived known type, it's time to call actual serialize
			const_cast<T*>(ptr)->serialize(s,version);
		}
	};

	bool saving;
	std::map<ui16,CBasicPointerSaver*> savers; // typeID => CPointerSaver<serializer,type>

	std::map<const void*, ui32> savedPointers;
	bool smartPointerSerialization;

	COSer(IBinaryWriter * w): CSaverBase(w)
	{
		saving=true;
		smartPointerSerialization = true;
	}
	~COSer()
	{
		std::map<ui16,CBasicPointerSaver*>::iterator iter;

		for(iter = savers.begin(); iter != savers.end(); iter++)
			delete iter->second;
	}

	template<typename T>
	void addSaver(const T * t = nullptr)
	{
		auto ID = typeList.getTypeID(t);
		if(!savers.count(ID))
			savers[ID] = new CPointerSaver<T>;
	}

	template<typename Base, typename Derived> void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		typeList.registerType(b, d);
		addSaver(b);
		addSaver(d);
	}

	template<class T>
	COSer & operator<<(const T &t)
	{
		this->save(t);
		return * this;
	}

	template<class T>
	COSer & operator&(const T & t)
	{
		return * this << t;
	}

	template <typename T>
	void savePrimitive(const T &data)
	{
		this->write(&data,sizeof(data));
	}

	template <typename T>
	void savePointer(const T &data)
	{
		//write if pointer is not nullptr
		ui8 hlp = (data!=nullptr);
		*this << hlp;

		//if pointer is nullptr then we don't need anything more...
		if(!hlp)
			return;

 		if(writer->smartVectorMembersSerialization)
		{
			typedef typename boost::remove_const<typename boost::remove_pointer<T>::type>::type TObjectType;
			typedef typename VectorisedTypeFor<TObjectType>::type VType;
			typedef typename VectorizedIDType<TObjectType>::type IDType;
 			if(const auto *info = writer->getVectorisedTypeInfo<VType, IDType>())
 			{
				IDType id = writer->getIdFromVectorItem<VType>(*info, data);
				*this << id;
				if(id != IDType(-1)) //vector id is enough
 					return;
 			}
 		}

		if(writer->sendStackInstanceByIds)
		{
			const bool gotSaved = SaveIfStackInstance<COSer,T>::invoke(*this, data);
			if(gotSaved)
				return;
		}

		if(smartPointerSerialization)
		{
			// We might have an object that has multiple inheritance and store it via the non-first base pointer.
			// Therefore, all pointers need to be normalized to the actual object address.
			auto actualPointer = typeList.castToMostDerived(data);
			std::map<const void*,ui32>::iterator i = savedPointers.find(actualPointer);
			if(i != savedPointers.end())
			{
				//this pointer has been already serialized - write only it's id
				*this << i->second;
				return;
			}

			//give id to this pointer
			ui32 pid = (ui32)savedPointers.size();
			savedPointers[actualPointer] = pid;
			*this << pid;
		}

		//write type identifier
		ui16 tid = typeList.getTypeID(data);
		*this << tid;

		this->savePointerHlp(tid, data);
	}

	//that part of ptr serialization was extracted to allow customization of its behavior in derived classes
	template <typename T>
	void savePointerHlp(ui16 tid, const T &data)
	{
		if(!tid)
			*this << *data;	 //if type is unregistered simply write all data in a standard way
		else
			savers[tid]->savePtr(*this, typeList.castToMostDerived(data));  //call serializer specific for our real type
	}

	template <typename T>
	void saveArray(const T &data)
	{
		ui32 size = ARRAY_COUNT(data);
		for(ui32 i=0; i < size; i++)
			*this << data[i];
	}
	template <typename T>
	void save(const T &data)
	{
		typedef
			//if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Boolean> >,
			mpl::identity<SaveBoolean>,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<BooleanVector> >,
			mpl::identity<SaveBooleanVector>,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<SavePrimitive<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Enum> >,
			mpl::identity<SaveEnum<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Pointer> >,
			mpl::identity<SavePointer<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Array> >,
			mpl::identity<SaveArray<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Serializable> >,
			mpl::identity<SaveSerializable<T> >,
			//else
			mpl::identity<SaveWrong<T> >
			>
			>
			>
			>
			>
			>
			>::type typex;
		typex::invoke(* this, data);
	}
	template <typename T>
	void saveSerializable(const T &data)
	{
		const_cast<T&>(data).serialize(*this,version);
	}
	template <typename T>
	void saveSerializable(const std::shared_ptr<T> &data)
	{
		T *internalPtr = data.get();
		*this << internalPtr;
	}
	template <typename T>
	void saveSerializable(const std::unique_ptr<T> &data)
	{
		T *internalPtr = data.get();
		*this << internalPtr;
	}
	template <typename T>
	void saveSerializable(const std::vector<T> &data)
	{
		ui32 length = data.size();
		*this << length;
		for(ui32 i=0;i<length;i++)
			*this << data[i];
	}
	template <typename T, size_t N>
	void saveSerializable(const std::array<T, N> &data)
	{
		for(ui32 i=0; i < N; i++)
			*this << data[i];
	}
	template <typename T>
	void saveSerializable(const std::set<T> &data)
	{
		std::set<T> &d = const_cast<std::set<T> &>(data);
		ui32 length = d.size();
		*this << length;
		for(typename std::set<T>::iterator i=d.begin();i!=d.end();i++)
			*this << *i;
	}
	template <typename T, typename U>
	void saveSerializable(const std::unordered_set<T, U> &data)
	{
		std::unordered_set<T, U> &d = const_cast<std::unordered_set<T, U> &>(data);
		ui32 length = d.size();
		*this << length;
		for(typename std::unordered_set<T, U>::iterator i=d.begin();i!=d.end();i++)
			*this << *i;
	}
	template <typename T>
	void saveSerializable(const std::list<T> &data)
	{
		std::list<T> &d = const_cast<std::list<T> &>(data);
		ui32 length = d.size();
		*this << length;
		for(typename std::list<T>::iterator i=d.begin();i!=d.end();i++)
			*this << *i;
	}
	void saveSerializable(const std::string &data)
	{
		*this << ui32(data.length());
		this->write(data.c_str(),data.size());
	}
	template <typename T1, typename T2>
	void saveSerializable(const std::pair<T1,T2> &data)
	{
		*this << data.first << data.second;
	}
	template <typename T1, typename T2>
	void saveSerializable(const std::map<T1,T2> &data)
	{
		*this << ui32(data.size());
		for(typename std::map<T1,T2>::const_iterator i=data.begin();i!=data.end();i++)
			*this << i->first << i->second;
	}
	template <typename T1, typename T2>
	void saveSerializable(const std::multimap<T1, T2> &data)
	{
		*this << ui32(data.size());
		for(typename std::map<T1, T2>::const_iterator i = data.begin(); i != data.end(); i++)
			*this << i->first << i->second;
	}
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void saveSerializable(const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &data)
	{
		si32 which = data.which();
		*this << which;

		VariantVisitorSaver<COSer> visitor(*this);
		boost::apply_visitor(visitor, data);
	}
	template <typename T>
	void saveSerializable(const boost::optional<T> &data)
	{
		if(data)
		{
			*this << (ui8)1;
			*this << *data;
		}
		else
		{
			*this << (ui8)0;
		}
	}
	template <typename E>
	void saveEnum(const E &data)
	{
		si32 writ = static_cast<si32>(data);
		*this << writ;
	}
	void saveBoolean(const bool & data)
	{
		ui8 writ = static_cast<ui8>(data);
		*this << writ;
	}
	void saveBooleanVector(const std::vector<bool> & data)
	{
		std::vector<ui8> convData;
		std::copy(data.begin(), data.end(), std::back_inserter(convData));
		saveSerializable(convData);
	}
};

class IBinaryReader : public virtual CSerializer
{
public:
	virtual int read(void * data, unsigned size) = 0;
};

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

class CBasicPointerLoader
{
public:
	virtual const std::type_info * loadPtr(CLoaderBase &ar, void *data, ui32 pid) const =0; //data is pointer to the ACTUAL POINTER
	virtual ~CBasicPointerLoader(){}
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


/// The class which manages loading of objects.
class DLL_LINKAGE CISer : public CLoaderBase
{
public:
	struct LoadBoolean
	{
		static void invoke(CISer &s, bool &data)
		{
			s.loadBoolean(data);
		}
	};

	struct LoadBooleanVector
	{
		static void invoke(CISer &s, std::vector<bool> &data)
		{
			s.loadBooleanVector(data);
		}
	};

	template<typename T>
	struct LoadEnum
	{
		static void invoke(CISer &s, T &data)
		{
			s.loadEnum(data);
		}
	};

	template<typename T>
	struct LoadPrimitive
	{
		static void invoke(CISer &s, T &data)
		{
			s.loadPrimitive(data);
		}
	};

	template<typename T>
	struct LoadPointer
	{
		static void invoke(CISer &s, T &data)
		{
			s.loadPointer(data);
		}
	};

	template<typename T>
	struct LoadArray
	{
		static void invoke(CISer &s, T &data)
		{
			s.loadArray(data);
		}
	};

	template<typename T>
	struct LoadSerializable
	{
		static void invoke(CISer &s, T &data)
		{
			s.loadSerializable(data);
		}
	};

	template<typename T>
	struct LoadWrong
	{
		static void invoke(CISer &s, const T &data)
		{
			throw std::runtime_error("Wrong load serialization call!");
		}
	};

	template <typename T> class CPointerLoader : public CBasicPointerLoader
	{
	public:
		const std::type_info * loadPtr(CLoaderBase &ar, void *data, ui32 pid) const override //data is pointer to the ACTUAL POINTER
		{
			CISer &s = static_cast<CISer&>(ar);
			T *&ptr = *static_cast<T**>(data);

			//create new object under pointer
			typedef typename boost::remove_pointer<T>::type npT;
			ptr = ClassObjectCreator<npT>::invoke(); //does new npT or throws for abstract classes
			s.ptrAllocated(ptr, pid);
			//T is most derived known type, it's time to call actual serialize
			ptr->serialize(s,version);
			return &typeid(T);
		}
	};

	bool saving;
	std::map<ui16,CBasicPointerLoader*> loaders; // typeID => CPointerSaver<serializer,type>
	si32 fileVersion;
	bool reverseEndianess; //if source has different endianness than us, we reverse bytes

	std::map<ui32, void*> loadedPointers;
	std::map<ui32, const std::type_info*> loadedPointersTypes;
	std::map<const void*, boost::any> loadedSharedPointers;

	bool smartPointerSerialization;

	CISer(IBinaryReader * r): CLoaderBase(r)
	{
		saving = false;
		fileVersion = 0;
		smartPointerSerialization = true;
		reverseEndianess = false;
	}

	~CISer()
	{
		std::map<ui16,CBasicPointerLoader*>::iterator iter;

		for(iter = loaders.begin(); iter != loaders.end(); iter++)
			delete iter->second;
	}

	template<typename T>
	void addLoader(const T * t = nullptr)
	{
		auto ID = typeList.getTypeID(t);
		if(!loaders.count(ID))
			loaders[ID] = new CPointerLoader<T>;
	}

	template<typename Base, typename Derived> void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		typeList.registerType(b, d);
		addLoader(b);
		addLoader(d);
	}

	template<class T>
	CISer & operator>>(T &t)
	{
		this->load(t);
		return * this;
	}

	template<class T>
	CISer & operator&(T & t)
	{
		return * this >> t;
	}

	int write(const void * data, unsigned size);
	template <typename T>
	void load(T &data)
	{
		typedef
			//if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Boolean> >,
			mpl::identity<LoadBoolean>,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<BooleanVector> >,
			mpl::identity<LoadBooleanVector>,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<LoadPrimitive<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Enum> >,
			mpl::identity<LoadEnum<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Pointer> >,
			mpl::identity<LoadPointer<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Array> >,
			mpl::identity<LoadArray<T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Serializable> >,
			mpl::identity<LoadSerializable<T> >,
			//else
			mpl::identity<LoadWrong<T> >
			>
			>
			>
			>
			>
			>
			>::type typex;
		typex::invoke(* this, data);
	}
	template <typename T>
	void loadPrimitive(T &data)
	{
		if(0) //for testing #989
		{
 			this->read(&data,sizeof(data));
		}
		else
		{
			unsigned length = sizeof(data);
			char* dataPtr = (char*)&data;
			this->read(dataPtr,length);
			if(reverseEndianess)
				std::reverse(dataPtr, dataPtr + length);
		}
	}

	template <typename T>
	void loadSerializableBySerializeCall(T &data)
	{
		////that const cast is evil because it allows to implicitly overwrite const objects when deserializing
		typedef typename boost::remove_const<T>::type nonConstT;
		nonConstT &hlp = const_cast<nonConstT&>(data);
		hlp.serialize(*this,fileVersion);
		//data.serialize(*this,myVersion);
	}

	template <typename T>
	void loadSerializable(T &data)
	{
		loadSerializableBySerializeCall(data);
	}
	template <typename T>
	void loadArray(T &data)
	{
		ui32 size = ARRAY_COUNT(data);
		for(ui32 i = 0; i < size; i++)
			*this >> data[i];
	}
	template <typename T>
	void loadPointer(T &data)
	{
		ui8 hlp;
		*this >> hlp;
		if(!hlp)
		{
			data = nullptr;
			return;
		}

		if(reader->smartVectorMembersSerialization)
		{
			typedef typename boost::remove_const<typename boost::remove_pointer<T>::type>::type TObjectType; //eg: const CGHeroInstance * => CGHeroInstance
			typedef typename VectorisedTypeFor<TObjectType>::type VType;									 //eg: CGHeroInstance -> CGobjectInstance
			typedef typename VectorizedIDType<TObjectType>::type IDType;
			if(const auto *info = reader->getVectorisedTypeInfo<VType, IDType>())
			{
				IDType id;
				*this >> id;
				if(id != IDType(-1))
				{
					data = static_cast<T>(reader->getVectorItemFromId<VType, IDType>(*info, id));
					return;
				}
			}
		}

		if(reader->sendStackInstanceByIds)
		{
			bool gotLoaded = LoadIfStackInstance<CISer,T>::invoke(* this, data);
			if(gotLoaded)
				return;
		}

		ui32 pid = 0xffffffff; //pointer id (or maybe rather pointee id)
		if(smartPointerSerialization)
		{
			*this >> pid; //get the id
			std::map<ui32, void*>::iterator i = loadedPointers.find(pid); //lookup

			if(i != loadedPointers.end())
			{
				// We already got this pointer
				// Cast it in case we are loading it to a non-first base pointer
				assert(loadedPointersTypes.count(pid));
				data = reinterpret_cast<T>(typeList.castRaw(i->second, loadedPointersTypes.at(pid), &typeid(typename boost::remove_const<typename boost::remove_pointer<T>::type>::type)));
				return;
			}
		}

		//get type id
		ui16 tid;
		*this >> tid;
		this->loadPointerHlp(tid, data, pid);
	}

	//that part of ptr deserialization was extracted to allow customization of its behavior in derived classes
	template <typename T>
	void loadPointerHlp( ui16 tid, T & data, ui32 pid )
	{
		if(!tid)
		{
			typedef typename boost::remove_pointer<T>::type npT;
			typedef typename boost::remove_const<npT>::type ncpT;
			data = ClassObjectCreator<ncpT>::invoke();
			ptrAllocated(data, pid);
			*this >> *data;
		}
		else
		{
			auto typeInfo = loaders[tid]->loadPtr(*this,&data, pid);
			data = reinterpret_cast<T>(typeList.castRaw((void*)data, typeInfo, &typeid(typename boost::remove_const<typename boost::remove_pointer<T>::type>::type)));
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

#define READ_CHECK_U32(x)			\
	ui32 length;			\
	*this >> length;				\
	if(length > 500000)				\
	{								\
        logGlobal->warnStream() << "Warning: very big length: " << length;\
        reader->reportState(logGlobal);			\
	};


	template <typename T>
	void loadSerializable(std::shared_ptr<T> &data)
	{
		typedef typename boost::remove_const<T>::type NonConstT;
		NonConstT *internalPtr;
		*this >> internalPtr;

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
						// No casting needed, just unpack already stored std::shared_ptr and return it
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
	void loadSerializable(std::unique_ptr<T> &data)
	{
		T *internalPtr;
		*this >> internalPtr;
		data.reset(internalPtr);
	}
	template <typename T>
	void loadSerializable(std::vector<T> &data)
	{
		READ_CHECK_U32(length);
		data.resize(length);
		for(ui32 i=0;i<length;i++)
			*this >> data[i];
	}
	template <typename T, size_t N>
	void loadSerializable(std::array<T, N> &data)
	{
		for(ui32 i = 0; i < N; i++)
			*this >> data[i];
	}
	template <typename T>
	void loadSerializable(std::set<T> &data)
	{
		READ_CHECK_U32(length);
        data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			*this >> ins;
			data.insert(ins);
		}
	}
	template <typename T, typename U>
	void loadSerializable(std::unordered_set<T, U> &data)
	{
		READ_CHECK_U32(length);
        data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			*this >> ins;
			data.insert(ins);
		}
	}
	template <typename T>
	void loadSerializable(std::list<T> &data)
	{
		READ_CHECK_U32(length);
        data.clear();
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			*this >> ins;
			data.push_back(ins);
		}
	}
	template <typename T1, typename T2>
	void loadSerializable(std::pair<T1,T2> &data)
	{
		*this >> data.first >> data.second;
	}

	template <typename T1, typename T2>
	void loadSerializable(std::map<T1,T2> &data)
	{
		READ_CHECK_U32(length);
        data.clear();
		T1 key;
		T2 value;
		for(ui32 i=0;i<length;i++)
		{
			*this >> key >> value;
			data.insert(std::pair<T1, T2>(std::move(key), std::move(value)));
		}
	}
	template <typename T1, typename T2>
	void loadSerializable(std::multimap<T1, T2> &data)
	{
		READ_CHECK_U32(length);
		data.clear();
		T1 key;
		T2 value;
		for(ui32 i = 0; i < length; i++)
		{
			*this >> key >> value;
			data.insert(std::pair<T1, T2>(std::move(key), std::move(value)));
		}
	}
	void loadSerializable(std::string &data)
	{
		READ_CHECK_U32(length);
		data.resize(length);
		this->read((void*)data.c_str(),length);
	}

	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void loadSerializable(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &data)
	{
		typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> TVariant;

		VariantLoaderHelper<TVariant, CISer> loader(*this);

		si32 which;
		*this >> which;
		assert(which < loader.funcs.size());
		data = loader.funcs.at(which)();
	}

	template <typename T>
	void loadSerializable(boost::optional<T> & data)
	{
		ui8 present;
		*this >> present;
		if(present)
		{
			T t;
			*this >> t;
			data = t;
		}
		else
		{
			data = boost::optional<T>();
		}
	}
// 	void loadSerializable(CStackInstance *&s)
// 	{
// 		if(sendStackInstanceByIds)
// 		{
// 			CArmedInstance *armed;
// 			SlotID slot;
// 			*this >> armed >> slot;
// 			assert(armed->hasStackAtSlot(slot));
// 			s = armed->stacks[slot];
// 		}
// 		else
// 			loadSerializableBySerializeCall(s);
// 	}

	template <typename E>
	void loadEnum(E &data)
	{
		si32 read;
		*this >> read;
		data = static_cast<E>(read);
	}
	void loadBoolean(bool &data)
	{
		ui8 read;
		*this >> read;
		data = static_cast<bool>(read);
	}
	void loadBooleanVector(std::vector<bool> & data)
	{
		std::vector<ui8> convData;
		loadSerializable(convData);
		convData.resize(data.size());
		range::copy(convData, data.begin());
	}
};

class DLL_LINKAGE CSaveFile
	:public IBinaryWriter
{
public:

	COSer serializer;

	boost::filesystem::path fName;
	std::unique_ptr<FileStream> sfile;

	CSaveFile(const boost::filesystem::path &fname); //throws!
	~CSaveFile();
	int write(const void * data, unsigned size) override;

	void openNextFile(const boost::filesystem::path &fname); //throws!
	void clear();
    void reportState(CLogger * out) override;

	void putMagicBytes(const std::string &text);

	template<class T>
	CSaveFile & operator<<(const T &t)
	{
		serializer << t;
		return * this;
	}
};

class DLL_LINKAGE CLoadFile
	: public IBinaryReader
{
public:
	CISer serializer;

	boost::filesystem::path fName;
	std::unique_ptr<FileStream> sfile;

	CLoadFile(const boost::filesystem::path & fname, int minimalVersion = version); //throws!
	~CLoadFile();
	int read(void * data, unsigned size) override; //throws!

	void openNextFile(const boost::filesystem::path & fname, int minimalVersion); //throws!
	void clear();
    void reportState(CLogger * out) override;

	void checkMagicBytes(const std::string & text);

	template<class T>
	CLoadFile & operator>>(T &t)
	{
		serializer >> t;
		return * this;
	}
};

class DLL_LINKAGE CLoadIntegrityValidator
	: public IBinaryReader
{
public:
	CISer serializer;
	std::unique_ptr<CLoadFile> primaryFile, controlFile;
	bool foundDesync;

	CLoadIntegrityValidator(const boost::filesystem::path &primaryFileName, const boost::filesystem::path &controlFileName, int minimalVersion = version); //throws!

	int read( void * data, unsigned size) override; //throws!
	void checkMagicBytes(const std::string &text);

	std::unique_ptr<CLoadFile> decay(); //returns primary file. CLoadIntegrityValidator stops being usable anymore
};

typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > TSocket;
typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > TAcceptor;

class DLL_LINKAGE CConnection
	: public IBinaryReader, public IBinaryWriter
{
	//CGameState *gs;
	CConnection(void);

	void init();
    void reportState(CLogger * out) override;
public:
	CISer iser;
	COSer oser;

	boost::mutex *rmx, *wmx; // read/write mutexes
	TSocket * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if endianness is different we'll have to revert received multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	int connectionID;
	boost::thread *handler;

	bool receivedStop, sendStop;

	CConnection(std::string host, std::string port, std::string Name);
	CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name);
	CConnection(TSocket * Socket, std::string Name); //use immediately after accepting connection into socket

	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
	void close();
	bool isOpen() const;
    template<class T>
    CConnection &operator&(const T&);
	virtual ~CConnection(void);

	CPack *retreivePack(); //gets from server next pack (allocates it with new)
	void sendPackToServer(const CPack &pack, PlayerColor player, ui32 requestID);

	void disableStackSendingByID();
	void enableStackSendingByID();
	void disableSmartPointerSerialization();
	void enableSmartPointerSerializatoin();
	void disableSmartVectorMemberSerialization();
	void enableSmartVectorMemberSerializatoin();

	void prepareForSendingHeroes(); //disables sending vectorised, enables smart pointer serialization, clears saved/loaded ptr cache
	void enterPregameConnectionMode();

	template<class T>
	CConnection & operator>>(T &t)
	{
		iser >> t;
		return * this;
	}

	template<class T>
	CConnection & operator<<(const T &t)
	{
		oser << t;
		return * this;
	}
};

DLL_LINKAGE std::ostream &operator<<(std::ostream &str, const CConnection &cpc);


// Serializer that stores objects in the dynamic buffer. Allows performing deep object copies.
class DLL_LINKAGE CMemorySerializer
	: public IBinaryReader, public IBinaryWriter
{
	std::vector<ui8> buffer;

	size_t readPos; //index of the next byte to be read
public:
	CISer iser;
	COSer oser;

	int read(void * data, unsigned size) override; //throws!
	int write(const void * data, unsigned size) override;

	CMemorySerializer();

	template <typename T>
	static std::unique_ptr<T> deepCopy(const T &data)
	{
		CMemorySerializer mem;
		mem.oser << &data;

		std::unique_ptr<T> ret;
		mem.iser >> ret;
		return ret;
	}
};

template<typename T>
class CApplier
{
public:
	std::map<ui16,T*> apps;

	~CApplier()
	{
		typename std::map<ui16, T*>::iterator iter;

		for(iter = apps.begin(); iter != apps.end(); iter++)
			delete iter->second;
	}

	template<typename RegisteredType>
	void addApplier(ui16 ID)
	{
		if(!apps.count(ID))
		{
			RegisteredType * rtype = nullptr;
			apps[ID] = T::getApplier(rtype);
		}
	}

	template<typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		typeList.registerType(b, d);
		addApplier<Base>(typeList.getTypeID(b));
		addApplier<Derived>(typeList.getTypeID(d));
	}

};
