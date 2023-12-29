/*
 * LobbyServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "LobbyDefines.h"
#include "../lib/network/NetworkListener.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class LobbyDatabase;

class LobbyServer : public INetworkServerListener
{
	struct AccountState
	{
		std::string accountID;
		std::string displayName;
		std::string version;
		std::string language;
	};
	struct GameRoomState
	{
		std::string roomID;
	};
	struct AwaitingProxyState
	{
		std::string accountID;
		std::string roomID;
		std::weak_ptr<NetworkConnection> accountConnection;
		std::weak_ptr<NetworkConnection> roomConnection;
	};

	/// list of connected proxies. All messages received from (key) will be redirected to (value) connection
	std::map<NetworkConnectionPtr, std::weak_ptr<NetworkConnection>> activeProxies;

	/// list of half-established proxies from server that are still waiting for client to connect
	std::vector<AwaitingProxyState> awaitingProxies;

	/// list of logged in accounts (vcmiclient's)
	std::map<NetworkConnectionPtr, AccountState> activeAccounts;

	/// list of currently logged in game rooms (vcmiserver's)
	std::map<NetworkConnectionPtr, GameRoomState> activeGameRooms;

	std::unique_ptr<LobbyDatabase> database;
	std::unique_ptr<NetworkServer> networkServer;

	std::string sanitizeChatMessage(const std::string & inputString) const;
	bool isAccountNameValid(const std::string & accountName);

	NetworkConnectionPtr findAccount(const std::string & accountID);
	NetworkConnectionPtr findGameRoom(const std::string & gameRoomID);

	void onNewConnection(const NetworkConnectionPtr & connection) override;
	void onDisconnected(const NetworkConnectionPtr & connection) override;
	void onPacketReceived(const NetworkConnectionPtr & connection, const std::vector<uint8_t> & message) override;
	void onTimer() override;

	void sendMessage(const NetworkConnectionPtr & target, const JsonNode & json);

	void broadcastActiveAccounts();
	void broadcastActiveGameRooms();

	void sendChatMessage(const NetworkConnectionPtr & target, const std::string & roomMode, const std::string & roomName, const std::string & senderName, const std::string & messageText);
	void sendAccountCreated(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & accountCookie);
	void sendLoginFailed(const NetworkConnectionPtr & target, const std::string & reason);
	void sendLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie);
	void sendChatHistory(const NetworkConnectionPtr & target, const std::vector<LobbyChatMessage> &);
	void sendAccountJoinsRoom(const NetworkConnectionPtr & target, const std::string & accountID);
	void sendJoinRoomSuccess(const NetworkConnectionPtr & target, const std::string & gameRoomID);
	void sendInviteReceived(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & gameRoomID);

	void receiveClientRegister(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveClientLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveServerLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveClientProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveServerProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json);

	void receiveSendChatMessage(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveOpenGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveJoinGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveLeaveGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveSendInvite(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveDeclineInvite(const NetworkConnectionPtr & connection, const JsonNode & json);

public:
	explicit LobbyServer(const boost::filesystem::path & databasePath);
	~LobbyServer();

	void start(uint16_t port);
	void run();
};
