#define VCMI_DLL
#pragma warning(disable:4355)
#include "Connection.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
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

void CConnection::init()
{
#ifdef LIL_ENDIAN
	myEndianess = true;
#else
	myEndianess = false;
#endif
	connected = true;
	std::string pom;
	//we got connection
	(*this) << std::string("Aiya!\n") << name << myEndianess; //identify ourselves
	(*this) >> pom >> pom >> contactEndianess;
	out << "Established connection with "<<pom<<std::endl;
	wmx = new boost::mutex;
	rmx = new boost::mutex;
}

CConnection::CConnection(std::string host, std::string port, std::string Name, std::ostream & Out)
:io_service(new asio::io_service), name(Name), out(Out)//, send(this), rec(this)
{
	int i;
	boost::system::error_code error = asio::error::host_not_found;
	socket = new tcp::socket(*io_service);
    tcp::resolver resolver(*io_service);
    tcp::resolver::iterator end, pom, endpoint_iterator = resolver.resolve(tcp::resolver::query(host,port),error);
	if(error)
	{
		tlog1 << "Problem with resolving: " << std::endl << error <<std::endl;
		goto connerror1;
	}
	pom = endpoint_iterator;
	if(pom != end)
		tlog0<<"Found endpoints:" << std::endl;
	else
	{
		tlog1 << "Critical problem: No endpoints found!" << std::endl;
		goto connerror1;
	}
	i=0;
	while(pom != end)
	{
		tlog0 << "\t" << i << ": " << (boost::asio::ip::tcp::endpoint&)*pom << std::endl;
		pom++;
	}
	i=0;
	while(endpoint_iterator != end)
	{
		tlog0 << "Trying connection to " << (boost::asio::ip::tcp::endpoint&)*endpoint_iterator << "  (" << i++ << ")" << std::endl;
		socket->connect(*endpoint_iterator, error);
		if(!error)
		{
			init();
			return;
		}
		else
		{
			tlog1 << "Problem with connecting: " << std::endl <<  error << std::endl;
		}
		endpoint_iterator++;
	}

	//we shouldn't be here - error handling
connerror1:
	tlog1 << "Something went wrong... checking for error info" << std::endl;
	if(error)
		std::cout << error <<std::endl;
	else
		tlog1 << "No error info. " << std::endl;
	delete io_service;
	//delete socket;	
	throw std::string("Can't establish connection :(");
}
CConnection::CConnection(
			boost::asio::basic_stream_socket<boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, 
			std::string Name, 
			std::ostream & Out	)
			:socket(Socket),io_service(&Socket->io_service()), out(Out), name(Name)//, send(this), rec(this)
{
	init();
}
CConnection::CConnection(boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > * acceptor, boost::asio::io_service *Io_service, std::string Name, std::ostream & Out)
: out(Out), name(Name)//, send(this), rec(this)
{
	boost::system::error_code error = asio::error::host_not_found;
	socket = new tcp::socket(*io_service);
	acceptor->accept(*socket,error);
	if (error)
	{ 
		std::cout << "Error on accepting: " << std::endl << error << std::endl;
		delete socket;	
		throw "Can't establish connection :("; 
	}
	init();
}
int CConnection::write(const void * data, unsigned size)
{
	//LOG("Sending " << size << " byte(s) of data" <<std::endl);
	int ret;
	ret = asio::write(*socket,asio::const_buffers_1(asio::const_buffer(data,size)));
	return ret;
}
int CConnection::read(void * data, unsigned size)
{
	//LOG("Receiving " << size << " byte(s) of data" <<std::endl);
	int ret = asio::read(*socket,asio::mutable_buffers_1(asio::mutable_buffer(data,size)));
	return ret;
}
CConnection::~CConnection(void)
{
	close();
	delete io_service;
	delete wmx;
	delete rmx;
}

void CConnection::close()
{
	if(socket)
	{
		socket->close();
		delete socket;
		socket = NULL;
	}
}
template <>
void CConnection::saveSerializable<std::string>(const std::string &data)
{
	*this << ui32(data.size());
	write(data.c_str(),data.size());
}

template <>
void CConnection::loadSerializable<std::string>(std::string &data)
{
	ui32 l;
	*this >> l;
	data.resize(l);
	read((void*)data.c_str(),l);
}
