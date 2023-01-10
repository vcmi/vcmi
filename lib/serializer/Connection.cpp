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

CConnection::CConnection(ENetHost * _client, ENetPeer * _peer, std::string Name, std::string UUID)
	: client(_client), peer(_peer), iser(this), oser(this), name(Name), uuid(UUID), connectionID(0), connected(false), channel(1)
{	
	//init();
}

CConnection::CConnection(ENetHost * _client, std::string host, ui16 port, std::string Name, std::string UUID)
	: client(_client), iser(this), oser(this), name(Name), uuid(UUID), connectionID(0), connected(false), channel(0)
{
	ENetAddress address;
	enet_address_set_host(&address, host.c_str());
	address.port = port;
	
	peer = enet_host_connect(client, &address, 2, 0);
	if(peer == NULL)
	{
		throw std::runtime_error("Can't establish connection :(");
	}
}

void CConnection::dispatch(ENetPacket * packet)
{
	boost::unique_lock<boost::mutex> lock(mutexRead);
	packets.push_back(packet);
}

const ENetPeer * CConnection::getPeer() const
{
	return peer;
}

int CConnection::write(const void * data, unsigned size)
{
	if(size == 0)
		return 0;
	boost::unique_lock<boost::mutex> lock(mutexWrite);
	ENetPacket * packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, channel, packet);
	return size;
}
int CConnection::read(void * data, unsigned size)
{
	const int timout = 1000;
	if(size == 0)
		return 0;
	for(int i = 0; packets.empty() && (i < timout || connected); ++i)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	}
	boost::unique_lock<boost::mutex> lock(mutexRead);
	auto * packet = packets.front();
	packets.pop_front();
	
	if(packet->dataLength > 0)
		memcpy(data, packet->data, packet->dataLength);
	int ret = packet->dataLength;
	assert(ret == size);
	enet_packet_destroy(packet);

	return ret;
}

CConnection::~CConnection()
{
	if(handler)
		handler->join();
	
	for(auto * packet : packets)
		enet_packet_destroy(packet);

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

void CConnection::close()
{
	boost::unique_lock<boost::mutex> lock(mutexWrite);
	enet_peer_disconnect(peer, 0);
	enet_peer_reset(peer);
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
