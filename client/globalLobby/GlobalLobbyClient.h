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

#include "GlobalLobbyDefines.h"
#include "../../lib/network/NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class GlobalLobbyLoginWindow;
class GlobalLobbyWindow;

class GlobalLobbyClient final : public INetworkClientListener, boost::noncopyable
{
	std::vector<GlobalLobbyAccount> activeAccounts;
	std::vector<GlobalLobbyRoom> activeRooms;

	std::shared_ptr<INetworkConnection> networkConnection;

	std::weak_ptr<GlobalLobbyLoginWindow> loginWindow;
	std::weak_ptr<GlobalLobbyWindow> lobbyWindow;
	std::shared_ptr<GlobalLobbyWindow> lobbyWindowLock; // helper strong reference to prevent window destruction on closing

	void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> &, const std::string & errorMessage) override;

	void receiveAccountCreated(const JsonNode & json);
	void receiveOperationFailed(const JsonNode & json);
	void receiveClientLoginSuccess(const JsonNode & json);
	void receiveChatHistory(const JsonNode & json);
	void receiveChatMessage(const JsonNode & json);
	void receiveActiveAccounts(const JsonNode & json);
	void receiveActiveGameRooms(const JsonNode & json);
	void receiveJoinRoomSuccess(const JsonNode & json);
	void receiveInviteReceived(const JsonNode & json);

	std::shared_ptr<GlobalLobbyLoginWindow> createLoginWindow();
	std::shared_ptr<GlobalLobbyWindow> createLobbyWindow();

public:
	explicit GlobalLobbyClient();
	~GlobalLobbyClient();

	const std::vector<GlobalLobbyAccount> & getActiveAccounts() const;
	const std::vector<GlobalLobbyRoom> & getActiveRooms() const;

	/// Activate interface and pushes lobby UI as top window
	void activateInterface();
	void sendMessage(const JsonNode & data);
	void sendClientRegister(const std::string & accountName);
	void sendClientLogin();
	void sendOpenRoom(const std::string & mode, int playerLimit);

	void sendProxyConnectionLogin(const NetworkConnectionPtr & netConnection);

	void connect();
	bool isConnected() const;
};
