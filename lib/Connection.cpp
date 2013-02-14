#include "StdInc.h"
#include "Connection.h"

#ifndef _MSC_VER
#include "RegisterTypes.h"
#endif

//for smart objs serialization over net
#include "Mapping/CMapInfo.h"
#include "StartInfo.h"
#include "BattleState.h"
#include "CGameState.h"
#include "Mapping/CMap.h"
#include "CModHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include "CTownHandler.h"
#include "Mapping/CCampaignHandler.h"
#include "NetPacks.h"
#include "CDefObjInfoHandler.h"

#include <boost/asio.hpp>

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
template<typename Serializer> DLL_LINKAGE void registerTypes(Serializer &s); //defined elsewhere and explicitly instantiated for used serializers

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
	enableSmartPointerSerializatoin();
	disableStackSendingByID();
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
	throw std::runtime_error("Can't establish connection :(");
}
CConnection::CConnection(TSocket * Socket, std::string Name )
	:socket(Socket),io_service(&Socket->get_io_service()), name(Name)//, send(this), rec(this)
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
		throw std::runtime_error("Can't establish connection :("); 
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
	throw std::exception();
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

void CConnection::sendPackToServer(const CPack &pack, TPlayerColor player, ui32 requestID)
{
	boost::unique_lock<boost::mutex> lock(*wmx);
	tlog5 << "Sending to server a pack of type " << typeid(pack).name() << std::endl;
	*this << player << requestID << &pack; //packs has to be sent as polymorphic pointers!
}

void CConnection::disableStackSendingByID()
{
	CISer<CConnection>::sendStackInstanceByIds = false;
	COSer<CConnection>::sendStackInstanceByIds = false;
}

void CConnection::enableStackSendingByID()
{
	CISer<CConnection>::sendStackInstanceByIds = true;
	COSer<CConnection>::sendStackInstanceByIds = true;
}

void CConnection::disableSmartPointerSerialization()
{
	CISer<CConnection>::smartPointerSerialization = false;
	COSer<CConnection>::smartPointerSerialization = false;
}

void CConnection::enableSmartPointerSerializatoin()
{
	CISer<CConnection>::smartPointerSerialization = true;
	COSer<CConnection>::smartPointerSerialization = true;
}

void CConnection::prepareForSendingHeroes()
{
	loadedPointers.clear();
	savedPointers.clear();
	CISer<CConnection>::smartVectorMembersSerialization = false;
	COSer<CConnection>::smartVectorMembersSerialization = false;
	enableSmartPointerSerializatoin();
}

void CConnection::enterPregameConnectionMode()
{
	loadedPointers.clear();
	savedPointers.clear();
	CISer<CConnection>::smartVectorMembersSerialization = false;
	COSer<CConnection>::smartVectorMembersSerialization = false;
	disableSmartPointerSerialization();
}

CSaveFile::CSaveFile( const std::string &fname )
{
	registerTypes(*this);
	openNextFile(fname);
}

CSaveFile::~CSaveFile()
{
}

int CSaveFile::write( const void * data, unsigned size )
{
	sfile->write((char *)data,size);
	return size;
}

void CSaveFile::openNextFile(const std::string &fname)
{
	fName = fname;
	try
	{
		sfile = make_unique<std::ofstream>(fname.c_str(), std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to write %s!", fname);

		sfile->write("VCMI",4); //write magic identifier
		*this << version; //write format version
	}
	catch(...)
	{
		tlog1 << "Failed to save to " << fname << std::endl;
		clear();
		throw;
	}
}

void CSaveFile::reportState(CLogger &out)
{
	out << "CSaveFile" << std::endl;
	if(sfile.get() && *sfile)
	{
		out << "\tOpened " << fName << "\n\tPosition: " << sfile->tellp() << std::endl;
	}
}

void CSaveFile::clear()
{
	fName.clear();
	sfile = nullptr;
}

CLoadFile::CLoadFile(const std::string &fname, int minimalVersion /*= version*/)
{
	registerTypes(*this);
	openNextFile(fname, minimalVersion);
}

CLoadFile::~CLoadFile()
{
}

int CLoadFile::read( const void * data, unsigned size )
{
	sfile->read((char *)data,size);
	return size;
}

void CLoadFile::openNextFile(const std::string &fname, int minimalVersion)
{
	assert(!reverseEndianess);
	assert(minimalVersion <= version);

	try
	{
		fName = fname;
		sfile = make_unique<std::ifstream>(fname, std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to read %s!", fname);

		//we can read
		char buffer[4];
		sfile->read(buffer, 4);
		if(std::memcmp(buffer,"VCMI",4))
			THROW_FORMAT("Error: not a VCMI file(%s)!", fname);

		*this >> fileVersion;	
		if(fileVersion < minimalVersion)
			THROW_FORMAT("Error: too old file format (%s)!", fname);

		if(fileVersion > version)
		{
			tlog3 << boost::format("Warning format version mismatch: found %d when current is %d! (file %s)\n") % fileVersion % version % fname;

			auto versionptr = (char*)&fileVersion;
			std::reverse(versionptr, versionptr + 4);
			tlog3 << "Version number reversed is " << fileVersion << ", checking...\n";

			if(fileVersion == version)
			{
				tlog3 << fname << " seems to have different endianess! Entering reversing mode.\n";
				reverseEndianess = true;
			}
			else
				THROW_FORMAT("Error: too new file format (%s)!", fname);
		}
	}
	catch(...)
	{
		clear(); //if anything went wrong, we delete file and rethrow
		throw;
	}
}

void CLoadFile::reportState(CLogger &out)
{
	out << "CLoadFile" << std::endl;
	if(!!sfile && *sfile)
	{
		out << "\tOpened " << fName << "\n\tPosition: " << sfile->tellg() << std::endl;
	}
}

void CLoadFile::clear()
{
	sfile = nullptr;
	fName.clear();
	fileVersion = 0;
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
	sendStackInstanceByIds = false;
}


void CSerializer::addStdVecItems(CGameState *gs, LibClasses *lib)
{
	registerVectoredType(&gs->map->objects, &CGObjectInstance::id);
	registerVectoredType(&lib->heroh->heroes, &CHero::ID);
	registerVectoredType(&lib->creh->creatures, &CCreature::idNumber);
	registerVectoredType(&lib->arth->artifacts, &CArtifact::id);
	registerVectoredType(&gs->map->artInstances, &CArtifactInstance::id);
	registerVectoredType(&gs->map->quests, &CQuest::qid);
	smartVectorMembersSerialization = true;
}
