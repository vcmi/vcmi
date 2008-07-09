#pragma once
#include "../global.h"
#include <string>
#include <vector>
#include <set>
const int version = 63;
class CConnection;

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
};

class CSender
{
public:
	CConnection* c;
	CSender(CConnection* C):c(C){};
	template <typename T> CSender & operator&&(T &data) //send built-in type
	{
		*c << data;
		return *this;
	}
	template <typename T> CSender & operator&(T &data) //send serializable type
	{
		*c < data;
		return *this;
	}
};
class CReceiver
{
public:
	CConnection *c;
	CReceiver(CConnection* C):c(C){};
	template <typename T> CReceiver & operator&&(T &data) //get built-in type
	{
		*c >> data;
		return *this;
	}
	template <typename T> CReceiver & operator&(T &data) //get serializable type
	{
		*c > data;
		return *this;
	}
};
class DLL_EXPORT CConnection
{
	std::ostream &out;
	CConnection(void);
	void init();
public:
	CSender send;
	CReceiver rec;
	boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if ednianess is different we'll have to revert recieved multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	CConnection
		(std::string host, std::string port, std::string Name, std::ostream & Out);
	CConnection
		(boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > * acceptor, 
		boost::asio::io_service *Io_service, std::string Name, std::ostream & Out);
	CConnection
		(boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, 
		std::string Name, std::ostream & Out); //use immediately after accepting connection into socket
	int write(const void * data, unsigned size);
	int read(void * data, unsigned size);
	int readLine(void * data, unsigned maxSize);
	~CConnection(void);
};


template <typename T> CConnection & operator<<(CConnection &c, const T &data);
template <typename T> CConnection & operator>>(CConnection &c, T &data);
CConnection & operator<<(CConnection &c, const std::string &data)
{
	boost::uint32_t length = data.size();
	c << length;
	c.write(data.c_str(),length);
	return c;
}
CConnection & operator>>(CConnection &c, std::string &data)
{
	boost::uint32_t length;
	c >> length;
	data.resize(length);
	c.read((void*)data.c_str(),length);
	return c;
}
CConnection & operator<<(CConnection &c, const char * &data)
{
	boost::uint32_t length = strlen(data);
	c << length;
	c.write(data,length);
	return c;
}
CConnection & operator>>(CConnection &c, char * &data)
{
	boost::uint32_t length;
	c >> length;
	std::cout <<"Alokujemy " <<length << " bajtow."<<std::endl;
	data = new char[length];
	c.read(data,length);
	return c;
}
template <typename T> CConnection & operator<<(CConnection &c, std::vector<T> &data)
{
	boost::uint32_t length = data.size();
	c << length;
	for(ui32 i=0;i<length;i++)
		c << data[i];
	return c;
}
template <typename T> CConnection & operator>>(CConnection &c,  std::vector<T> &data)
{
	boost::uint32_t length;
	c >> length;
	data.resize(length);
	for(ui32 i=0;i<length;i++)
		c >> data[i];
	return c;
}
//template <typename T> CConnection & operator<<(CConnection &c, std::set<T> &data)
//{
//	boost::uint32_t length = data.size();
//	c << length;
//	for(std::set<T>::iterator i=data.begin();i!=data.end();i++)
//		c << *i;
//	return c;
//}
//template <typename T> CConnection & operator>>(CConnection &c,  std::set<T> &data)
//{
//	boost::uint32_t length;
//	c >> length;
//	data.resize(length);
//	T pom;
//	for(int i=0;i<length;i++)
//	{
//		c >> pom;
//		data.insert(pom);
//	}
//	return c;
//}
template <typename T> CConnection & operator<<(CConnection &c, const T &data)
{
	c.write(&data,sizeof(data));
	return c;
}
template <typename T> CConnection & operator>>(CConnection &c, T &data)
{
	c.read(&data,sizeof(data));
	return c;
}
template <typename T> CConnection & operator<(CConnection &c, std::vector<T> &data)
{
	boost::uint32_t length = data.size();
	c << length;
	for(ui32 i=0;i<length;i++)
		data[i].serialize(c.send,version);
	return c;
}
template <typename T> CConnection & operator>(CConnection &c,  std::vector<T> &data)
{
	boost::uint32_t length;
	c >> length;
	data.resize(length);
	for(ui32 i=0;i<length;i++)
		data[i].serialize(c.rec,version);
	return c;
}
template <typename T> CConnection & operator<(CConnection &c, T &data)
{
	data.serialize(c.send,version);
	return c;
}
template <typename T> CConnection & operator>(CConnection &c, T &data)
{
	data.serialize(c.rec,version);
	return c;
}