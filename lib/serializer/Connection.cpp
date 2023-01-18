/*
 * Connection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Connection.h"

#include "../registerTypes/RegisterTypes.h"
#include "../mapping/CMap.h"
#include "../CGameState.h"


VCMI_LIB_NAMESPACE_BEGIN


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
	enableSmartPointerSerialization();
	disableStackSendingByID();
	registerTypes(iser);
	registerTypes(oser);
#ifdef LIL_ENDIAN
	myEndianess = true;
#else
	myEndianess = false;
#endif
	std::string pom;

	//we got connection
	oser & std::string("Aiya!\n") & name & uuid & myEndianess; //identify ourselves
	
	iser & pom & pom & contactUuid & contactEndianess;
	logNetwork->info("Established connection with %s. UUID: %s", pom, contactUuid);

	connected = true;
	iser.fileVersion = SERIALIZATION_VERSION;
}

CConnection::CConnection(std::shared_ptr<EnetConnection> _c, std::string Name, std::string UUID)
	: enetConnection(_c), iser(this), oser(this), name(Name), uuid(UUID), connectionID(0), connected(false)
{	
	init();
}

int CConnection::write(const void * data, unsigned size)
{
	if(!enetConnection->isOpen())
		throw std::logic_error("Write in closed connection");
	
	enetConnection->write(data, size);
	return size;
}

int CConnection::read(void * data, unsigned size)
{
	if(!enetConnection->isOpen())
		throw std::logic_error("Read from closed connection");
	
	enetConnection->read(data, size);
	return size;
}

CConnection::~CConnection()
{
	if(handler)
		handler->join();

	close();
}

template<class T>
CConnection & CConnection::operator&(const T &t) {
//	throw std::exception();
//XXX this is temporaly ? solution to fix gcc (4.3.3, other?) compilation
//    problem for more details contact t0@czlug.icis.pcz.pl or impono@gmail.com
//    do not remove this exception it shoudnt be called
	return *this;
}

const std::shared_ptr<EnetConnection> CConnection::getEnetConnection() const
{
	return enetConnection;
}

void CConnection::close()
{
	connected = false;
	if(enetConnection->isOpen())
		enetConnection->close();
	else
		enetConnection->kill();
}

bool CConnection::isOpen() const
{
	return connected;
}

void CConnection::reportState(vstd::CLoggerBase * out)
{
	out->debug("CConnection");
	/*if(socket && socket->is_open())
	{
		out->debug("\tWe have an open and valid socket");
		out->debug("\t %d bytes awaiting", socket->available());
	}*/
}

CPack * CConnection::retrievePack()
{

	CPack * pack = nullptr;
	iser & pack;
	logNetwork->trace("Received CPack of type %s", (pack ? typeid(*pack).name() : "nullptr"));
	if(pack == nullptr)
	{
		logNetwork->error("Received a nullptr CPack! You should check whether client and server ABI matches.");
	}
	else
	{
		pack->c = this->shared_from_this();
	}


	return pack;
}

void CConnection::sendPack(const CPack * pack)
{
	logNetwork->trace("Sending a pack of type %s", typeid(*pack).name());

	oser & pack;
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

void CConnection::enableSmartPointerSerialization()
{
	iser.smartPointerSerialization = oser.smartPointerSerialization = true;
}

void CConnection::enterLobbyConnectionMode()
{
	iser.loadedPointers.clear();
	oser.savedPointers.clear();
	disableSmartVectorMemberSerialization();
	disableSmartPointerSerialization();
}

void CConnection::enterGameplayConnectionMode(CGameState * gs)
{
	enableStackSendingByID();
	disableSmartPointerSerialization();
	addStdVecItems(gs);
}

void CConnection::disableSmartVectorMemberSerialization()
{
	CSerializer::smartVectorMembersSerialization = false;
}

void CConnection::enableSmartVectorMemberSerializatoin()
{
	CSerializer::smartVectorMembersSerialization = true;
}

std::string CConnection::toString() const
{
	boost::format fmt("Connection with %s (ID: %d UUID: %s)");
	fmt % name % connectionID % uuid;
	return fmt.str();
}

VCMI_LIB_NAMESPACE_END
