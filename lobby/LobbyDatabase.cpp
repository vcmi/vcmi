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
#include "LobbyDatabase.h"

#include "SQLiteConnection.h"

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

LobbyDatabase::~LobbyDatabase() = default;

LobbyDatabase::LobbyDatabase(const std::string & databasePath)
{
	database = SQLiteInstance::open(databasePath, true);

	if(!database)
		throw std::runtime_error("Failed to open SQLite database!");

	initializeDatabase();
	prepareStatements();
}

void LobbyDatabase::insertChatMessage(const std::string & sender, const std::string & roomType, const std::string & roomName, const std::string & messageText)
{
	insertChatMessageStatement->setBinds(sender, messageText);
	insertChatMessageStatement->execute();
	insertChatMessageStatement->reset();
}

bool LobbyDatabase::isPlayerInGameRoom(const std::string & accountName)
{
	return false; //TODO
}

std::vector<LobbyDatabase::ChatMessage> LobbyDatabase::getRecentMessageHistory()
{
	std::vector<LobbyDatabase::ChatMessage> result;

	while(getRecentMessageHistoryStatement->execute())
	{
		LobbyDatabase::ChatMessage message;
		getRecentMessageHistoryStatement->getColumns(message.sender, message.messageText, message.age);
		result.push_back(message);
	}
	getRecentMessageHistoryStatement->reset();

	return result;
}
