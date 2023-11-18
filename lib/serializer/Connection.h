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

#include "CSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class BinaryDeserializer;
class BinarySerializer;
struct CPack;
struct ConnectionBuffers;
class NetworkConnection;

/// Main class for network communication
/// Allows establishing connection and bidirectional read-write
class DLL_LINKAGE CConnection : public IBinaryReader, public IBinaryWriter, public std::enable_shared_from_this<CConnection>
{
	/// Non-owning pointer to underlying connection
	std::weak_ptr<NetworkConnection> networkConnection;

//	void init();
//	void reportState(vstd::CLoggerBase * out) override;
//
	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
//	void flushBuffers();
//
//	bool enableBufferedWrite;
//	bool enableBufferedRead;
//	std::unique_ptr<ConnectionBuffers> connectionBuffers;
//
	std::unique_ptr<BinaryDeserializer> iser;
	std::unique_ptr<BinarySerializer> oser;
//
//	std::string contactUuid;
//	std::string name; //who uses this connection

public:
	std::string uuid;
	int connectionID;

	CConnection(std::weak_ptr<NetworkConnection> networkConnection);
//	CConnection(const std::string & host, ui16 port, std::string Name, std::string UUID);
//	CConnection(const std::shared_ptr<TAcceptor> & acceptor, const std::shared_ptr<boost::asio::io_service> & Io_service, std::string Name, std::string UUID);
//	CConnection(std::shared_ptr<TSocket> Socket, std::string Name, std::string UUID); //use immediately after accepting connection into socket
//	virtual ~CConnection();

//	void close();
//	bool isOpen() const;
//
//	CPack * retrievePack();
	void sendPack(const CPack * pack);

	CPack * retrievePack(const std::vector<uint8_t> & data);
//	std::vector<uint8_t> serializePack(const CPack * pack);
//
	void disableStackSendingByID();
//	void enableStackSendingByID();
//	void disableSmartPointerSerialization();
//	void enableSmartPointerSerialization();
//	void disableSmartVectorMemberSerialization();
//	void enableSmartVectorMemberSerializatoin();
//
	void enterLobbyConnectionMode();
	void enterGameplayConnectionMode(CGameState * gs);
//
//	std::string toString() const;
//
//	template<class T>
//	CConnection & operator>>(T &t)
//	{
//		iser & t;
//		return * this;
//	}
//
//	template<class T>
//	CConnection & operator<<(const T &t)
//	{
//		oser & t;
//		return * this;
//	}
};

VCMI_LIB_NAMESPACE_END
