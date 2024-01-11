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

bool LobbyServer::isAccountNameValid(const std::string & accountName)
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

NetworkConnectionPtr LobbyServer::findAccount(const std::string & accountID)
{
	for(const auto & account : activeAccounts)
		if(account.second.accountID == accountID)
			return account.first;

	return nullptr;
}

NetworkConnectionPtr LobbyServer::findGameRoom(const std::string & gameRoomID)
{
	for(const auto & account : activeGameRooms)
		if(account.second.roomID == gameRoomID)
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

void LobbyServer::sendLoginFailed(const NetworkConnectionPtr & target, const std::string & reason)
{
	JsonNode reply;
	reply["type"].String() = "loginFailed";
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
		//		jsonEntry["status"].String() = account.status;
		reply["accounts"].Vector().push_back(jsonEntry);
	}

	for(const auto & connection : activeAccounts)
		sendMessage(connection.first, reply);
}

void LobbyServer::broadcastActiveGameRooms()
{
	auto activeGameRoomStats = database->getActiveGameRooms();
	JsonNode reply;
	reply["type"].String() = "activeGameRooms";

	for(const auto & gameRoom : activeGameRoomStats)
	{
		JsonNode jsonEntry;
		jsonEntry["gameRoomID"].String() = gameRoom.roomUUID;
		jsonEntry["status"].String() = gameRoom.roomStatus;
		jsonEntry["status"].Integer() = gameRoom.playersCount;
		jsonEntry["status"].Integer() = gameRoom.playersLimit;
		reply["gameRooms"].Vector().push_back(jsonEntry);
	}

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

void LobbyServer::sendJoinRoomSuccess(const NetworkConnectionPtr & target, const std::string & gameRoomID)
{
	JsonNode reply;
	reply["type"].String() = "joinRoomSuccess";
	reply["gameRoomID"].String() = gameRoomID;
	sendMessage(target, reply);
}

void LobbyServer::sendChatMessage(const NetworkConnectionPtr & target, const std::string & roomMode, const std::string & roomName, const std::string & accountID, std::string & displayName, const std::string & messageText)
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
			lockedPtr->sendPacket(message);
		return;
	}

	JsonNode json(message.data(), message.size());

	// TODO: check for json parsing errors
	// TODO: validate json based on received message type

	// communication messages from vcmiclient
	if(activeAccounts.count(connection))
	{
		if(json["type"].String() == "sendChatMessage")
			return receiveSendChatMessage(connection, json);

		if(json["type"].String() == "openGameRoom")
			return receiveOpenGameRoom(connection, json);

		if(json["type"].String() == "joinGameRoom")
			return receiveJoinGameRoom(connection, json);

		if(json["type"].String() == "sendInvite")
			return receiveSendInvite(connection, json);

		if(json["type"].String() == "declineInvite")
			return receiveDeclineInvite(connection, json);

		return;
	}

	// communication messages from vcmiserver
	if(activeGameRooms.count(connection))
	{
		if(json["type"].String() == "leaveGameRoom")
			return receiveLeaveGameRoom(connection, json);

		return;
	}

	// unauthorized connections - permit only login or register attempts
	if(json["type"].String() == "clientLogin")
		return receiveClientLogin(connection, json);

	if(json["type"].String() == "clientRegister")
		return receiveClientRegister(connection, json);

	if(json["type"].String() == "serverLogin")
		return receiveServerLogin(connection, json);

	if(json["type"].String() == "clientProxyLogin")
		return receiveClientProxyLogin(connection, json);

	if(json["type"].String() == "serverProxyLogin")
		return receiveServerProxyLogin(connection, json);

	// TODO: add logging of suspicious connections.
	networkServer->closeConnection(connection);
}

void LobbyServer::receiveSendChatMessage(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = activeAccounts[connection].accountID;
	std::string messageText = json["messageText"].String();
	std::string messageTextClean = sanitizeChatMessage(messageText);
	std::string displayName = database->getAccountDisplayName(accountID);

	if(messageTextClean.empty())
		return;

	database->insertChatMessage(accountID, "global", "english", messageText);

	for(const auto & connection : activeAccounts)
		sendChatMessage(connection.first, "global", "english", accountID, displayName, messageText);
}

void LobbyServer::receiveClientRegister(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string displayName = json["displayName"].String();
	std::string language = json["language"].String();

	if(isAccountNameValid(displayName))
		return sendLoginFailed(connection, "Illegal account name");

	if(database->isAccountNameExists(displayName))
		return sendLoginFailed(connection, "Account name already in use");

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
		return sendLoginFailed(connection, "Account not found");

	auto clientCookieStatus = database->getAccountCookieStatus(accountID, accountCookie, accountCookieLifetime);

	if(clientCookieStatus == LobbyCookieStatus::INVALID)
		return sendLoginFailed(connection, "Authentification failure");

	// prolong existing cookie
	database->updateAccessCookie(accountID, accountCookie);
	database->updateAccountLoginTime(accountID);

	std::string displayName = database->getAccountDisplayName(accountID);

	activeAccounts[connection].accountID = accountID;
	activeAccounts[connection].displayName = displayName;
	activeAccounts[connection].version = version;
	activeAccounts[connection].language = language;

	sendLoginSuccess(connection, accountCookie, displayName);
	sendChatHistory(connection, database->getRecentMessageHistory());

	// send active accounts list to new account
	// and update acount list to everybody else
	broadcastActiveAccounts();
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
		sendLoginFailed(connection, "Invalid credentials");
	}
	else
	{
		database->insertGameRoom(gameRoomID, accountID);
		activeGameRooms[connection].roomID = gameRoomID;
		sendLoginSuccess(connection, accountCookie, {});
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

	networkServer->closeConnection(connection);
}

void LobbyServer::receiveServerProxyLogin(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string guestAccountID = json["guestAccountID"].String();
	std::string hostCookie = json["hostCookie"].String();

	auto clientCookieStatus = database->getGameRoomCookieStatus(gameRoomID, hostCookie, accountCookieLifetime);

	if(clientCookieStatus != LobbyCookieStatus::INVALID)
	{
		NetworkConnectionPtr targetAccount = findAccount(guestAccountID);

		if(targetAccount == nullptr)
			return; // unknown / disconnected account

		sendJoinRoomSuccess(targetAccount, gameRoomID);

		AwaitingProxyState proxy;
		proxy.accountID = guestAccountID;
		proxy.roomID = gameRoomID;
		proxy.roomConnection = connection;
		awaitingProxies.push_back(proxy);
		return;
	}

	networkServer->closeConnection(connection);
}

void LobbyServer::receiveOpenGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string hostAccountID = json["hostAccountID"].String();
	std::string accountID = activeAccounts[connection].accountID;

	if(database->isPlayerInGameRoom(accountID))
		return; // only 1 room per player allowed

	std::string gameRoomID = database->getIdleGameRoom(hostAccountID);
	if(gameRoomID.empty())
		return;

	std::string roomType = json["roomType"].String();
	if(roomType == "public")
		database->setGameRoomStatus(gameRoomID, LobbyRoomState::PUBLIC);
	if(roomType == "private")
		database->setGameRoomStatus(gameRoomID, LobbyRoomState::PRIVATE);

	// TODO: additional flags / initial settings, e.g. allowCheats
	// TODO: connection mode: direct or proxy. For now direct is assumed

	broadcastActiveGameRooms();
	sendJoinRoomSuccess(connection, gameRoomID);
}

void LobbyServer::receiveJoinGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = activeAccounts[connection].accountID;

	if(database->isPlayerInGameRoom(accountID))
		return; // only 1 room per player allowed

	NetworkConnectionPtr targetRoom = findGameRoom(gameRoomID);

	if(targetRoom == nullptr)
		return; // unknown / disconnected room

	auto roomStatus = database->getGameRoomStatus(gameRoomID);

	if(roomStatus == LobbyRoomState::PRIVATE)
	{
		if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::INVITED)
			return;
	}

	if(database->getGameRoomFreeSlots(gameRoomID) == 0)
		return;

	sendAccountJoinsRoom(targetRoom, accountID);
	//No reply to client - will be sent once match server establishes proxy connection with lobby

	broadcastActiveGameRooms();
}

void LobbyServer::receiveLeaveGameRoom(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string gameRoomID = json["gameRoomID"].String();
	std::string senderName = activeAccounts[connection].accountID;

	if(!database->isPlayerInGameRoom(senderName, gameRoomID))
		return;

	broadcastActiveGameRooms();
}

void LobbyServer::receiveSendInvite(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string senderName = activeAccounts[connection].accountID;
	std::string accountID = json["accountID"].String();
	std::string gameRoomID = database->getAccountGameRoom(senderName);

	auto targetAccount = findAccount(accountID);

	if(!targetAccount)
		return; // target player does not exists or offline

	if(!database->isPlayerInGameRoom(senderName))
		return; // current player is not in room

	if(database->isPlayerInGameRoom(accountID))
		return; // target player is busy

	if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::NOT_INVITED)
		return; // already has invite

	database->insertGameRoomInvite(accountID, gameRoomID);
	sendInviteReceived(targetAccount, senderName, gameRoomID);
}

void LobbyServer::receiveDeclineInvite(const NetworkConnectionPtr & connection, const JsonNode & json)
{
	std::string accountID = activeAccounts[connection].accountID;
	std::string gameRoomID = json["gameRoomID"].String();

	if(database->getAccountInviteStatus(accountID, gameRoomID) != LobbyInviteStatus::INVITED)
		return; // already has invite

	database->deleteGameRoomInvite(accountID, gameRoomID);
}

LobbyServer::~LobbyServer() = default;

LobbyServer::LobbyServer(const boost::filesystem::path & databasePath)
	: database(new LobbyDatabase(databasePath))
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
