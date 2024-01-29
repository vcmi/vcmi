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

	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) override;

	void sendMessage(const NetworkConnectionPtr & target, const JsonNode & data);

	void receiveLoginFailed(const JsonNode & json);
	void receiveLoginSuccess(const JsonNode & json);
	void receiveAccountJoinsRoom(const JsonNode & json);

	void establishNewConnection();
public:
	GlobalLobbyProcessor(CVCMIServer & owner);
};
