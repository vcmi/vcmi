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

#include "SQLiteConnection.h"

#include "../lib/JsonNode.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

static const std::string DATABASE_PATH = "/home/ivan/vcmi.db";
static const int LISTENING_PORT = 30303;
//static const std::string SERVER_NAME = GameConstants::VCMI_VERSION + " (server)";
//static const std::string SERVER_UUID = boost::uuids::to_string(boost::uuids::random_generator()());

void LobbyDatabase::prepareStatements()
{
	static const std::string insertChatMessageText = R"(
		INSERT INTO chatMessages(senderName, messageText) VALUES( ?, ?);
	)";

	static const std::string getRecentMessageHistoryText = R"(
		SELECT senderName, messageText, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',sendTime)  AS secondsElapsed
		FROM chatMessages
		WHERE secondsElapsed < 60*60*24
		ORDER BY sendTime DESC
		LIMIT 100
	)";

	insertChatMessageStatement = database->prepare(insertChatMessageText);
	getRecentMessageHistoryStatement = database->prepare(getRecentMessageHistoryText);
}

void LobbyDatabase::createTableChatMessages()
{
	static const std::string statementText = R"(
		CREATE TABLE IF NOT EXISTS chatMessages (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			senderName TEXT,
			messageText TEXT,
			sendTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
		);
	)";

	auto statement = database->prepare(statementText);
	statement->execute();
}

void LobbyDatabase::initializeDatabase()
{
	createTableChatMessages();
}

LobbyDatabase::LobbyDatabase()
{
	database = SQLiteInstance::open(DATABASE_PATH, true);

	if (!database)
		throw std::runtime_error("Failed to open SQLite database!");

	initializeDatabase();
	prepareStatements();
}

void LobbyDatabase::insertChatMessage(const std::string & sender, const std::string & messageText)
{
	insertChatMessageStatement->setBinds(sender, messageText);
	insertChatMessageStatement->execute();
	insertChatMessageStatement->reset();
}

std::vector<LobbyDatabase::ChatMessage> LobbyDatabase::getRecentMessageHistory()
{
	std::vector<LobbyDatabase::ChatMessage> result;

	while(getRecentMessageHistoryStatement->execute())
	{
		LobbyDatabase::ChatMessage message;
		getRecentMessageHistoryStatement->getColumns(message.sender, message.messageText, message.messageAgeSeconds);
		result.push_back(message);
	}
	getRecentMessageHistoryStatement->reset();

	return result;
}

void LobbyServer::sendMessage(const std::shared_ptr<NetworkConnection> & target, const JsonNode & json)
{
	//FIXME: copy-paste from LobbyClient::sendMessage
	std::string payloadString = json.toJson(true);

	// FIXME: find better approach
	uint8_t * payloadBegin = reinterpret_cast<uint8_t*>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	networkServer->sendPacket(target, payloadBuffer);
}

void LobbyServer::onNewConnection(const std::shared_ptr<NetworkConnection> & connection)
{
}

void LobbyServer::onDisconnected(const std::shared_ptr<NetworkConnection> & connection)
{
	activeAccounts.erase(connection);
}

void LobbyServer::onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	// FIXME: find better approach
	const char * payloadBegin = reinterpret_cast<const char*>(message.data());
	JsonNode json(payloadBegin, message.size());

	if (json["type"].String() == "sendChatMessage")
	{
		if (activeAccounts.count(connection) == 0)
			return; // unauthenticated

		std::string senderName = activeAccounts[connection].accountName;
		std::string messageText = json["messageText"].String();

		database->insertChatMessage(senderName, messageText);

		JsonNode reply;
		reply["type"].String() = "chatMessage";
		reply["messageText"].String() = messageText;
		reply["senderName"].String() = senderName;

		for (auto const & connection : activeAccounts)
			sendMessage(connection.first, reply);
	}

	if (json["type"].String() == "authentication")
	{
		std::string accountName = json["accountName"].String();

		activeAccounts[connection].accountName = accountName;

		auto history = database->getRecentMessageHistory();

		JsonNode json;
		json["type"].String() = "chatHistory";

		for (auto const & message : boost::adaptors::reverse(history))
		{
			JsonNode jsonEntry;

			jsonEntry["messageText"].String() = message.messageText;
			jsonEntry["senderName"].String() = message.sender;
			jsonEntry["ageSeconds"].Integer() = message.messageAgeSeconds;

			json["messages"].Vector().push_back(jsonEntry);
		}

		sendMessage(connection, json);
	}
}

LobbyServer::LobbyServer()
	: database(new LobbyDatabase())
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

int main(int argc, const char * argv[])
{
	LobbyServer server;

	server.start(LISTENING_PORT);
	server.run();
}