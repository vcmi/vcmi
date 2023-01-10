/*
 * Connection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <enet/enet.h>

#include "BinaryDeserializer.h"
#include "BinarySerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CPack;
struct ConnectionBuffers;

/// Main class for network communication
/// Allows establishing connection and bidirectional read-write
class DLL_LINKAGE CConnection
	: public IBinaryReader, public IBinaryWriter, public std::enable_shared_from_this<CConnection>
{
	void reportState(vstd::CLoggerBase * out) override;

	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
	
	std::list<ENetPacket*> packets;
	int channel;
	ENetPeer * peer = nullptr;
	ENetHost * client = nullptr;

	BinaryDeserializer iser;
	BinarySerializer oser;
	
	bool connected;
	boost::mutex mutexRead;
	boost::mutex mutexWrite;
	
public:

	bool myEndianess, contactEndianess; //true if little endian, if endianness is different we'll have to revert received multi-byte vars
	std::string contactUuid;
	std::string name; //who uses this connection
	std::string uuid;

	int connectionID;
	std::shared_ptr<boost::thread> handler;

	CConnection(ENetHost * client, ENetPeer * peer, std::string Name, std::string UUID);
	CConnection(ENetHost * client, std::string host, ui16 port, std::string Name, std::string UUID);
	void init(); //must be called from outside after connection message received
	void dispatch(ENetPacket * packet);
	const ENetPeer * getPeer() const;

	void close();
	bool isOpen() const;
	template<class T>
	CConnection &operator&(const T&);
	virtual ~CConnection();

	CPack * retrievePack();
	void sendPack(const CPack * pack);

	void disableStackSendingByID();
	void enableStackSendingByID();
	void disableSmartPointerSerialization();
	void enableSmartPointerSerialization();
	void disableSmartVectorMemberSerialization();
	void enableSmartVectorMemberSerializatoin();

	void enterLobbyConnectionMode();
	void enterGameplayConnectionMode(CGameState * gs);

	std::string toString() const;

	template<class T>
	CConnection & operator>>(T &t)
	{
		iser & t;
		return * this;
	}

	template<class T>
	CConnection & operator<<(const T &t)
	{
		oser & t;
		return * this;
	}
};

VCMI_LIB_NAMESPACE_END
