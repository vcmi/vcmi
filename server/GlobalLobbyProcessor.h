/*
 * GlobalLobbyProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/network/NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CVCMIServer;

class GlobalLobbyProcessor : public INetworkClientListener
{
	CVCMIServer & owner;

	NetworkConnectionPtr controlConnection;
	std::map<std::string, NetworkConnectionPtr> proxyConnections;

	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) override;

	void receiveOperationFailed(const JsonNode & json);
	void receiveServerLoginSuccess(const JsonNode & json);
	void receiveAccountJoinsRoom(const JsonNode & json);

	void establishNewConnection();
	void sendMessage(const NetworkConnectionPtr & targetConnection, const JsonNode & payload);

	const std::string & getHostAccountID() const;
	const std::string & getHostAccountCookie() const;
	const std::string & getHostAccountDisplayName() const;
public:
	void sendChangeRoomDescription(const std::string & description);
	void sendGameStarted();

	explicit GlobalLobbyProcessor(CVCMIServer & owner);
};
