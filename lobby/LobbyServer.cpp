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
#include "../lib/network/NetworkServer.h"

void LobbyServer::sendMessage(const std::shared_ptr<NetworkConnection> & target, const JsonNode & json)
{
	//FIXME: copy-paste from LobbyClient::sendMessage
	std::string payloadString = json.toJson(true);

	// FIXME: find better approach
	uint8_t * payloadBegin = reinterpret_cast<uint8_t *>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	networkServer->sendPacket(target, payloadBuffer);
}

void LobbyServer::onTimer()
{
	// no-op
}

void LobbyServer::onNewConnection(const std::shared_ptr<NetworkConnection> & connection)
{}

void LobbyServer::onDisconnected(const std::shared_ptr<NetworkConnection> & connection)
{
	activeAccounts.erase(connection);
}

void LobbyServer::onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	// FIXME: find better approach
	JsonNode json(message.data(), message.size());

	if(json["type"].String() == "sendChatMessage")
		return receiveSendChatMessage(connection, json);

	if(json["type"].String() == "authentication")
		return receiveAuthentication(connection, json);

	if(json["type"].String() == "joinGameRoom")
		return receiveJoinGameRoom(connection, json);
}

void LobbyServer::receiveSendChatMessage(const std::shared_ptr<NetworkConnection> & connection, const JsonNode & json)
{
	if(activeAccounts.count(connection) == 0)
		return; // unauthenticated

	std::string senderName = activeAccounts[connection].accountName;
	std::string messageText = json["messageText"].String();

	database->insertChatMessage(senderName, "general", "english", messageText);

	JsonNode reply;
	reply["type"].String() = "chatMessage";
	reply["messageText"].String() = messageText;
	reply["senderName"].String() = senderName;

	for(const auto & connection : activeAccounts)
		sendMessage(connection.first, reply);
}

void LobbyServer::receiveAuthentication(const std::shared_ptr<NetworkConnection> & connection, const JsonNode & json)
{
	std::string accountName = json["accountName"].String();

	// TODO: account cookie check
	// TODO: account password check
	// TODO: protocol version number
	// TODO: client/server mode flag
	// TODO: client language

	activeAccounts[connection].accountName = accountName;

	{
		JsonNode reply;
		reply["type"].String() = "authentication";

		sendMessage(connection, reply);
	}

	auto history = database->getRecentMessageHistory();

	{
		JsonNode reply;
		reply["type"].String() = "chatHistory";

		for(const auto & message : boost::adaptors::reverse(history))
		{
			JsonNode jsonEntry;

			jsonEntry["messageText"].String() = message.messageText;
			jsonEntry["senderName"].String() = message.sender;
			jsonEntry["ageSeconds"].Integer() = message.age.count();

			reply["messages"].Vector().push_back(jsonEntry);
		}

		sendMessage(connection, reply);
	}
}

void LobbyServer::receiveJoinGameRoom(const std::shared_ptr<NetworkConnection> & connection, const JsonNode & json)
{
	if(activeAccounts.count(connection) == 0)
		return; // unauthenticated

	std::string senderName = activeAccounts[connection].accountName;

	if(database->isPlayerInGameRoom(senderName))
		return; // only 1 room per player allowed

	// TODO: roomType: private, public
	// TODO: additional flags, e.g. allowCheats
	// TODO: connection mode: direct or proxy
}

LobbyServer::~LobbyServer() = default;

LobbyServer::LobbyServer(const std::string & databasePath)
	: database(new LobbyDatabase(databasePath))
	, networkServer(new NetworkServer(*this))
{
}

void LobbyServer::start(uint16_t port)
{
	networkServer->start(port);
}

void LobbyServer::run()
{
	networkServer->run();
}
