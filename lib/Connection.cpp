#define VCMI_DLL
#include "Connection.h"
#include <boost/asio.hpp>
using namespace boost;
using namespace boost::asio::ip;

#define LOG(a) \
	if(logging)\
		out << a
#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__)
#define BIG_ENDIAN
#else
#define LIL_ENDIAN
#endif


CConnection::CConnection(std::string host, std::string port, std::string Name, std::ostream & Out)
:io_service(new asio::io_service), name(Name), out(Out)
{
#ifdef LIL_ENDIAN
	myEndianess = true;
#else
	myEndianess = false;
#endif
    system::error_code error = asio::error::host_not_found;
	socket = new tcp::socket(*io_service);
    tcp::resolver resolver(*io_service);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(tcp::resolver::query(host,port));
    socket->connect(*endpoint_iterator, error);
	if(error)
	{ 
		connected = false;
		return;
	}
	std::string pom;
	//we got connection
	(*this) << std::string("Aiya!\n") << name << myEndianess; //identify ourselves
	(*this) >> pom >> pom >> contactEndianess;
}
CConnection::CConnection(
			boost::asio::basic_stream_socket<boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, 
			boost::asio::io_service *Io_service, 
			std::string Name, 
			std::ostream & Out	)
:socket(Socket),io_service(Io_service), out(Out), name(Name)
{
	std::string pom;
	//we start with just connected socket
	(*this) << std::string("Aiya!\n") << name << myEndianess; //identify ourselves
	(*this) >> pom >> pom >> contactEndianess;
}
int CConnection::write(const void * data, unsigned size)
{
	LOG("wysylam dane o rozmiarze " << size << std::endl);
	int ret;
	ret = asio::write(*socket,asio::const_buffers_1(asio::const_buffer(data,size)));
	return ret;
}
int CConnection::read(void * data, unsigned size)
{
	int ret = asio::read(*socket,asio::mutable_buffers_1(asio::mutable_buffer(data,size)));
	return ret;
}
CConnection::~CConnection(void)
{
	delete io_service;
	delete socket;
}
