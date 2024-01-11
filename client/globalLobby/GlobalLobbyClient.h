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

#include "../../lib/network/NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class GlobalLobbyLoginWindow;
class GlobalLobbyWindow;

class GlobalLobbyClient : public INetworkClientListener, boost::noncopyable
{
	std::unique_ptr<INetworkClient> networkClient;

	std::weak_ptr<GlobalLobbyLoginWindow> loginWindow;
	std::weak_ptr<GlobalLobbyWindow> lobbyWindow;
	std::shared_ptr<GlobalLobbyWindow> lobbyWindowLock; // helper strong reference to prevent window destruction on closing

	void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> &) override;

	void sendClientRegister();
	void sendClientLogin();

	void receiveAccountCreated(const JsonNode & json);
	void receiveLoginFailed(const JsonNode & json);
	void receiveLoginSuccess(const JsonNode & json);
	void receiveChatHistory(const JsonNode & json);
	void receiveChatMessage(const JsonNode & json);
	void receiveActiveAccounts(const JsonNode & json);

public:
	explicit GlobalLobbyClient(const std::unique_ptr<INetworkHandler> & handler);
	~GlobalLobbyClient();

	void sendMessage(const JsonNode & data);
	void connect();
	bool isConnected();
	std::shared_ptr<GlobalLobbyLoginWindow> createLoginWindow();
	std::shared_ptr<GlobalLobbyWindow> createLobbyWindow();
};
