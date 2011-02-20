#define VCMI_DLL
#pragma warning(disable:4355)
#include "Connection.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <fstream>

#ifndef _MSC_VER
#include "../lib/RegisterTypes.cpp"
#endif

//for smart objs serialization over net
#include "../lib/CMapInfo.h"
#include "../StartInfo.h"
#include "BattleState.h"
#include "CGameState.h"
#include "map.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include "CTownHandler.h"
#include "CCampaignHandler.h"
#include "NetPacks.h"


/*
 * Connection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
	CISer<CConnection>::smartPointerSerialization = false;
	COSer<CConnection>::smartPointerSerialization = false;
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

	handler = NULL;
	receivedStop = sendStop = false;
	static int cid = 1;
	connectionID = cid++;
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
CConnection::CConnection(TSocket * Socket, std::string Name )
	:socket(Socket),io_service(&Socket->io_service()), name(Name)//, send(this), rec(this)
{
	init();
}
CConnection::CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name)
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
	try
	{
		int ret;
		ret = asio::write(*socket,asio::const_buffers_1(asio::const_buffer(data,size)));
		return ret;
	}
	catch(...)
	{
		//connection has been lost
		connected = false;
		throw;
	}
}
int CConnection::read(void * data, unsigned size)
{
	//LOG("Receiving " << size << " byte(s) of data" <<std::endl);
	try
	{
		int ret = asio::read(*socket,asio::mutable_buffers_1(asio::mutable_buffer(data,size)));
		return ret;
	}
	catch(...)
	{
		//connection has been lost
		connected = false;
		throw;
	}
}
CConnection::~CConnection(void)
{
	if(handler)
		handler->join();

	delete handler;

	close();
 	delete io_service;
 	delete wmx;
 	delete rmx;
}

template<class T>
CConnection & CConnection::operator&(const T &t) {
    throw new std::exception();
//XXX this is temporaly ? solution to fix gcc (4.3.3, other?) compilation
//    problem for more details contact t0@czlug.icis.pcz.pl or impono@gmail.com
//    do not remove this exception it shoudnt be called
    return *this;
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

bool CConnection::isOpen() const
{
	return socket && connected;
}

void CConnection::reportState(CLogger &out)
{
	out << "CConnection\n";
	if(socket && socket->is_open())
	{
		out << "\tWe have an open and valid socket\n";
		out << "\t" << socket->available() <<" bytes awaiting\n";
	}
}

CPack * CConnection::retreivePack()
{
	CPack *ret = NULL;
	boost::unique_lock<boost::mutex> lock(*rmx);
	tlog5 << "Listening... ";
	*this >> ret;
	tlog5 << "\treceived server message of type " << typeid(*ret).name() << std::endl;
	return ret;
}

CSaveFile::CSaveFile( const std::string &fname )
	:sfile(NULL)
{
	registerTypes(*this);
	openNextFile(fname);
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

void CSaveFile::close()
{
	delete sfile;
	sfile = NULL;
}

void CSaveFile::openNextFile(const std::string &fname)
{
	fName = fname;
	close();
	sfile = new std::ofstream(fname.c_str(),std::ios::binary);
	if(!(*sfile))
	{
		tlog1 << "Error: cannot open to write " << fname << std::endl;
		sfile = NULL;
	}
	else
	{
		sfile->write("VCMI",4); //write magic identifier
		*this << version; //write format version
	}
}

void CSaveFile::reportState(CLogger &out)
{
	out << "CSaveFile" << std::endl;
	if(sfile && *sfile)
	{
		out << "\tOpened " << fName << "\n\tPosition: " << sfile->tellp() << std::endl;
	}
}

CLoadFile::CLoadFile(const std::string &fname, int minimalVersion /*= version*/)
:sfile(NULL)
{
	registerTypes(*this);
	openNextFile(fname, minimalVersion);
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

void CLoadFile::close()
{
	delete sfile;
	sfile = NULL;
}

void CLoadFile::openNextFile(const std::string &fname, int minimalVersion)
{
	fName = fname;
	sfile = new std::ifstream(fname.c_str(),std::ios::binary);
	if(!(*sfile))
	{
		tlog1 << "Error: cannot open to read " << fname << std::endl;
		sfile = NULL;
	}
	else
	{
		char buffer[4];
		sfile->read(buffer, 4);

		if(std::memcmp(buffer,"VCMI",4))
		{
			tlog1 << "Error: not a VCMI file! ( " << fname << " )\n";
			delete sfile;
			sfile = NULL;
			return;
		}

		*this >> myVersion;	
		if(myVersion < minimalVersion)
		{
			tlog1 << "Error: Old file format! (file " << fname << " )\n";
			delete sfile;
			sfile = NULL;
		}
	}
}

void CLoadFile::reportState(CLogger &out)
{
	out << "CLoadFile" << std::endl;
	if(sfile && *sfile)
	{
		out << "\tOpened " << fName << "\n\tPosition: " << sfile->tellg() << std::endl;
	}
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

 std::ostream & operator<<(std::ostream &str, const CConnection &cpc)
 {
 	return str << "Connection with " << cpc.name << " (ID: " << cpc.connectionID << /*", " << (cpc.host ? "host" : "guest") <<*/ ")";
 }

CSerializer::~CSerializer()
{

}

CSerializer::CSerializer()
{
	smartVectorMembersSerialization = false;
}


void CSerializer::addStdVecItems(CGameState *gs, LibClasses *lib)
{
	registerVectoredType(&gs->map->objects, &CGObjectInstance::id);
	registerVectoredType(&lib->heroh->heroes, &CHero::ID);
	registerVectoredType(&lib->creh->creatures, &CCreature::idNumber);
	registerVectoredType(&lib->arth->artifacts, &CArtifact::id);
	registerVectoredType(&gs->map->artInstances, &CArtifactInstance::id);
	smartVectorMembersSerialization = true;
}
