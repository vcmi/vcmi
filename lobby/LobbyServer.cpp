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

#include "../lib/JsonNode.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

static const auto accountCookieLifetime = std::chrono::hours(24 * 7);

bool LobbyServer::isAccountNameValid(const std::string & accountName) const
{
	if(accountName.size() < 4)
		return false;

	if(accountName.size() < 20)
		return false;

	for(const auto & c : accountName)
		if(!std::isalnum(c))
			return false;

	return true;
}

std::string LobbyServer::sanitizeChatMessage(const std::string & inputString) const
{
	// TODO: sanitize message and remove any "weird" symbols from it
	return inputString;
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
	//NOTE: copy-paste from LobbyClient::sendMessage
	std::string payloadString = json.toJson(true);

	// TODO: find better approach
	const uint8_t * payloadBegin = reinterpret_cast<uint8_t *>(payloadString.data());
	const uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	networkServer->sendPacket(target, payloadBuffer);
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

void LobbyServer::sendLoginSuccess(const NetworkConnectionPtr & target, const std::string & accountCookie, const std::string & displayName)
{
	JsonNode reply;
	reply["type"].String() = "loginSuccess";
	reply["accountCookie"].String() = accountCookie;
	if(!displayName.empty())
		reply["displayName"].String() = displayName;
	sendMessage(target, reply);
}

void LobbyServer::sendChatHistory(const NetworkConnectionPtr & target, const std::vector<LobbyChatMessage> & history)
{
	JsonNode reply;
	reply["type"].String() = "chatHistory";

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

JsonNode LobbyServer::prepareActiveGameRooms()
{
	auto activeGameRoomStats = database->getActiveGameRooms();
	JsonNode reply;
	reply["type"].String() = "activeGameRooms";

	for(const auto & gameRoom : activeGameRoomStats)
	{
		JsonNode jsonEntry;
		jsonEntry["gameRoomID"].String() = gameRoom.roomID;
		jsonEntry["hostAccountID"].String() = gameRoom.hostAccountID;
		jsonEntry["hostAccountDisplayName"].String() = gameRoom.hostAccountDisplayName;
		jsonEntry["description"].String() = "TODO: ROOM DESCRIPTION";
		jsonEntry["playersCount"].Integer() = gameRoom.playersCount;
		jsonEntry["playersLimit"].Integer() = gameRoom.playersLimit;
		reply["gameRooms"].Vector().push_back(jsonEntry);
	}

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

void LobbyServer::sendChatMessage(const NetworkConnectionPtr & target, const std::string & roomMode, const std::string & roomName, const std::string & accountID, const std::string & displayName, const std::string & messageText)
{
	JsonNode reply;
	reply["type"].String() = "chatMessage";
	reply["messageText"].String() = messageText;
	reply["accountID"].String() = accountID;
	reply["displayName"].String() = displayName;
	reply["roomMode"].String() = roomMode;
	reply["roomName"].String() = roomName;

	sendMessage(target, reply);
}

void LobbyServer::onNewConnection(const NetworkConnectionPtr & connection)
{
	// no-op - waiting for incoming data
}

void LobbyServer::onDisconnected(const NetworkConnectionPtr & connection)
{
	if (activeAccounts.count(connection))
		database->setAccountOnline(activeAccounts.at(connection), false);

	if (activeGameRooms.count(connection))
		database->setGameRoomStatus(activeGameRooms.at(connection), LobbyRoomState::CLOSED);

	// NOTE: lost connection can be in only one of these lists (or in none of them)
	// calling on all possible containers since calling std::map::erase() with non-existing key is legal
	activeAccounts.erase(connection);
	activeProxies.erase(connection);
	activeGameRooms.erase(connection);

	broadcastActiveAccounts();
	broadcastActiveGameRooms();
}

void LobbyServer::onPacketReceived(const NetworkConnectionPtr & connection, const std::vector<uint8_t> & message)
{
	// proxy connection - no processing, only redirect
	if(activeProxies.count(connection))
	{
		auto lockedPtr = activeProxies.at(connection).lock();
		if(lockedPtr)
			return lockedPtr->sendPacket(message);

		logGlobal->info("Received unexpected message for inactive proxy!");
	}

	JsonNode json(message.data(), message.size());

	// TODO: check for json parsing errors
	// TODO: validate json based on received message type

	std::string messageType = json["type"].String();

	// communication messages from vcmiclient
	if(activeAccounts.count(connection))
	{
		std::string accountName = activeAccounts.at(connection);
		logGlobal->info("%s: Received message of type %s", accountName, messageType);

		if(messageType == "sendChatMessage")
			return receiveSendChatMessage(connection, json);

		if(messageType == "openGameRoom")
			return receiveOpenGameRoom(connection, json);

		if(messageType == "joinGameRoom")
			return receiveJoinGameRoom(connection, json);

		if(messageType == "sendInvite")
			return receiveSendInvite(connection, json);

		if(messageType == "declineInvite")
			return receiveDeclineInvite(connection, json);

		logGlobal->warn("%s: Unknown message type: %s", accountName, messageType);
		return;
	}

	// communication messages from vcmiserver
	if(activeGameRooms.count(connection))
	{
		std::string roomName = activeGameRooms.at(connection);
		logGlobal->info("%s: Received message of type %s", roomName, messageType);

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

	// TODO: add logging of suspicious connections.
	networkServer->closeConnection(connection);

	logGlobal->info("(unauthorised): Unknown message type %s", messageType);
}

void LobbyServer::receiveSendChatMessage(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = activeAccounts[connection];
	std::string messageText = json["messageText"].String();
	std::string messageTextClean = sanitizeChatMessage(messageText);
	std::string displayName = database->getAccountDisplayName(accountID);

	if(messageTextClean.empty())
		return sendOperationFailed(connection, "No printable characters in sent message!");

	database->insertChatMessage(accountID, "global", "english", messageText);

	for(const auto & otherConnection : activeAccounts)
		sendChatMessage(otherConnection.first, "global", "english", accountID, displayName, messageText);
}

void LobbyServer::receiveClientRegister(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string displayName = json["displayName"].String();
	std::string language = json["language"].String();

	if(isAccountNameValid(displayName))
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

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie, accountCookieLifetime);

	if(clientCookieStatus == LobbyCookieStatus::INVALID)
		return sendOperationFailed(connection, "Authentification failure");

	// prolong existing cookie
	database->updateAccessCookie(accountID, accountCookie);
	database->updateAccountLoginTime(accountID);
	database->setAccountOnline(accountID, true);

	std::string displayName = database->getAccountDisplayName(accountID);

	activeAccounts[connection] = accountID;

	sendLoginSuccess(connection, accountCookie, displayName);
	sendChatHistory(connection, database->getRecentMessageHistory());

	// send active accounts list to new account
	// and update acount list to everybody else
	broadcastActiveAccounts();
	sendMessage(connection, prepareActiveGameRooms());
}

void LobbyServer::receiveServerLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = json["accountID"].String();
	std::string accountCookie = json["accountCookie"].String();
	std::string version = json["version"].String();

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie, accountCookieLifetime);

	if(clientCookieStatus == LobbyCookieStatus::INVALID)
	{
		sendOperationFailed(connection, "Invalid credentials");
	}
	else
	{
		database->insertGameRoom(gameRoomID, accountID);
		activeGameRooms[connection] = gameRoomID;
		sendLoginSuccess(connection, accountCookie, {});
		broadcastActiveGameRooms();
	}
}

void LobbyServer::receiveClientProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = json["accountID"].String();
	std::string accountCookie = json["accountCookie"].String();

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie, accountCookieLifetime);

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
	networkServer->closeConnection(connection);
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

	//networkServer->closeConnection(connection);
}

void LobbyServer::receiveOpenGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string hostAccountID = json["hostAccountID"].String();
	std::string accountID = activeAccounts[connection];

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

	// TODO: additional flags / initial settings, e.g. allowCheats
	// TODO: connection mode: direct or proxy. For now direct is assumed. Proxy might be needed later, for hosted servers

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

void LobbyServer::receiveLeaveGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string senderName = activeAccounts[connection];

	if(!database->isPlayerInGameRoom(senderName, gameRoomID))
		return sendOperationFailed(connection, "You are not in the room!");

	broadcastActiveGameRooms();
}

void LobbyServer::receiveSendInvite(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string senderName = activeAccounts[connection];
	std::string accountID = json["accountID"].String();
	std::string gameRoomID = database->getAccountGameRoom(senderName);

	auto targetAccount = findAccount(accountID);

	if(!targetAccount)
		return sendOperationFailed(connection, "Invalid account to invite!");

	if(!database->isPlayerInGameRoom(senderName))
		return sendOperationFailed(connection, "You are not in the room!");

	if(database->isPlayerInGameRoom(accountID))
		return sendOperationFailed(connection, "This player is already in a room!");

	if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::NOT_INVITED)
		return sendOperationFailed(connection, "This player is already invited!");

	database->insertGameRoomInvite(accountID, gameRoomID);
	sendInviteReceived(targetAccount, senderName, gameRoomID);
}

void LobbyServer::receiveDeclineInvite(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = activeAccounts[connection];
	std::string gameRoomID = json["gameRoomID"].String();

	if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::INVITED)
		return sendOperationFailed(connection, "No active invite found!");

	database->deleteGameRoomInvite(accountID, gameRoomID);
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
