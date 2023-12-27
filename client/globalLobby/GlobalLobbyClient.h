/*
 * GlobalLobbyClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/network/NetworkListener.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class GlobalLobbyWindow;

class GlobalLobbyClient : public INetworkClientListener
{
	std::unique_ptr<NetworkClient> networkClient;
	GlobalLobbyWindow * window;

	void onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<NetworkConnection> &) override;
	void onDisconnected(const std::shared_ptr<NetworkConnection> &) override;
	void onTimer() override;

public:
	explicit GlobalLobbyClient(GlobalLobbyWindow * window);
	~GlobalLobbyClient();

	void sendMessage(const JsonNode & data);
	void start(const std::string & host, uint16_t port);
	void run();
	void poll();

};
