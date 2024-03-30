/*
 * LobbyServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LobbyServer.h"

#include "LobbyDatabase.h"

#include "../lib/Languages.h"
#include "../lib/TextOperations.h"
#include "../lib/json/JsonFormatException.h"
#include "../lib/json/JsonNode.h"
#include "../lib/json/JsonUtils.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

bool LobbyServer::isAccountNameValid(const std::string & accountName) const
{
	// Arbitrary limit on account name length.
	// Can be extended if there are no issues with UI space
	if(accountName.size() < 4)
		return false;

	if(accountName.size() > 20)
		return false;

	// For now permit only latin alphabet and numbers
	// Can be extended, but makes sure that such symbols will be present in all H3 fonts
	for(const auto & c : accountName)
		if(!std::isalnum(c))
			return false;

	return true;
}

std::string LobbyServer::sanitizeChatMessage(const std::string & inputString) const
{
	static const std::string blacklist = "{}";
	std::string sanitized;

	for(const auto & ch : inputString)
	{
		// Remove all control characters
		if (ch >= '\0' && ch < ' ')
			continue;

		// Remove blacklisted characters such as brackets that are used for text formatting
		if (blacklist.find(ch) != std::string::npos)
			continue;

		sanitized += ch;
	}

	return boost::trim_copy(sanitized);
}

NetworkConnectionPtr LobbyServer::findAccount(const std::string & accountID) const
{
	for(const auto & account : activeAccounts)
		if(account.second == accountID)
			return account.first;

	return nullptr;
}

NetworkConnectionPtr LobbyServer::findGameRoom(const std::string & gameRoomID) const
{
	for(const auto & account : activeGameRooms)
		if(account.second == gameRoomID)
			return account.first;

	return nullptr;
}

void LobbyServer::sendMessage(const NetworkConnectionPtr & target, const JsonNode & json)
{
	logGlobal->info("Sending message of type %s", json["type"].String());

	assert(JsonUtils::validate(json, "vcmi:lobbyProtocol/" + json["type"].String(), json["type"].String() + " pack"));
	target->sendPacket(json.toBytes());
}

void LobbyServer::sendAccountCreated(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & accountCookie)
{
	JsonNode reply;
	reply["type"].String() = "accountCreated";
	reply["accountID"].String() = accountID;
	reply["accountCookie"].String() = accountCookie;
	sendMessage(target, reply);
}

void LobbyServer::sendInviteReceived(const NetworkConnectionPtr & target, const std::string & accountID, const std::string & gameRoomID)
{
	JsonNode reply;
	reply["type"].String() = "inviteReceived";
	reply["accountID"].String() = accountID;
	reply["gameRoomID"].String() = gameRoomID;
	sendMessage(target, reply);
}

void LobbyServer::sendOperationFailed(const NetworkConnectionPtr & target, const std::string & reason)
{
	JsonNode reply;
	reply["type"].String() = "operationFailed";
	reply["reason"].String() = reason;
	sendMessage(target, reply);
}

void LobbyServer::sendClientLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie, const std::string & displayName)
{
	JsonNode reply;
	reply["type"].String() = "clientLoginSuccess";
	reply["accountCookie"].String() = accountCookie;
	reply["displayName"].String() = displayName;
	sendMessage(target, reply);
}

void LobbyServer::sendServerLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie)
{
	JsonNode reply;
	reply["type"].String() = "serverLoginSuccess";
	reply["accountCookie"].String() = accountCookie;
	sendMessage(target, reply);
}

void LobbyServer::sendFullChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::string & channelNameForClient)
{
	sendChatHistory(target, channelType, channelNameForClient, database->getFullMessageHistory(channelType, channelName));
}

void LobbyServer::sendRecentChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName)
{
	sendChatHistory(target, channelType, channelName, database->getRecentMessageHistory(channelType, channelName));
}

void LobbyServer::sendChatHistory(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::vector<LobbyChatMessage> & history)
{
	JsonNode reply;
	reply["type"].String() = "chatHistory";
	reply["channelType"].String() = channelType;
	reply["channelName"].String() = channelName;
	reply["messages"].Vector(); // force creation of empty vector

	for(const auto & message : boost::adaptors::reverse(history))
	{
		JsonNode jsonEntry;

		jsonEntry["accountID"].String() = message.accountID;
		jsonEntry["displayName"].String() = message.displayName;
		jsonEntry["messageText"].String() = message.messageText;
		jsonEntry["ageSeconds"].Integer() = message.age.count();

		reply["messages"].Vector().push_back(jsonEntry);
	}

	sendMessage(target, reply);
}

void LobbyServer::broadcastActiveAccounts()
{
	auto activeAccountsStats = database->getActiveAccounts();

	JsonNode reply;
	reply["type"].String() = "activeAccounts";
	reply["accounts"].Vector(); // force creation of empty vector

	for(const auto & account : activeAccountsStats)
	{
		JsonNode jsonEntry;
		jsonEntry["accountID"].String() = account.accountID;
		jsonEntry["displayName"].String() = account.displayName;
		jsonEntry["status"].String() = "In Lobby"; // TODO: in room status, in match status, offline status(?)
		reply["accounts"].Vector().push_back(jsonEntry);
	}

	for(const auto & connection : activeAccounts)
		sendMessage(connection.first, reply);
}

static JsonNode loadLobbyAccountToJson(const LobbyAccount & account)
{
	JsonNode jsonEntry;
	jsonEntry["accountID"].String() = account.accountID;
	jsonEntry["displayName"].String() = account.displayName;
	return jsonEntry;
}

static JsonNode loadLobbyGameRoomToJson(const LobbyGameRoom & gameRoom)
{
	static constexpr std::array LOBBY_ROOM_STATE_NAMES = {
		"idle",
		"public",
		"private",
		"busy",
		"cancelled",
		"closed"
	};

	JsonNode jsonEntry;
	jsonEntry["gameRoomID"].String() = gameRoom.roomID;
	jsonEntry["hostAccountID"].String() = gameRoom.hostAccountID;
	jsonEntry["hostAccountDisplayName"].String() = gameRoom.hostAccountDisplayName;
	jsonEntry["description"].String() = gameRoom.description;
	jsonEntry["status"].String() = LOBBY_ROOM_STATE_NAMES[vstd::to_underlying(gameRoom.roomState)];
	jsonEntry["playerLimit"].Integer() = gameRoom.playerLimit;
	jsonEntry["ageSeconds"].Integer() = gameRoom.age.count();

	for(const auto & account : gameRoom.participants)
		jsonEntry["participants"].Vector().push_back(loadLobbyAccountToJson(account));

	return jsonEntry;
}

void LobbyServer::sendMatchesHistory(const NetworkConnectionPtr & target)
{
	std::string accountID = activeAccounts.at(target);

	auto matchesHistory = database->getAccountGameHistory(accountID);
	JsonNode reply;
	reply["type"].String() = "matchesHistory";
	reply["matchesHistory"].Vector(); // force creation of empty vector

	for(const auto & gameRoom : matchesHistory)
		reply["matchesHistory"].Vector().push_back(loadLobbyGameRoomToJson(gameRoom));

	sendMessage(target, reply);
}

JsonNode LobbyServer::prepareActiveGameRooms()
{
	auto activeGameRoomStats = database->getActiveGameRooms();
	JsonNode reply;
	reply["type"].String() = "activeGameRooms";
	reply["gameRooms"].Vector(); // force creation of empty vector

	for(const auto & gameRoom : activeGameRoomStats)
		reply["gameRooms"].Vector().push_back(loadLobbyGameRoomToJson(gameRoom));

	return reply;
}

void LobbyServer::broadcastActiveGameRooms()
{
	auto reply = prepareActiveGameRooms();

	for(const auto & connection : activeAccounts)
		sendMessage(connection.first, reply);
}

void LobbyServer::sendAccountJoinsRoom(const NetworkConnectionPtr & target, const std::string & accountID)
{
	JsonNode reply;
	reply["type"].String() = "accountJoinsRoom";
	reply["accountID"].String() = accountID;
	sendMessage(target, reply);
}

void LobbyServer::sendJoinRoomSuccess(const NetworkConnectionPtr & target, const std::string & gameRoomID, bool proxyMode)
{
	JsonNode reply;
	reply["type"].String() = "joinRoomSuccess";
	reply["gameRoomID"].String() = gameRoomID;
	reply["proxyMode"].Bool() = proxyMode;
	sendMessage(target, reply);
}

void LobbyServer::sendChatMessage(const NetworkConnectionPtr & target, const std::string & channelType, const std::string & channelName, const std::string & accountID, const std::string & displayName, const std::string & messageText)
{
	JsonNode reply;
	reply["type"].String() = "chatMessage";
	reply["messageText"].String() = messageText;
	reply["accountID"].String() = accountID;
	reply["displayName"].String() = displayName;
	reply["channelType"].String() = channelType;
	reply["channelName"].String() = channelName;

	sendMessage(target, reply);
}

void LobbyServer::onNewConnection(const NetworkConnectionPtr & connection)
{
	// no-op - waiting for incoming data
}

void LobbyServer::onDisconnected(const NetworkConnectionPtr & connection, const std::string & errorMessage)
{
	if(activeAccounts.count(connection))
	{
		logGlobal->info("Account %s disconnecting. Accounts online: %d", activeAccounts.at(connection), activeAccounts.size() - 1);
		database->setAccountOnline(activeAccounts.at(connection), false);
		activeAccounts.erase(connection);
	}

	if(activeGameRooms.count(connection))
	{
		std::string gameRoomID = activeGameRooms.at(connection);
		logGlobal->info("Game room %s disconnecting. Rooms online: %d", gameRoomID, activeGameRooms.size() - 1);

		if (database->getGameRoomStatus(gameRoomID) == LobbyRoomState::BUSY)
		{
			database->setGameRoomStatus(gameRoomID, LobbyRoomState::CLOSED);
			for(const auto & accountConnection : activeAccounts)
				if (database->isPlayerInGameRoom(accountConnection.second, gameRoomID))
					sendMatchesHistory(accountConnection.first);
		}
		else
			database->setGameRoomStatus(gameRoomID, LobbyRoomState::CANCELLED);

		activeGameRooms.erase(connection);
	}

	if(activeProxies.count(connection))
	{
		const auto & otherConnection = activeProxies.at(connection);

		if (otherConnection)
			otherConnection->close();

		activeProxies.erase(connection);
		activeProxies.erase(otherConnection);
	}

	broadcastActiveAccounts();
	broadcastActiveGameRooms();
}

JsonNode LobbyServer::parseAndValidateMessage(const std::vector<std::byte> & message) const
{
	JsonParsingSettings parserSettings;
	parserSettings.mode = JsonParsingSettings::JsonFormatMode::JSON;
	parserSettings.maxDepth = 2;
	parserSettings.strict = true;

	JsonNode json;
	try
	{
		JsonNode jsonTemp(message.data(), message.size());
		json = std::move(jsonTemp);
	}
	catch (const JsonFormatException & e)
	{
		logGlobal->info(std::string("Json parsing error encountered: ") + e.what());
		return JsonNode();
	}

	std::string messageType = json["type"].String();

	if (messageType.empty())
	{
		logGlobal->info("Json parsing error encountered: Message type not set!");
		return JsonNode();
	}

	std::string schemaName = "vcmi:lobbyProtocol/" + messageType;

	if (!JsonUtils::validate(json, schemaName, messageType + " pack"))
	{
		logGlobal->info("Json validation error encountered!");
		assert(0);
		return JsonNode();
	}

	return json;
}

void LobbyServer::onPacketReceived(const NetworkConnectionPtr & connection, const std::vector<std::byte> & message)
{
	// proxy connection - no processing, only redirect
	if(activeProxies.count(connection))
	{
		auto lockedPtr = activeProxies.at(connection);
		if(lockedPtr)
			return lockedPtr->sendPacket(message);

		logGlobal->info("Received unexpected message for inactive proxy!");
	}

	JsonNode json = parseAndValidateMessage(message);

	std::string messageType = json["type"].String();

	// communication messages from vcmiclient
	if(activeAccounts.count(connection))
	{
		std::string accountName = activeAccounts.at(connection);
		logGlobal->info("%s: Received message of type %s", accountName, messageType);

		if(messageType == "sendChatMessage")
			return receiveSendChatMessage(connection, json);

		if(messageType == "requestChatHistory")
			return receiveRequestChatHistory(connection, json);

		if(messageType == "activateGameRoom")
			return receiveActivateGameRoom(connection, json);

		if(messageType == "joinGameRoom")
			return receiveJoinGameRoom(connection, json);

		if(messageType == "sendInvite")
			return receiveSendInvite(connection, json);

		logGlobal->warn("%s: Unknown message type: %s", accountName, messageType);
		return;
	}

	// communication messages from vcmiserver
	if(activeGameRooms.count(connection))
	{
		std::string roomName = activeGameRooms.at(connection);
		logGlobal->info("%s: Received message of type %s", roomName, messageType);

		if(messageType == "changeRoomDescription")
			return receiveChangeRoomDescription(connection, json);

		if(messageType == "gameStarted")
			return receiveGameStarted(connection, json);

		if(messageType == "leaveGameRoom")
			return receiveLeaveGameRoom(connection, json);

		logGlobal->warn("%s: Unknown message type: %s", roomName, messageType);
		return;
	}

	logGlobal->info("(unauthorised): Received message of type %s", messageType);

	// unauthorized connections - permit only login or register attempts
	if(messageType == "clientLogin")
		return receiveClientLogin(connection, json);

	if(messageType == "clientRegister")
		return receiveClientRegister(connection, json);

	if(messageType == "serverLogin")
		return receiveServerLogin(connection, json);

	if(messageType == "clientProxyLogin")
		return receiveClientProxyLogin(connection, json);

	if(messageType == "serverProxyLogin")
		return receiveServerProxyLogin(connection, json);

	connection->close();
	logGlobal->info("(unauthorised): Unknown message type %s", messageType);
}

void LobbyServer::receiveRequestChatHistory(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = activeAccounts[connection];
	std::string channelType = json["channelType"].String();
	std::string channelName = json["channelName"].String();

	if (channelType == "global")
	{
		// can only be sent on connection, initiated by server
		sendOperationFailed(connection, "Operation not supported!");
	}

	if (channelType == "match")
	{
		if (!database->isPlayerInGameRoom(accountID, channelName))
			return sendOperationFailed(connection, "Can not access room you are not part of!");

		sendFullChatHistory(connection, channelType, channelName, channelName);
	}

	if (channelType == "player")
	{
		if (!database->isAccountIDExists(channelName))
			return sendOperationFailed(connection, "Such player does not exists!");

		// room ID for private messages is actually <player 1 ID>_<player 2 ID>, with player ID's sorted alphabetically (to generate unique room ID)
		std::string roomID = std::min(accountID, channelName) + "_" + std::max(accountID, channelName);
		sendFullChatHistory(connection, channelType, roomID, channelName);
	}
}

void LobbyServer::receiveSendChatMessage(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string senderAccountID = activeAccounts[connection];
	std::string messageText = json["messageText"].String();
	std::string channelType = json["channelType"].String();
	std::string channelName = json["channelName"].String();
	std::string displayName = database->getAccountDisplayName(senderAccountID);

	if(!TextOperations::isValidUnicodeString(messageText))
		return sendOperationFailed(connection, "String contains invalid characters!");

	std::string messageTextClean = sanitizeChatMessage(messageText);
	if(messageTextClean.empty())
		return sendOperationFailed(connection, "No printable characters in sent message!");

	if (channelType == "global")
	{
		try
		{
			Languages::getLanguageOptions(channelName);
		}
		catch (const std::out_of_range &)
		{
			return sendOperationFailed(connection, "Unknown language!");
		}
		database->insertChatMessage(senderAccountID, channelType, channelName, messageText);

		for(const auto & otherConnection : activeAccounts)
			sendChatMessage(otherConnection.first, channelType, channelName, senderAccountID, displayName, messageText);
	}

	if (channelType == "match")
	{
		if (!database->isPlayerInGameRoom(senderAccountID, channelName))
			return sendOperationFailed(connection, "Can not access room you are not part of!");

		database->insertChatMessage(senderAccountID, channelType, channelName, messageText);

		LobbyRoomState roomStatus = database->getGameRoomStatus(channelName);

		// Broadcast chat message only if it being sent to already closed match
		// Othervice it will be handled by match server
		if (roomStatus == LobbyRoomState::CLOSED)
		{
			for(const auto & otherConnection : activeAccounts)
			{
				if (database->isPlayerInGameRoom(otherConnection.second, channelName))
					sendChatMessage(otherConnection.first, channelType, channelName, senderAccountID, displayName, messageText);
			}
		}
	}

	if (channelType == "player")
	{
		const std::string & receiverAccountID = channelName;
		std::string roomID = std::min(senderAccountID, receiverAccountID) + "_" + std::max(senderAccountID, receiverAccountID);

		if (!database->isAccountIDExists(receiverAccountID))
			return sendOperationFailed(connection, "Such player does not exists!");

		database->insertChatMessage(senderAccountID, channelType, roomID, messageText);

		sendChatMessage(connection, channelType, receiverAccountID, senderAccountID, displayName, messageText);
		if (senderAccountID != receiverAccountID)
		{
			for(const auto & otherConnection : activeAccounts)
				if (otherConnection.second == receiverAccountID)
					sendChatMessage(otherConnection.first, channelType, senderAccountID, senderAccountID, displayName, messageText);
		}
	}
}

void LobbyServer::receiveClientRegister(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string displayName = json["displayName"].String();
	std::string language = json["language"].String();

	if(!isAccountNameValid(displayName))
		return sendOperationFailed(connection, "Illegal account name");

	if(database->isAccountNameExists(displayName))
		return sendOperationFailed(connection, "Account name already in use");

	std::string accountCookie = boost::uuids::to_string(boost::uuids::random_generator()());
	std::string accountID = boost::uuids::to_string(boost::uuids::random_generator()());

	database->insertAccount(accountID, displayName);
	database->insertAccessCookie(accountID, accountCookie);

	sendAccountCreated(connection, accountID, accountCookie);
}

void LobbyServer::receiveClientLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = json["accountID"].String();
	std::string accountCookie = json["accountCookie"].String();
	std::string language = json["language"].String();
	std::string version = json["version"].String();

	if(!database->isAccountIDExists(accountID))
		return sendOperationFailed(connection, "Account not found");

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie);

	if(clientCookieStatus == LobbyCookieStatus::INVALID)
		return sendOperationFailed(connection, "Authentification failure");

	database->updateAccountLoginTime(accountID);
	database->setAccountOnline(accountID, true);

	std::string displayName = database->getAccountDisplayName(accountID);

	activeAccounts[connection] = accountID;

	logGlobal->info("%s: Logged in as %s", accountID, displayName);
	sendClientLoginSuccess(connection, accountCookie, displayName);
	sendRecentChatHistory(connection, "global", "english");
	if (language != "english")
		sendRecentChatHistory(connection, "global", language);

	// send active game rooms list to new account
	// and update acount list to everybody else including new account
	broadcastActiveAccounts();
	sendMessage(connection, prepareActiveGameRooms());
	sendMatchesHistory(connection);
}

void LobbyServer::receiveServerLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = json["accountID"].String();
	std::string accountCookie = json["accountCookie"].String();
	std::string version = json["version"].String();

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie);

	if(clientCookieStatus == LobbyCookieStatus::INVALID)
	{
		sendOperationFailed(connection, "Invalid credentials");
	}
	else
	{
		database->insertGameRoom(gameRoomID, accountID);
		activeGameRooms[connection] = gameRoomID;
		sendServerLoginSuccess(connection, accountCookie);
		broadcastActiveGameRooms();
	}
}

void LobbyServer::receiveClientProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = json["accountID"].String();
	std::string accountCookie = json["accountCookie"].String();

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie);

	if(clientCookieStatus != LobbyCookieStatus::INVALID)
	{
		for(auto & proxyEntry : awaitingProxies)
		{
			if(proxyEntry.accountID != accountID)
				continue;
			if(proxyEntry.roomID != gameRoomID)
				continue;

			proxyEntry.accountConnection = connection;

			auto gameRoomConnection = proxyEntry.roomConnection.lock();

			if(gameRoomConnection)
			{
				activeProxies[gameRoomConnection] = connection;
				activeProxies[connection] = gameRoomConnection;
			}
			return;
		}
	}

	sendOperationFailed(connection, "Invalid credentials");
	connection->close();
}

void LobbyServer::receiveServerProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string guestAccountID = json["guestAccountID"].String();
	std::string accountCookie = json["accountCookie"].String();

	// FIXME: find host account ID and validate his cookie
	//auto clientCookieStatus = database->getAccountCookieStatus(hostAccountID, accountCookie, accountCookieLifetime);

	//if(clientCookieStatus != LobbyCookieStatus::INVALID)
	{
		NetworkConnectionPtr targetAccount = findAccount(guestAccountID);

		if(targetAccount == nullptr)
		{
			sendOperationFailed(connection, "Invalid credentials");
			return; // unknown / disconnected account
		}

		sendJoinRoomSuccess(targetAccount, gameRoomID, true);

		AwaitingProxyState proxy;
		proxy.accountID = guestAccountID;
		proxy.roomID = gameRoomID;
		proxy.roomConnection = connection;
		awaitingProxies.push_back(proxy);
		return;
	}

	//connection->close();
}

void LobbyServer::receiveActivateGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string hostAccountID = json["hostAccountID"].String();
	std::string accountID = activeAccounts[connection];
	int playerLimit = json["playerLimit"].Integer();

	if(database->isPlayerInGameRoom(accountID))
		return sendOperationFailed(connection, "Player already in the room!");

	std::string gameRoomID = database->getIdleGameRoom(hostAccountID);
	if(gameRoomID.empty())
		return sendOperationFailed(connection, "Failed to find idle server to join!");

	std::string roomType = json["roomType"].String();
	if(roomType != "public" && roomType != "private")
		return sendOperationFailed(connection, "Invalid room type!");

	if(roomType == "public")
		database->setGameRoomStatus(gameRoomID, LobbyRoomState::PUBLIC);
	if(roomType == "private")
		database->setGameRoomStatus(gameRoomID, LobbyRoomState::PRIVATE);

	database->updateRoomPlayerLimit(gameRoomID, playerLimit);
	database->insertPlayerIntoGameRoom(accountID, gameRoomID);
	broadcastActiveGameRooms();
	sendJoinRoomSuccess(connection, gameRoomID, false);
}

void LobbyServer::receiveJoinGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = activeAccounts[connection];

	if(database->isPlayerInGameRoom(accountID))
		return sendOperationFailed(connection, "Player already in the room!");

	NetworkConnectionPtr targetRoom = findGameRoom(gameRoomID);

	if(targetRoom == nullptr)
		return sendOperationFailed(connection, "Failed to find game room to join!");

	auto roomStatus = database->getGameRoomStatus(gameRoomID);

	if(roomStatus != LobbyRoomState::PRIVATE && roomStatus != LobbyRoomState::PUBLIC)
		return sendOperationFailed(connection, "Room does not accepts new players!");

	if(roomStatus == LobbyRoomState::PRIVATE)
	{
		if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::INVITED)
			return sendOperationFailed(connection, "You are not permitted to join private room without invite!");
	}

	if(database->getGameRoomFreeSlots(gameRoomID) == 0)
		return sendOperationFailed(connection, "Room is already full!");

	database->insertPlayerIntoGameRoom(accountID, gameRoomID);
	sendAccountJoinsRoom(targetRoom, accountID);
	//No reply to client - will be sent once match server establishes proxy connection with lobby

	broadcastActiveGameRooms();
}

void LobbyServer::receiveChangeRoomDescription(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = activeGameRooms[connection];
	std::string description = json["description"].String();

	database->updateRoomDescription(gameRoomID, description);
	broadcastActiveGameRooms();
}

void LobbyServer::receiveGameStarted(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = activeGameRooms[connection];

	database->setGameRoomStatus(gameRoomID, LobbyRoomState::BUSY);
	broadcastActiveGameRooms();
}

void LobbyServer::receiveLeaveGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = json["accountID"].String();
	std::string gameRoomID = activeGameRooms[connection];

	if(!database->isPlayerInGameRoom(accountID, gameRoomID))
		return sendOperationFailed(connection, "You are not in the room!");

	database->deletePlayerFromGameRoom(accountID, gameRoomID);

	broadcastActiveGameRooms();
}

void LobbyServer::receiveSendInvite(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string senderName = activeAccounts[connection];
	std::string accountID = json["accountID"].String();
	std::string gameRoomID = database->getAccountGameRoom(senderName);

	auto targetAccountConnection = findAccount(accountID);

	if(!targetAccountConnection)
		return sendOperationFailed(connection, "Player is offline or does not exists!");

	if(!database->isPlayerInGameRoom(senderName))
		return sendOperationFailed(connection, "You are not in the room!");

	if(database->isPlayerInGameRoom(accountID))
		return sendOperationFailed(connection, "This player is already in a room!");

	if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::NOT_INVITED)
		return sendOperationFailed(connection, "This player is already invited!");

	database->insertGameRoomInvite(accountID, gameRoomID);
	sendInviteReceived(targetAccountConnection, senderName, gameRoomID);
}

LobbyServer::~LobbyServer() = default;

LobbyServer::LobbyServer(const boost::filesystem::path & databasePath)
	: database(std::make_unique<LobbyDatabase>(databasePath))
	, networkHandler(INetworkHandler::createHandler())
	, networkServer(networkHandler->createServerTCP(*this))
{
}

void LobbyServer::start(uint16_t port)
{
	networkServer->start(port);
}

void LobbyServer::run()
{
	networkHandler->run();
}
