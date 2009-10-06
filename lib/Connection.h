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
#include  <boost/type_traits/remove_pointer.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/type_traits/is_array.hpp>
const ui32 version = 710;
class CConnection;
class CGObjectInstance;
class CGameState;
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

class DLL_EXPORT CSerializerBase
{
public:
};

class DLL_EXPORT CSaverBase : public virtual CSerializerBase
{
};

class CBasicPointerSaver
{
public:
	virtual void savePtr(CSaverBase &ar, const void *data) const =0;
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

template <typename Serializer> class DLL_EXPORT COSer : public CSaverBase
{
public:
	bool saving;
	std::map<ui16,CBasicPointerSaver*> savers; // typeID => CPointerSaver<serializer,type>

	COSer()
	{
		saving=true;
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



class DLL_EXPORT CLoaderBase : public virtual CSerializerBase
{};

class CBasicPointerLoader
{
public:
	virtual void loadPtr(CLoaderBase &ar, void *data) const =0; //data is pointer to the ACTUAL POINTER
};

template <typename Serializer, typename T> class CPointerLoader : public CBasicPointerLoader
{
public:
	void loadPtr(CLoaderBase &ar, void *data) const //data is pointer to the ACTUAL POINTER
	{
		Serializer &s = static_cast<Serializer&>(ar);
		T *&ptr = *static_cast<T**>(data);

		//create new object under pointer
		typedef typename boost::remove_pointer<T>::type npT;
		ptr = new npT;

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

	CISer()
	{
		saving = false;
		myVersion = version;
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
		////that const cast would be evil because it allows to implicitly overwrite const objects when deserializing
		//typedef typename boost::remove_const<T>::type nonConstT;
		//nonConstT &hlp = const_cast<nonConstT&>(data);
		//hlp.serialize(*this,myVersion);
		data.serialize(*this,myVersion);
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

		//get type id
		ui16 tid;
		*this >> tid;
		This()->loadPointerHlp(tid, data);
	}

	//that part of ptr deserialization was extracted to allow customization of its behavior in derived classes
	template <typename T>
	void loadPointerHlp( ui16 tid, T & data )
	{
		if(!tid)
		{
			typedef typename boost::remove_pointer<T>::type npT;
			data = new npT;
			*this >> *data;
		}
		else
		{
			loaders[tid]->loadPtr(*this,&data);
		}
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
	CLoadFile(const std::string &fname);
	~CLoadFile();
	int read(const void * data, unsigned size);
};

class DLL_EXPORT CConnection
	:public CISer<CConnection>, public COSer<CConnection>
{
	CGameState *gs;
	CConnection(void);

	void init();
public:
	boost::mutex *rmx, *wmx; // read/write mutexes
	boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if ednianess is different we'll have to revert recieved multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	CConnection
		(std::string host, std::string port, std::string Name);
	CConnection
		(boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > * acceptor, 
		boost::asio::io_service *Io_service, std::string Name);
	CConnection
		(boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, 
		std::string Name); //use immediately after accepting connection into socket
	int write(const void * data, unsigned size);
	int read(void * data, unsigned size);
	int readLine(void * data, unsigned maxSize);
	void close();
    template<class T>
    CConnection &operator&(const T&);
	~CConnection(void);
	void setGS(CGameState *state);

	CGObjectInstance *loadObject(); //reads id from net and returns that obj
	void saveObject(const CGObjectInstance *data);


	template<typename T>
	struct loadObjectHelper
	{
		static void invoke(CConnection &s, T &data, ui16 tid)
		{
			data = static_cast<T>(s.loadObject());
		}
	};
	template<typename T>
	struct loadRestHelper
	{
		static void invoke(CConnection &s, T &data, ui16 tid)
		{
			s.CISer<CConnection>::loadPointerHlp(tid, data);
		}
	};
	template<typename T>
	struct saveObjectHelper
	{
		static void invoke(CConnection &s, const T &data, ui16 tid)
		{
			//CGObjectInstance *&hlp = const_cast<CGObjectInstance*&>(data); //for loading pointer to const obj we must remove the qualifier
			s.saveObject(data);
		}
	};
	template<typename T>
	struct saveRestHelper
	{
		static void invoke(CConnection &s, const T &data, ui16 tid)
		{
			s.COSer<CConnection>::savePointerHlp(tid, data);
		}
	};


	//"overload" loading pointer procedure
	template <typename T>
	void loadPointerHlp( ui16 tid, T & data )
	{
		typedef typename 
			//if
			mpl::eval_if< boost::is_base_of<CGObjectInstance, typename boost::remove_pointer<T>::type>,
			mpl::identity<loadObjectHelper<T> >,
			//else
			mpl::identity<loadRestHelper<T> >
			>::type typex;
		typex::invoke(*this, data, tid);
	}

	//"overload" saving pointer procedure
	template <typename T>
	void savePointerHlp( ui16 tid, const T & data )
	{
		typedef typename 
			//if
			mpl::eval_if< boost::is_base_of<CGObjectInstance, typename boost::remove_pointer<T>::type>,
				mpl::identity<saveObjectHelper<T> >,
			//else
				mpl::identity<saveRestHelper<T> >
			>::type typex;
		typex::invoke(*this, data, tid);
	}
};

#endif // __CONNECTION_H__
