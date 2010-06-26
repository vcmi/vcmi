#ifndef __CONNECTION_H__
#define __CONNECTION_H__
#include "../global.h"
#include <string>
#include <vector>
#include <set>
#include <list>
#include <typeinfo> //XXX this is in namespace std if you want w/o use typeinfo.h?

#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_pointer.hpp> 
#include <boost/type_traits/is_class.hpp> 
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/any.hpp>

const ui32 version = 723;
class CConnection;
class CGObjectInstance;
class CGameState;
class CCreature;
class LibClasses;
class CHero;
extern DLL_EXPORT LibClasses * VLC;
namespace mpl = boost::mpl;

/*
 * Connection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
};

enum SerializationLvl
{
	Wrong=0,
	Primitive,
	Array,
	Pointer,
	Serializable
};


struct TypeComparer
{
	bool operator()(const std::type_info *a, const std::type_info *b) const
	{
		return a->before(*b);
	}
};

class DLL_EXPORT CTypeList
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
	template <typename T> ui16 getTypeID(const T * t)
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

extern DLL_EXPORT CTypeList typeList;

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
		throw std::string("Wrong save serialization call!");
	}
};
template<typename Ser,typename T>
struct LoadWrong
{
	static void invoke(Ser &s, const T &data)
	{
		throw std::string("Wrong load serialization call!");
	}
};

template<typename T>
struct SerializationLevel
{    
	typedef mpl::integral_c_tag tag;
	typedef
		typename mpl::eval_if<
			boost::is_fundamental<T>,
			mpl::int_<Primitive>,
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
		>::type type;
	static const int value = SerializationLevel::type::value;
};

template <typename T>
struct VectorisedObjectInfo
{
	const std::vector<T*> *vector;	//pointer to the appropriate vector
	const si32 T::*idPtr;			//pointer to the field representing the position in the vector

	VectorisedObjectInfo(const std::vector<T*> *Vector, const si32 T::*IdPtr)
		:vector(Vector), idPtr(IdPtr)
	{
	}
};

class DLL_EXPORT CSerializer
{
public:
	typedef std::map<const std::type_info *, boost::any, TypeComparer> TTypeVecMap;
	TTypeVecMap vectors; //entry must be a pointer to vector containing pointers to the objects of key type

	bool smartVectorMembersSerialization;

	CSerializer();
	~CSerializer();

	template <typename T>
	void registerVectoredType(const std::vector<T*> *Vector, const si32 T::*IdPtr)
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
	T* getVectorItemFromId(const VectorisedObjectInfo<T> &oInfo, si32 id) const
	{
		if(id < 0)
			return NULL;

		assert(oInfo.vector);
		assert(oInfo.vector->size() > id);
		return (*oInfo.vector)[id];
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

class DLL_EXPORT CSaverBase : public virtual CSerializer
{
};

class CBasicPointerSaver
{
public:
	virtual void savePtr(CSaverBase &ar, const void *data) const =0;
	~CBasicPointerSaver(){}
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

template <typename Serializer> class DLL_EXPORT COSer : public CSaverBase
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
 				*this << getIdFromVectorItem<VType>(*info, data);
 				return;
 			}
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
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<SavePrimitive<Serializer,T> >,
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
			>::type typex;
		typex::invoke(* this->This(), data);
	}
	template <typename T>
	void saveSerializable(const T &data)
	{
		const_cast<T&>(data).serialize(*this,version);
	}
	template <typename T>
	void saveSerializable(const std::vector<T> &data)
	{
		boost::uint32_t length = data.size();
		*this << length;
		for(ui32 i=0;i<length;i++)
			*this << data[i];
	}
	template <typename T>
	void saveSerializable(const std::set<T> &data)
	{
		std::set<T> &d = const_cast<std::set<T> &>(data);
		boost::uint32_t length = d.size();
		*this << length;
		for(typename std::set<T>::iterator i=d.begin();i!=d.end();i++)
			*this << *i;
	}
	template <typename T>
	void saveSerializable(const std::list<T> &data)
	{
		std::list<T> &d = const_cast<std::list<T> &>(data);
		boost::uint32_t length = d.size();
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
};



class DLL_EXPORT CLoaderBase : public virtual CSerializer
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

template <typename Serializer> class DLL_EXPORT CISer : public CLoaderBase
{
public:
	bool saving;
	std::map<ui16,CBasicPointerLoader*> loaders; // typeID => CPointerSaver<serializer,type>
	ui32 myVersion;

	std::map<ui32, void*> loadedPointers;
	bool smartPointerSerialization;

	CISer()
	{
		saving = false;
		myVersion = version;
		smartPointerSerialization = true;
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
			typename mpl::eval_if< mpl::equal_to<SerializationLevel<T>,mpl::int_<Primitive> >,
			mpl::identity<LoadPrimitive<Serializer,T> >,
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
			>::type typex;
		typex::invoke(* this->This(), data);
	}
	template <typename T>
	void loadPrimitive(T &data)
	{
		this->This()->read(&data,sizeof(data));
	}
	template <typename T>
	void loadSerializable(T &data)
	{
		////that const cast is evil because it allows to implicitly overwrite const objects when deserializing
		typedef typename boost::remove_const<T>::type nonConstT;
		nonConstT &hlp = const_cast<nonConstT&>(data);
		hlp.serialize(*this,myVersion);
		//data.serialize(*this,myVersion);
	}	
	template <typename T>
	void loadArray(T &data)
	{
		ui32 size = ARRAY_COUNT(data);
		for(ui32 i=0; i < size; i++)
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
				data = static_cast<T>(getVectorItemFromId(*info, id));
				return;
			}
		}

		ui32 pid = -1; //pointer id (or maybe rather pointee id) 
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
		if(smartPointerSerialization && pid != -1)
			loadedPointers[pid] = (void*)ptr; //add loaded pointer to our lookup map; cast is to avoid errors with const T* pt
	}

	template <typename T>
	void loadSerializable(std::vector<T> &data)
	{
		boost::uint32_t length;
		*this >> length;
		data.resize(length);
		for(ui32 i=0;i<length;i++)
			*this >> data[i];
	}
	template <typename T>
	void loadSerializable(std::set<T> &data)
	{
		boost::uint32_t length;
		*this >> length;
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
		boost::uint32_t length;
		*this >> length;
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
		ui32 length;
		*this >> length;
		T1 t;
		for(ui32 i=0;i<length;i++)
		{
			*this >> t;
			*this >> data[t];
		}
	}
	void loadSerializable(std::string &data)
	{
		ui32 length;
		*this >> length;
		data.resize(length);
		this->This()->read((void*)data.c_str(),length);
	}

};


class DLL_EXPORT CSaveFile
	: public COSer<CSaveFile>
{
	void dummyMagicFunction()
	{
		*this << std::string("This function makes stuff working.");
	}
public:
	std::ofstream *sfile;
	CSaveFile(const std::string &fname);
	~CSaveFile();
	int write(const void * data, unsigned size);

	void close();
	void openNextFile(const std::string &fname);
};

class DLL_EXPORT CLoadFile
	: public CISer<CLoadFile>
{
	void dummyMagicFunction()
	{
		std::string dummy = "This function makes stuff working.";
		*this >> dummy;
	}
public:
	std::ifstream *sfile;
	CLoadFile(const std::string &fname, bool requireLatest = true);
	~CLoadFile();
	int read(const void * data, unsigned size);

	void close();
	void openNextFile(const std::string &fname, bool requireLatest);
};

typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > TSocket;
typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > TAcceptor;

class DLL_EXPORT CConnection
	:public CISer<CConnection>, public COSer<CConnection>
{
	//CGameState *gs;
	CConnection(void);

	void init();
public:
	boost::mutex *rmx, *wmx; // read/write mutexes
	TSocket * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if ednianess is different we'll have to revert recieved multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	CConnection(std::string host, std::string port, std::string Name);
	CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name);
	CConnection(TSocket * Socket, std::string Name); //use immediately after accepting connection into socket

	int write(const void * data, unsigned size);
	int read(void * data, unsigned size);
	void close();
	bool isOpen() const;
    template<class T>
    CConnection &operator&(const T&);
	~CConnection(void);
};

#endif // __CONNECTION_H__
