#pragma once
#include "../global.h"
#include <boost/cstdint.hpp>
#include <string>
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
		template <typename Protocol,
			typename StreamSocketService>
		class basic_stream_socket;
	}
};

class DLL_EXPORT CConnection
{
	std::ostream &out;
	CConnection(void);
public:
	boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if ednianess is different we'll have to revert recieved multi-byte vars
    boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	CConnection(std::string host, std::string port, std::string Name, std::ostream & Out);
	CConnection(boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, boost::asio::io_service *Io_service, std::string Name, std::ostream & Out); //use immediately after accepting connection into socket
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
	data.reserve(length);
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
	data = new char[length];
	c.read(data,length);
	return c;
}
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