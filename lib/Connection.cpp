#include "StdInc.h"
#include "Connection.h"

#include "registerTypes/RegisterTypes.h"
#include "mapping/CMap.h"
#include "CGameState.h"
#include "filesystem/FileStream.h"

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

extern template void registerTypes<CISer>(CISer & s);
extern template void registerTypes<COSer>(COSer & s);
extern template void registerTypes<CTypeList>(CTypeList & s);

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
	boost::asio::ip::tcp::no_delay option(true);
	socket->set_option(option);

	enableSmartPointerSerializatoin();
	disableStackSendingByID();
	registerTypes(iser);
	registerTypes(oser);
#ifdef LIL_ENDIAN
	myEndianess = true;
#else
	myEndianess = false;
#endif
	connected = true;
	std::string pom;
	//we got connection
	oser << std::string("Aiya!\n") << name << myEndianess; //identify ourselves
	iser >> pom >> pom >> contactEndianess;
	logNetwork->infoStream() << "Established connection with "<<pom;
	wmx = new boost::mutex;
	rmx = new boost::mutex;

	handler = nullptr;
	receivedStop = sendStop = false;
	static int cid = 1;
	connectionID = cid++;
}

CConnection::CConnection(std::string host, std::string port, std::string Name)
:iser(this), oser(this), io_service(new boost::asio::io_service), name(Name)
{
	int i;
	boost::system::error_code error = boost::asio::error::host_not_found;
	socket = new boost::asio::ip::tcp::socket(*io_service);
	boost::asio::ip::tcp::resolver resolver(*io_service);
	boost::asio::ip::tcp::resolver::iterator end, pom, endpoint_iterator = resolver.resolve(boost::asio::ip::tcp::resolver::query(host,port),error);
	if(error)
	{
		logNetwork->errorStream() << "Problem with resolving: \n" << error;
		goto connerror1;
	}
	pom = endpoint_iterator;
	if(pom != end)
		logNetwork->infoStream()<<"Found endpoints:";
	else
	{
		logNetwork->errorStream() << "Critical problem: No endpoints found!";
		goto connerror1;
	}
	i=0;
	while(pom != end)
	{
		logNetwork->infoStream() << "\t" << i << ": " << (boost::asio::ip::tcp::endpoint&)*pom;
		pom++;
	}
	i=0;
	while(endpoint_iterator != end)
	{
		logNetwork->infoStream() << "Trying connection to " << (boost::asio::ip::tcp::endpoint&)*endpoint_iterator << "  (" << i++ << ")";
		socket->connect(*endpoint_iterator, error);
		if(!error)
		{
			init();
			return;
		}
		else
		{
			logNetwork->errorStream() << "Problem with connecting: " <<  error;
		}
		endpoint_iterator++;
	}

	//we shouldn't be here - error handling
connerror1:
	logNetwork->errorStream() << "Something went wrong... checking for error info";
	if(error)
		logNetwork->errorStream() << error;
	else
		logNetwork->errorStream() << "No error info. ";
	delete io_service;
	//delete socket;
	throw std::runtime_error("Can't establish connection :(");
}
CConnection::CConnection(TSocket * Socket, std::string Name )
	:iser(this), oser(this), socket(Socket),io_service(&Socket->get_io_service()), name(Name)//, send(this), rec(this)
{
	init();
}
CConnection::CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name)
: iser(this), oser(this), name(Name)//, send(this), rec(this)
{
	boost::system::error_code error = boost::asio::error::host_not_found;
	socket = new boost::asio::ip::tcp::socket(*io_service);
	acceptor->accept(*socket,error);
	if (error)
	{
		logNetwork->errorStream() << "Error on accepting: " << error;
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
		ret = boost::asio::write(*socket,boost::asio::const_buffers_1(boost::asio::const_buffer(data,size)));
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
		int ret = boost::asio::read(*socket,boost::asio::mutable_buffers_1(boost::asio::mutable_buffer(data,size)));
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
//	throw std::exception();
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
		socket = nullptr;
	}
}

bool CConnection::isOpen() const
{
	return socket && connected;
}

void CConnection::reportState(CLogger * out)
{
	out->debugStream() << "CConnection";
	if(socket && socket->is_open())
	{
		out->debugStream() << "\tWe have an open and valid socket";
		out->debugStream() << "\t" << socket->available() <<" bytes awaiting";
	}
}

CPack * CConnection::retreivePack()
{
	CPack *ret = nullptr;
	boost::unique_lock<boost::mutex> lock(*rmx);
	logNetwork->traceStream() << "Listening... ";
	iser >> ret;
	logNetwork->traceStream() << "\treceived server message of type " << typeid(*ret).name() << ", data: " << ret;
	return ret;
}

void CConnection::sendPackToServer(const CPack &pack, PlayerColor player, ui32 requestID)
{
	boost::unique_lock<boost::mutex> lock(*wmx);
	logNetwork->traceStream() << "Sending to server a pack of type " << typeid(pack).name();
	oser << player << requestID << &pack; //packs has to be sent as polymorphic pointers!
}

void CConnection::disableStackSendingByID()
{
	CSerializer::sendStackInstanceByIds = false;
}

void CConnection::enableStackSendingByID()
{
	CSerializer::sendStackInstanceByIds = true;
}

void CConnection::disableSmartPointerSerialization()
{
	iser.smartPointerSerialization = oser.smartPointerSerialization = false;
}

void CConnection::enableSmartPointerSerializatoin()
{
	iser.smartPointerSerialization = oser.smartPointerSerialization = true;
}

void CConnection::prepareForSendingHeroes()
{
	iser.loadedPointers.clear();
	oser.savedPointers.clear();
	disableSmartVectorMemberSerialization();
	enableSmartPointerSerializatoin();
	disableStackSendingByID();
}

void CConnection::enterPregameConnectionMode()
{
	iser.loadedPointers.clear();
	oser.savedPointers.clear();
	disableSmartVectorMemberSerialization();
	disableSmartPointerSerialization();
}

void CConnection::disableSmartVectorMemberSerialization()
{
	CSerializer::smartVectorMembersSerialization = false;
}

void CConnection::enableSmartVectorMemberSerializatoin()
{
	CSerializer::smartVectorMembersSerialization = true;
}

CSaveFile::CSaveFile( const boost::filesystem::path &fname ): serializer(this)
{
	registerTypes(serializer);
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

void CSaveFile::openNextFile(const boost::filesystem::path &fname)
{
	fName = fname;
	try
	{
		sfile = make_unique<FileStream>(fname, std::ios::out | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to write %s!", fname);

		sfile->write("VCMI",4); //write magic identifier
		serializer << version; //write format version
	}
	catch(...)
	{
		logGlobal->errorStream() << "Failed to save to " << fname;
		clear();
		throw;
	}
}

void CSaveFile::reportState(CLogger * out)
{
	out->debugStream() << "CSaveFile";
	if(sfile.get() && *sfile)
	{
		out->debugStream() << "\tOpened " << fName << "\n\tPosition: " << sfile->tellp();
	}
}

void CSaveFile::clear()
{
	fName.clear();
	sfile = nullptr;
}

void CSaveFile::putMagicBytes( const std::string &text )
{
	write(text.c_str(), text.length());
}

CLoadFile::CLoadFile(const boost::filesystem::path & fname, int minimalVersion /*= version*/): serializer(this)
{
	registerTypes(serializer);
	openNextFile(fname, minimalVersion);
}

CLoadFile::~CLoadFile()
{
}

int CLoadFile::read(void * data, unsigned size)
{
	sfile->read((char*)data,size);
	return size;
}

void CLoadFile::openNextFile(const boost::filesystem::path & fname, int minimalVersion)
{
	assert(!serializer.reverseEndianess);
	assert(minimalVersion <= version);

	try
	{
		fName = fname.string();
		sfile = make_unique<FileStream>(fname, std::ios::in | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to read %s!", fName);

		//we can read
		char buffer[4];
		sfile->read(buffer, 4);
		if(std::memcmp(buffer,"VCMI",4))
			THROW_FORMAT("Error: not a VCMI file(%s)!", fName);

		serializer >> serializer.fileVersion;
		if(serializer.fileVersion < minimalVersion)
			THROW_FORMAT("Error: too old file format (%s)!", fName);

		if(serializer.fileVersion > version)
		{
			logGlobal->warnStream() << boost::format("Warning format version mismatch: found %d when current is %d! (file %s)\n") % serializer.fileVersion % version % fName;

			auto versionptr = (char*)&serializer.fileVersion;
			std::reverse(versionptr, versionptr + 4);
			logGlobal->warnStream() << "Version number reversed is " << serializer.fileVersion << ", checking...";

			if(serializer.fileVersion == version)
			{
				logGlobal->warnStream() << fname << " seems to have different endianness! Entering reversing mode.";
				serializer.reverseEndianess = true;
			}
			else
				THROW_FORMAT("Error: too new file format (%s)!", fName);
		}
	}
	catch(...)
	{
		clear(); //if anything went wrong, we delete file and rethrow
		throw;
	}
}

void CLoadFile::reportState(CLogger * out)
{
	out->debugStream() << "CLoadFile";
	if(!!sfile && *sfile)
	{
		out->debugStream() << "\tOpened " << fName << "\n\tPosition: " << sfile->tellg();
	}
}

void CLoadFile::clear()
{
	sfile = nullptr;
	fName.clear();
	serializer.fileVersion = 0;
}

void CLoadFile::checkMagicBytes( const std::string &text )
{
	std::string loaded = text;
	read((void*)loaded.data(), text.length());
	if(loaded != text)
		throw std::runtime_error("Magic bytes doesn't match!");
}

CTypeList::CTypeList()
{
	registerTypes(*this);
}

CTypeList::TypeInfoPtr CTypeList::registerType( const std::type_info *type )
{
	if(auto typeDescr = getTypeDescriptor(type, false))
		return typeDescr;  //type found, return ptr to structure

	//type not found - add it to the list and return given ID
	auto newType = std::make_shared<TypeDescriptor>();
	newType->typeID = typeInfos.size() + 1;
	newType->name = type->name();
	typeInfos[type] = newType;

	return newType;
}

ui16 CTypeList::getTypeID( const std::type_info *type, bool throws ) const
{
	auto descriptor = getTypeDescriptor(type, throws);
	if (descriptor == nullptr)
	{
		return 0;
	}
	return descriptor->typeID;
}

std::vector<CTypeList::TypeInfoPtr> CTypeList::castSequence(TypeInfoPtr from, TypeInfoPtr to) const
{
	if(!strcmp(from->name, to->name))
		return std::vector<CTypeList::TypeInfoPtr>();

	// Perform a simple BFS in the class hierarchy.

	auto BFS = [&](bool upcast)
	{
		std::map<TypeInfoPtr, TypeInfoPtr> previous;
		std::queue<TypeInfoPtr> q;
		q.push(to);
		while(q.size())
		{
			auto typeNode = q.front();
			q.pop();
			for(auto &nodeBase : upcast ? typeNode->parents : typeNode->children)
			{
				if(!previous.count(nodeBase))
				{
					previous[nodeBase] = typeNode;
					q.push(nodeBase);
				}
			}
		}

		std::vector<TypeInfoPtr> ret;

		if(!previous.count(from))
			return ret;

		ret.push_back(from);
		TypeInfoPtr ptr = from;
		do
		{
			ptr = previous.at(ptr);
			ret.push_back(ptr);
		} while(ptr != to);

		return ret;
	};

	// Try looking both up and down.
	auto ret = BFS(true);
	if(ret.empty())
		ret = BFS(false);

	if(ret.empty())
		THROW_FORMAT("Cannot find relation between types %s and %s. Were they (and all classes between them) properly registered?", from->name % to->name);

	return ret;
}

std::vector<CTypeList::TypeInfoPtr> CTypeList::castSequence(const std::type_info *from, const std::type_info *to) const
{
	//This additional if is needed because getTypeDescriptor might fail if type is not registered
	// (and if casting is not needed, then registereing should no  be required)
	if(!strcmp(from->name(), to->name()))
		return std::vector<CTypeList::TypeInfoPtr>();

	return castSequence(getTypeDescriptor(from), getTypeDescriptor(to));
}

CTypeList::TypeInfoPtr CTypeList::getTypeDescriptor(const std::type_info *type, bool throws) const
{
	auto i = typeInfos.find(type);
	if(i != typeInfos.end())
		return i->second; //type found, return ptr to structure

	if(!throws)
		return nullptr;

	THROW_FORMAT("Cannot find type descriptor for type %s. Was it registered?", type->name());
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
	registerVectoredType<CGObjectInstance, ObjectInstanceID>(&gs->map->objects,
		[](const CGObjectInstance &obj){ return obj.id; });
	registerVectoredType<CHero, HeroTypeID>(&lib->heroh->heroes,
		[](const CHero &h){ return h.ID; });
	registerVectoredType<CGHeroInstance, HeroTypeID>(&gs->map->allHeroes,
		[](const CGHeroInstance &h){ return h.type->ID; });
	registerVectoredType<CCreature, CreatureID>(&lib->creh->creatures,
		[](const CCreature &cre){ return cre.idNumber; });
	registerVectoredType<CArtifact, ArtifactID>(&lib->arth->artifacts,
		[](const CArtifact &art){ return art.id; });
	registerVectoredType<CArtifactInstance, ArtifactInstanceID>(&gs->map->artInstances,
		[](const CArtifactInstance &artInst){ return artInst.id; });
	registerVectoredType<CQuest, si32>(&gs->map->quests,
		[](const CQuest &q){ return q.qid; });

	smartVectorMembersSerialization = true;
}

CLoadIntegrityValidator::CLoadIntegrityValidator( const boost::filesystem::path &primaryFileName, const boost::filesystem::path &controlFileName, int minimalVersion /*= version*/ )
	: serializer(this), foundDesync(false)
{
	registerTypes(serializer);
	primaryFile = make_unique<CLoadFile>(primaryFileName, minimalVersion);
	controlFile = make_unique<CLoadFile>(controlFileName, minimalVersion);

	assert(primaryFile->serializer.fileVersion == controlFile->serializer.fileVersion);
	serializer.fileVersion = primaryFile->serializer.fileVersion;
}

int CLoadIntegrityValidator::read( void * data, unsigned size )
{
	assert(primaryFile);
	assert(controlFile);

	if(!size)
		return size;

	std::vector<ui8> controlData(size);
	auto ret = primaryFile->read(data, size);

	if(!foundDesync)
	{
		controlFile->read(controlData.data(), size);
		if(std::memcmp(data, controlData.data(), size))
		{
			logGlobal->errorStream() << "Desync found! Position: " << primaryFile->sfile->tellg();
			foundDesync = true;
			//throw std::runtime_error("Savegame dsynchronized!");
		}
	}
	return ret;
}

std::unique_ptr<CLoadFile> CLoadIntegrityValidator::decay()
{
	primaryFile->serializer.loadedPointers = this->serializer.loadedPointers;
	primaryFile->serializer.loadedPointersTypes = this->serializer.loadedPointersTypes;
	return std::move(primaryFile);
}

void CLoadIntegrityValidator::checkMagicBytes( const std::string &text )
{
	assert(primaryFile);
	assert(controlFile);

	primaryFile->checkMagicBytes(text);
	controlFile->checkMagicBytes(text);
}

int CMemorySerializer::read(void * data, unsigned size)
{
	if(buffer.size() < readPos + size)
		throw std::runtime_error(boost::str(boost::format("Cannot read past the buffer (accessing index %d, while size is %d)!") % (readPos + size - 1) % buffer.size()));

	std::memcpy(data, buffer.data() + readPos, size);
	readPos += size;
	return size;
}

int CMemorySerializer::write(const void * data, unsigned size)
{
	auto oldSize = buffer.size(); //and the pos to write from
	buffer.resize(oldSize + size);
	std::memcpy(buffer.data() + oldSize, data, size);
	return size;
}

CMemorySerializer::CMemorySerializer(): iser(this), oser(this)
{
	readPos = 0;
	registerTypes(iser);
	registerTypes(oser);
}
