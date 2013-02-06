
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

#include <typeinfo> //XXX this is in namespace std if you want w/o use typeinfo.h?

#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/variant.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/any.hpp>

#include "ConstTransitivePtr.h"
#include "CCreatureSet.h" //for CStackInstance
#include "CObjectHandler.h" //for CArmedInstance
#include "Mapping/CCampaignHandler.h" //for CCampaignState

const ui32 version = 735;
const TSlot COMMANDER_SLOT_PLACEHOLDER = -2;

class CConnection;
class CGObjectInstance;
class CStackInstance;
class CGameState;
class CCreature;
class LibClasses;
class CHero;
struct CPack;
extern DLL_LINKAGE LibClasses * VLC;
namespace mpl = boost::mpl;

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
		return a->before(*b);
	}
};

class DLL_LINKAGE CTypeList
{
	typedef std::multimap<const std::type_info *,ui16,TypeComparer> TTypeMap;
	TTypeMap types;
public:
	CTypeList();
	ui16 registerType(const std::type_info *type);
	template <typename T> ui16 registerType(const T * t = NULL)
	{
		return registerType(getTypeInfo(t));
	}

	ui16 getTypeID(const std::type_info *type);
	template <typename T> ui16 getTypeID(const T * t = NULL)
	{
		return getTypeID(getTypeInfo(t));
	}


	template <typename T> const std::type_info * getTypeInfo(const T * t = NULL)
	{
		if(t)
			return &typeid(*t);
		else
			return &typeid(T);
	}
};

extern DLL_LINKAGE CTypeList typeList;


template<typename Ser>
struct SaveBoolean
{
	static void invoke(Ser &s, const bool &data)
	{
		s.saveBoolean(data);
	}
};
template<typename Ser>
struct LoadBoolean
{
	static void invoke(Ser &s, bool &data)
	{
		s.loadBoolean(data);
	}
};

template<typename Ser>
struct SaveBooleanVector
{
	static void invoke(Ser &s, const std::vector<bool> &data)
	{
		s.saveBooleanVector(data);
	}
};
template<typename Ser>
struct LoadBooleanVector
{
	static void invoke(Ser &s, std::vector<bool> &data)
	{
		s.loadBooleanVector(data);
	}
};

template<typename Ser,typename T>
struct SavePrimitive
{
	static void invoke(Ser &s, const T &data)
	{
		s.savePrimitive(data);
	}
};
template<typename Ser,typename T>
struct SaveSerializable
{
	static void invoke(Ser &s, const T &data)
	{
		s.saveSerializable(data);
	}
};

template<typename Ser,typename T>
struct SaveEnum
{
	static void invoke(Ser &s, const T &data)
	{
		s.saveEnum(data);
	}
};
template<typename Ser,typename T>
struct LoadEnum
{
	static void invoke(Ser &s, T &data)
	{
		s.loadEnum(data);
	}
};
template<typename Ser,typename T>
struct LoadPrimitive
{
	static void invoke(Ser &s, T &data)
	{
		s.loadPrimitive(data);
	}
};
template<typename Ser,typename T>
struct SavePointer
{
	static void invoke(Ser &s, const T &data)
	{
		s.savePointer(data);
	}
};
template<typename Ser,typename T>
struct LoadPointer
{
	static void invoke(Ser &s, T &data)
	{
		s.loadPointer(data);
	}
};
template<typename Ser,typename T>
struct SaveArray
{
	static void invoke(Ser &s, const T &data)
	{
		s.saveArray(data);
	}
};
template<typename Ser,typename T>
struct LoadArray
{
	static void invoke(Ser &s, T &data)
	{
		s.loadArray(data);
	}
};
template<typename Ser,typename T>
struct LoadSerializable
{
	static void invoke(Ser &s, T &data)
	{
		s.loadSerializable(data);
	}
};

template<typename Ser,typename T>
struct SaveWrong
{
	static void invoke(Ser &s, const T &data)
	{
		throw std::runtime_error("Wrong save serialization call!");
	}
};
template<typename Ser,typename T>
struct LoadWrong
{
	static void invoke(Ser &s, const T &data)
	{
		throw std::runtime_error("Wrong load serialization call!");
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

template <typename T>
struct VectorisedObjectInfo
{
	const std::vector<ConstTransitivePtr<T> > *vector;	//pointer to the appropriate vector
	const si32 T::*idPtr;			//pointer to the field representing the position in the vector

	VectorisedObjectInfo(const std::vector< ConstTransitivePtr<T> > *Vector, const si32 T::*IdPtr)
		:vector(Vector), idPtr(IdPtr)
	{
	}
};

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

	virtual void reportState(CLogger &out){};

	template <typename T>
	void registerVectoredType(const std::vector<T*> *Vector, const si32 T::*IdPtr)
	{
		vectors[&typeid(T)] = VectorisedObjectInfo<T>(Vector, IdPtr);
	}
	template <typename T>
	void registerVectoredType(const std::vector<ConstTransitivePtr<T> > *Vector, const si32 T::*IdPtr)
	{
		vectors[&typeid(T)] = VectorisedObjectInfo<T>(Vector, IdPtr);
	}

	template <typename T>
	const VectorisedObjectInfo<T> *getVectorisedTypeInfo()
	{
		const std::type_info *myType = NULL;
//
// 		if(boost::is_base_of<CGObjectInstance, T>::value) //ugly workaround to support also types derived from CGObjectInstance -> if we encounter one, treat it aas CGObj..
// 			myType = &typeid(CGObjectInstance);
// 		else
			 myType = &typeid(T);

		TTypeVecMap::iterator i = vectors.find(myType);
		if(i == vectors.end())
			return NULL;
		else
		{
			assert(!i->second.empty());
			assert(i->second.type() == typeid(VectorisedObjectInfo<T>));
			VectorisedObjectInfo<T> *ret = &(boost::any_cast<VectorisedObjectInfo<T>&>(i->second));
			return ret;
		}
	}

	template <typename T>
	T* getVectorItemFromId(const VectorisedObjectInfo<T> &oInfo, ui32 id) const
	{
	/*	if(id < 0)
			return NULL;*/

		assert(oInfo.vector);
		assert(oInfo.vector->size() > id);
		return const_cast<T*>((*oInfo.vector)[id].get());
	}

	template <typename T>
	si32 getIdFromVectorItem(const VectorisedObjectInfo<T> &oInfo, const T* obj) const
	{
		if(!obj)
			return -1;

		return obj->*oInfo.idPtr;
	}

	void addStdVecItems(CGameState *gs, LibClasses *lib = VLC);
};

class DLL_LINKAGE CSaverBase : public virtual CSerializer
{
};

class CBasicPointerSaver
{
public:
	virtual void savePtr(CSaverBase &ar, const void *data) const =0;
	virtual ~CBasicPointerSaver(){}
};

template <typename Serializer, typename T> class CPointerSaver : public CBasicPointerSaver
{
public:
	void savePtr(CSaverBase &ar, const void *data) const
	{
		Serializer &s = static_cast<Serializer&>(ar);
		const T *ptr = static_cast<const T*>(data);

		//T is most derived known type, it's time to call actual serialize
		const_cast<T&>(*ptr).serialize(s,version);
	}
};

template <typename T> //metafunction returning CGObjectInstance if T is its derivate or T elsewise
struct VectorisedTypeFor
{
	typedef typename
		//if
		mpl::eval_if<boost::is_base_of<CGObjectInstance,T>,
		mpl::identity<CGObjectInstance>,
		//else
		mpl::identity<T>
		>::type type;
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
		TSlot slot = -1;

		if(data->getNodeType() == CBonusSystemNode::COMMANDER)
			slot = COMMANDER_SLOT_PLACEHOLDER;
		else
			slot = data->armyObj->findStack(data);

		assert(slot != -1);
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
		TSlot slot;
		s >> armedObj >> slot;
		if(slot != COMMANDER_SLOT_PLACEHOLDER)
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
template <typename Serializer> class DLL_LINKAGE COSer : public CSaverBase
{
public:
	bool saving;
	std::map<ui16,CBasicPointerSaver*> savers; // typeID => CPointerSaver<serializer,type>

	std::map<const void*, ui32> savedPointers;
	bool smartPointerSerialization;

	COSer()
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

	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		savers[ID] = new CPointerSaver<COSer<Serializer>,T>;
	}

    Serializer * This()
	{
		return static_cast<Serializer*>(this);
	}

	template<class T>
	Serializer & operator<<(const T &t)
	{
		this->This()->save(t);
		return * this->This();
	}

	template<class T>
	COSer & operator&(const T & t)
	{
		return * this->This() << t;
	}



	int write(const void * data, unsigned size);
	template <typename T>
	void savePrimitive(const T &data)
	{
		this->This()->write(&data,sizeof(data));
	}

	template <typename T>
	void savePointer(const T &data)
	{
		//write if pointer is not NULL
		ui8 hlp = (data!=NULL);
		*this << hlp;

		//if pointer is NULL then we don't need anything more...
		if(!hlp)
			return;

 		if(smartVectorMembersSerialization)
		{
			typedef typename boost::remove_const<typename boost::remove_pointer<T>::type>::type TObjectType;
			typedef typename VectorisedTypeFor<TObjectType>::type VType;
 			if(const VectorisedObjectInfo<VType> *info = getVectorisedTypeInfo<VType>())
 			{
				si32 id = getIdFromVectorItem<VType>(*info, data);
				*this << id;
				if(id != -1) //vector id is enough
 					return;
 			}
 		}

		if(sendStackInstanceByIds)
		{
			const bool gotSaved = SaveIfStackInstance<Serializer,T>::invoke(*This(), data);
			if(gotSaved)
				return;
		}

		if(smartPointerSerialization)
		{
			std::map<const void*,ui32>::iterator i = savedPointers.find(data);
			if(i != savedPointers.end())
			{
				//this pointer has been already serialized - write only it's id
				*this << i->second;
				return;
			}

			//give id to this pointer
			ui32 pid = (ui32)savedPointers.size();
			savedPointers[data] = pid;
			*this << pid;
		}

		//write type identifier
		ui16 tid = typeList.getTypeID(data);
		*this << tid;

		This()->savePointerHlp(tid, data);
	}

	//that part of ptr serialization was extracted to allow customization of its behavior in derived classes
	template <typename T>
	void savePointerHlp(ui16 tid, const T &data)
	{
		if(!tid)
			*this << *data;	 //if type is unregistered simply write all data in a standard way
		else
			savers[tid]->savePtr(*this,data);  //call serializer specific for our real type
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
			mpl::identity<SaveBoolean<Serializer> >,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<BooleanVector> >,
			mpl::identity<SaveBooleanVector<Serializer> >,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<SavePrimitive<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Enum> >,
			mpl::identity<SaveEnum<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Pointer> >,
			mpl::identity<SavePointer<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Array> >,
			mpl::identity<SaveArray<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Serializable> >,
			mpl::identity<SaveSerializable<Serializer,T> >,
			//else
			mpl::identity<SaveWrong<Serializer,T> >
			>
			>
			>
			>
			>
			>
			>::type typex;
		typex::invoke(* this->This(), data);
	}
	template <typename T>
	void saveSerializable(const T &data)
	{
		const_cast<T&>(data).serialize(*this,version);
	}
	template <typename T>
	void saveSerializable(const shared_ptr<T> &data)
	{
		T *internalPtr = data.get();
		*this << internalPtr;
	}
	template <typename T>
	void saveSerializable(const unique_ptr<T> &data)
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
	void saveSerializable(const boost::unordered_set<T, U> &data)
	{
		boost::unordered_set<T, U> &d = const_cast<boost::unordered_set<T, U> &>(data);
		ui32 length = d.size();
		*this << length;
		for(typename boost::unordered_set<T, U>::iterator i=d.begin();i!=d.end();i++)
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
		this->This()->write(data.c_str(),data.size());
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
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void saveSerializable(const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &data)
	{
		si32 which = data.which();
		*this << which;

		VariantVisitorSaver<Serializer> visitor(*this->This());
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



class DLL_LINKAGE CLoaderBase : public virtual CSerializer
{};

class CBasicPointerLoader
{
public:
	virtual void loadPtr(CLoaderBase &ar, void *data, ui32 pid) const =0; //data is pointer to the ACTUAL POINTER
	virtual ~CBasicPointerLoader(){}
};

template <typename Serializer, typename T> class CPointerLoader : public CBasicPointerLoader
{
public:
	void loadPtr(CLoaderBase &ar, void *data, ui32 pid) const //data is pointer to the ACTUAL POINTER
	{
		Serializer &s = static_cast<Serializer&>(ar);
		T *&ptr = *static_cast<T**>(data);

		//create new object under pointer
		typedef typename boost::remove_pointer<T>::type npT;
		ptr = new npT;
		s.ptrAllocated(ptr, pid);
		//T is most derived known type, it's time to call actual serialize
		ptr->serialize(s,version);
	}
};

/// The class which manages loading of objects.
template <typename Serializer> class DLL_LINKAGE CISer : public CLoaderBase
{
public:
	bool saving;
	std::map<ui16,CBasicPointerLoader*> loaders; // typeID => CPointerSaver<serializer,type>
	ui32 fileVersion;
	bool reverseEndianess; //if source has different endianess than us, we reverse bytes

	std::map<ui32, void*> loadedPointers;
	bool smartPointerSerialization;

	CISer()
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

	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		loaders[ID] = new CPointerLoader<CISer<Serializer>,T>;
	}

    Serializer * This()
	{
		return static_cast<Serializer*>(this);
	}

	template<class T>
	Serializer & operator>>(T &t)
	{
		this->This()->load(t);
		return * this->This();
	}

	template<class T>
	CISer & operator&(T & t)
	{
		return * this->This() >> t;
	}

	int write(const void * data, unsigned size);
	template <typename T>
	void load(T &data)
	{
		typedef
			//if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Boolean> >,
			mpl::identity<LoadBoolean<Serializer> >,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<BooleanVector> >,
			mpl::identity<LoadBooleanVector<Serializer> >,
			//else if
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<LoadPrimitive<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Enum> >,
			mpl::identity<LoadEnum<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Pointer> >,
			mpl::identity<LoadPointer<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Array> >,
			mpl::identity<LoadArray<Serializer,T> >,
			//else if
			typename mpl::eval_if<mpl::equal_to<SerializationLevel<T>,mpl::int_<Serializable> >,
			mpl::identity<LoadSerializable<Serializer,T> >,
			//else
			mpl::identity<LoadWrong<Serializer,T> >
			>
			>
			>
			>
			>
			>
			>::type typex;
		typex::invoke(* this->This(), data);
	}
	template <typename T>
	void loadPrimitive(T &data)
	{
		if(0) //for testing #989
		{
 			this->This()->read(&data,sizeof(data));
		}
		else
		{
			unsigned length = sizeof(data);
			char* dataPtr = (char*)&data;
			this->This()->read(dataPtr,length);
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
			data = NULL;
			return;
		}

		if(smartVectorMembersSerialization)
		{
			typedef typename boost::remove_const<typename boost::remove_pointer<T>::type>::type TObjectType; //eg: const CGHeroInstance * => CGHeroInstance
			typedef typename VectorisedTypeFor<TObjectType>::type VType;									 //eg: CGHeroInstance -> CGobjectInstance
			if(const VectorisedObjectInfo<VType> *info = getVectorisedTypeInfo<VType>())
			{
				si32 id;
				*this >> id;
				if(id != -1)
				{
					data = static_cast<T>(getVectorItemFromId(*info, id));
					return;
				}
			}
		}

		if(sendStackInstanceByIds)
		{
			bool gotLoaded = LoadIfStackInstance<Serializer,T>::invoke(*This(), data);
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
				//we already got this pointer
				data = static_cast<T>(i->second);
				return;
			}
		}

		//get type id
		ui16 tid;
		*this >> tid;
		This()->loadPointerHlp(tid, data, pid);
	}

	//that part of ptr deserialization was extracted to allow customization of its behavior in derived classes
	template <typename T>
	void loadPointerHlp( ui16 tid, T & data, ui32 pid )
	{
		if(!tid)
		{
			typedef typename boost::remove_pointer<T>::type npT;
			typedef typename boost::remove_const<npT>::type ncpT;
			data = new ncpT;
			ptrAllocated(data, pid);
			*this >> *data;
		}
		else
		{
			loaders[tid]->loadPtr(*this,&data, pid);
		}
	}

	template <typename T>
	void ptrAllocated(const T *ptr, ui32 pid)
	{
		if(smartPointerSerialization && pid != 0xffffffff)
			loadedPointers[pid] = (void*)ptr; //add loaded pointer to our lookup map; cast is to avoid errors with const T* pt
	}

#define READ_CHECK_U32(x)			\
	ui32 length;			\
	*this >> length;				\
	if(length > 500000)				\
	{								\
		tlog2 << "Warning: very big length: " << length << "\n" ;\
		reportState(tlog2);			\
	};


	template <typename T>
	void loadSerializable(shared_ptr<T> &data)
	{
		T *internalPtr;
		*this >> internalPtr;
		data.reset(internalPtr);
	}
	template <typename T>
	void loadSerializable(unique_ptr<T> &data)
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
	template <typename T>
	void loadSerializable(std::set<T> &data)
	{
		READ_CHECK_U32(length);
		T ins;
		for(ui32 i=0;i<length;i++)
		{
			*this >> ins;
			data.insert(ins);
		}
	}
	template <typename T, typename U>
	void loadSerializable(boost::unordered_set<T, U> &data)
	{
		READ_CHECK_U32(length);
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
		T1 t;
		for(ui32 i=0;i<length;i++)
		{
			*this >> t;
			*this >> data[t];
		}
	}
	void loadSerializable(std::string &data)
	{
		READ_CHECK_U32(length);
		data.resize(length);
		this->This()->read((void*)data.c_str(),length);
	}
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void loadSerializable(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &data)
	{
		si32 which;
		*this >> which;
		if(which == 0)
		{
			T0 obj;
			*this >> obj;
			data = obj;
		}
		else if(which == 1)
		{
			T1 obj;
			*this >> obj;
			data = obj;
		}
		else
			assert(0);
		//TODO write more if needed, general solution would be much longer
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
	void loadSerializable(CStackInstance *&s)
	{
		if(sendStackInstanceByIds)
		{
			CArmedInstance *armed;
			TSlot slot;
			*this >> armed >> slot;
			assert(armed->hasStackAtSlot(slot));
			s = armed->stacks[slot];
		}
		else
			loadSerializableBySerializeCall(s);
	}

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
		std::copy(convData.begin(), convData.end(), std::back_inserter(data));
	}
};

class DLL_LINKAGE CSaveFile
	: public COSer<CSaveFile>
{
	void dummyMagicFunction()
	{
		*this << std::string("This function makes stuff working.");
	}
public:
	std::string fName;
	unique_ptr<std::ofstream> sfile;

	CSaveFile(const std::string &fname); //throws!
	~CSaveFile();
	int write(const void * data, unsigned size);

	void openNextFile(const std::string &fname); //throws!
	void clear();
	void reportState(CLogger &out);
};

class DLL_LINKAGE CLoadFile
	: public CISer<CLoadFile>
{
	void dummyMagicFunction()
	{
		std::string dummy = "This function makes stuff working.";
		*this >> dummy;
	}
public:
	std::string fName;
	unique_ptr<std::ifstream> sfile;

	CLoadFile(const std::string &fname, int minimalVersion = version); //throws!
	~CLoadFile();
	int read(const void * data, unsigned size); //throws!

	void openNextFile(const std::string &fname, int minimalVersion); //throws!
	void clear();
	void reportState(CLogger &out);
};

typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > TSocket;
typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > TAcceptor;

class DLL_LINKAGE CConnection
	:public CISer<CConnection>, public COSer<CConnection>
{
	//CGameState *gs;
	CConnection(void);

	void init();
	void reportState(CLogger &out);
public:
	boost::mutex *rmx, *wmx; // read/write mutexes
	TSocket * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if endianess is different we'll have to revert received multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	int connectionID;
	boost::thread *handler;

	bool receivedStop, sendStop;

	CConnection(std::string host, std::string port, std::string Name);
	CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name);
	CConnection(TSocket * Socket, std::string Name); //use immediately after accepting connection into socket

	int write(const void * data, unsigned size);
	int read(void * data, unsigned size);
	void close();
	bool isOpen() const;
    template<class T>
    CConnection &operator&(const T&);
	virtual ~CConnection(void);

	CPack *retreivePack(); //gets from server next pack (allocates it with new)
	void sendPackToServer(const CPack &pack, TPlayerColor player, ui32 requestID);

	void disableStackSendingByID();
	void enableStackSendingByID();
	void disableSmartPointerSerialization();
	void enableSmartPointerSerializatoin();
};

DLL_LINKAGE std::ostream &operator<<(std::ostream &str, const CConnection &cpc);

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
	template<typename U> void registerType(const U * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		apps[ID] = T::getApplier(t);
	}

};
