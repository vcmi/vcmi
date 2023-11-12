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

	insertChatMessageStatement = database->prepare(insertChatMessageText);
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

void LobbyServer::onNewConnection(const std::shared_ptr<NetworkConnection> &)
{

}

void LobbyServer::onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message)
{
	// FIXME: find better approach
	const char * payloadBegin = reinterpret_cast<const char*>(message.data());
	JsonNode json(payloadBegin, message.size());

	if (json["type"].String() == "sendChatMessage")
	{
		database->insertChatMessage("Unknown", json["messageText"].String());
	}
}

LobbyServer::LobbyServer()
	: database(new LobbyDatabase())
{
}

int main(int argc, const char * argv[])
{
	LobbyServer server;

	server.start(LISTENING_PORT);
	server.run();
}
