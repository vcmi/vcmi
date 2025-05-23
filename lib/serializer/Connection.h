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

enum class ESerializationVersion : int32_t;

VCMI_LIB_NAMESPACE_BEGIN

class BinaryDeserializer;
class BinarySerializer;
struct CPack;
class INetworkConnection;
class ConnectionPackReader;
class ConnectionPackWriter;
class CGameState;
class IGameInfoCallback;

/// Wrapper class for game connection
/// Handles serialization and deserialization of data received from network
class DLL_LINKAGE CConnection : boost::noncopyable
{
	/// Non-owning pointer to underlying connection
	std::weak_ptr<INetworkConnection> networkConnection;

	std::unique_ptr<ConnectionPackReader> packReader;
	std::unique_ptr<ConnectionPackWriter> packWriter;
	std::unique_ptr<BinaryDeserializer> deserializer;
	std::unique_ptr<BinarySerializer> serializer;

	std::mutex writeMutex;

public:
	bool isMyConnection(const std::shared_ptr<INetworkConnection> & otherConnection) const;
	std::shared_ptr<INetworkConnection> getConnection();

	std::string uuid;
	int connectionID;

	explicit CConnection(std::weak_ptr<INetworkConnection> networkConnection);
	~CConnection();

	void sendPack(const CPack & pack);
	std::unique_ptr<CPack> retrievePack(const std::vector<std::byte> & data);

	void enterLobbyConnectionMode();
	void setCallback(IGameInfoCallback & cb);
	void setSerializationVersion(ESerializationVersion version);
};

VCMI_LIB_NAMESPACE_END
