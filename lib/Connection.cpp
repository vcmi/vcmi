#define VCMI_DLL
#pragma warning(disable:4355)
#include "Connection.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <fstream>

using namespace boost;
using namespace boost::asio::ip;
template<typename Serializer> DLL_EXPORT void registerTypes(Serializer &s); //defined elsewhere and explicitly instantiated for used serializers

CTypeList typeList;

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
	registerTypes(static_cast<CISer<CConnection>&>(*this));
	registerTypes(static_cast<COSer<CConnection>&>(*this));
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
	tlog0 << "Established connection with "<<pom<<std::endl;
	wmx = new boost::mutex;
	rmx = new boost::mutex;
}

CConnection::CConnection(std::string host, std::string port, std::string Name)
:io_service(new asio::io_service), name(Name)
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
		tlog1 << error <<std::endl;
	else
		tlog1 << "No error info. " << std::endl;
	delete io_service;
	//delete socket;	
	throw std::string("Can't establish connection :(");
}
CConnection::CConnection(
			boost::asio::basic_stream_socket<boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > * Socket, 
			std::string Name )
			:socket(Socket),io_service(&Socket->io_service()), name(Name)//, send(this), rec(this)
{
	init();
}
CConnection::CConnection(boost::asio::basic_socket_acceptor<boost::asio::ip::tcp,boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > * acceptor, 
						 boost::asio::io_service *Io_service, std::string Name)
: name(Name)//, send(this), rec(this)
{
	boost::system::error_code error = asio::error::host_not_found;
	socket = new tcp::socket(*io_service);
	acceptor->accept(*socket,error);
	if (error)
	{ 
		tlog1 << "Error on accepting: " << std::endl << error << std::endl;
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

CSaveFile::CSaveFile( const std::string &fname )
	:sfile(new std::ofstream(fname.c_str(),std::ios::binary))
{
	registerTypes(*this);
	if(!(*sfile))
	{
		tlog1 << "Error: cannot open to write " << fname << std::endl;
		sfile = NULL;
	}
}

CSaveFile::~CSaveFile()
{
	delete sfile;
}

int CSaveFile::write( const void * data, unsigned size )
{
	sfile->write((char *)data,size);
	return size;
}

CLoadFile::CLoadFile( const std::string &fname )
:sfile(new std::ifstream(fname.c_str(),std::ios::binary))
{
	registerTypes(*this);
	if(!(*sfile))
	{
		tlog1 << "Error: cannot open to read " << fname << std::endl;
		sfile = NULL;
	}
}

CLoadFile::~CLoadFile()
{
	delete sfile;
}

int CLoadFile::read( const void * data, unsigned size )
{
	sfile->read((char *)data,size);
	return size;
}

CTypeList::CTypeList()
{
	registerTypes(*this);
}

ui16 CTypeList::registerType( const std::type_info *type )
{
	TTypeMap::const_iterator i = types.find(type);
	if(i != types.end())
		return i->second; //type found, return ID

	//type not found - add it to the list and return given ID
	ui16 id = types.size() + 1;
	types.insert(std::make_pair(type,id));
	return id;
}

ui16 CTypeList::getTypeID( const std::type_info *type )
{
	TTypeMap::const_iterator i = types.find(type);
	if(i != types.end())
		return i->second;
	else
		return 0;
}