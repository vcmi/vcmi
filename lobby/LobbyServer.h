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

#include "../lib/network/NetworkDefines.h"
#include "LobbyDefines.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class LobbyDatabase;

class LobbyServer final : public INetworkServerListener
{
	struct AwaitingProxyState
	{
		std::string accountID;
		std::string roomID;
		NetworkConnectionWeakPtr accountConnection;
		NetworkConnectionWeakPtr roomConnection;
	};

	/// list of connected proxies. All messages received from (key) will be redirected to (value) connection
	std::map<NetworkConnectionPtr, NetworkConnectionPtr> activeProxies;

	/// list of half-established proxies from server that are still waiting for client to connect
	std::vector<AwaitingProxyState> awaitingProxies;

	/// list of logged in accounts (vcmiclient's)
	std::map<NetworkConnectionPtr, std::string> activeAccounts;

	/// list of currently logged in game rooms (vcmiserver's)
	std::map<NetworkConnectionPtr, std::string> activeGameRooms;

	std::unique_ptr<LobbyDatabase> database;
	std::unique_ptr<INetworkHandler> networkHandler;
	std::unique_ptr<INetworkServer> networkServer;

	/// RSA private key (PEM) loaded from userDataPath/lobbyPrivateKey.pem at startup
	std::string encryptionPrivateKey;

	/// Set of accepted authentication methods. Defaults to {"classic", "forum"}.
	std::set<std::string> allowedAuthMethods;

	/// Forum hostname used for credential verification (e.g. "forum.vcmi.eu").
	/// Set from EntryPoint.cpp FORUM_HOST constant.
	std::string forumHost;

	/// Returns true if the given auth method ("classic" or "forum") is accepted by server config.
	bool isAuthMethodAllowed(const std::string & method) const;

	/// Decrypt a field that was encrypted by the client with the public key.
	/// Throws on failure (bad key, corrupt ciphertext).
	std::string decryptField(const std::string & value) const;

	/// removes any "weird" symbols from chat message that might break UI
	std::string sanitizeChatMessage(const std::string & inputString) const;

	bool isAccountNameValid(const std::string & accountName) const;

	NetworkConnectionPtr findAccount(const std::string & accountID) const;
	NetworkConnectionPtr findGameRoom(const std::string & gameRoomID) const;

	void onNewConnection(const NetworkConnectionPtr & connection) override;
	void onDisconnected(const NetworkConnectionPtr & connection, const std::string & errorMessage) override;
	void onPacketReceived(const NetworkConnectionPtr & connection, const std::vector<std::byte> & message) override;

	void sendMessage(const NetworkConnectionPtr & target, const JsonNode & json);

	void broadcastActiveAccounts();
	void broadcastActiveGameRooms();

	JsonNode prepareActiveGameRooms();

	/// Attempts to load json from incoming byte stream and validate it
	/// Returns parsed json on success or empty json node on failure
	JsonNode parseAndValidateMessage(const std::vector<std::byte> & message) const;

	void sendChatMessage(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::string & accountID, const std::string & displayName, const std::string & messageText);
	void sendAccountCreated(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & accountCookie, const std::string & clientPublicKey);
	void sendOperationFailed(const NetworkConnectionPtr & target, const std::string & reason);
	void sendServerLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie);
	void sendClientLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie, const std::string & displayName, const std::string & clientPublicKey);
	void sendFullChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::string & channelNameForClient);
	void sendRecentChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName);
	void sendChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::vector<LobbyChatMessage> & history);
	void sendServerCapabilities(const NetworkConnectionPtr & target);
	void sendAccountJoinsRoom(const NetworkConnectionPtr & target, const std::string & accountID);
	void sendJoinRoomSuccess(const NetworkConnectionPtr & target, const std::string & gameRoomID, bool proxyMode);
	void sendInviteReceived(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & gameRoomID);
	void sendMatchesHistory(const NetworkConnectionPtr & target);

	void receiveClientRegister(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveClientLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveServerLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveForumLogin(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveClientProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json);

	void finishClientLogin(const NetworkConnectionPtr & connection, const std::string & accountID, const std::string & accountCookie, const std::string & language, const std::string & version, const std::vector<std::string> & languageRooms, const std::string & clientPublicKey);
	void receiveServerProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json);

	void receiveSendChatMessage(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveRequestChatHistory(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveActivateGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveJoinGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveLeaveGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveChangeRoomDescription(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveGameStarted(const NetworkConnectionPtr & connection, const JsonNode & json);
	void receiveSendInvite(const NetworkConnectionPtr & connection, const JsonNode & json);

public:
	explicit LobbyServer(const boost::filesystem::path & databasePath, const std::set<std::string> & authMethods, const std::string & forumHost);
	~LobbyServer();

	void start(uint16_t port);
	void run();

	LobbyDatabase * getDatabase() const;
	NetworkContext & getNetworkContext();
};
