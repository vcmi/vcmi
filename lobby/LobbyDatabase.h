/*
 * LobbyDatabase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class SQLiteInstance;
class SQLiteStatement;

using SQLiteInstancePtr = std::unique_ptr<SQLiteInstance>;
using SQLiteStatementPtr = std::unique_ptr<SQLiteStatement>;

class LobbyDatabase
{
	SQLiteInstancePtr database;

	SQLiteStatementPtr insertChatMessageStatement;
	SQLiteStatementPtr insertAccountStatement;
	SQLiteStatementPtr insertAccessCookieStatement;
	SQLiteStatementPtr insertGameRoomStatement;

	SQLiteStatementPtr checkAccessCookieStatement;
	SQLiteStatementPtr isPlayerInGameRoomStatement;
	SQLiteStatementPtr isAccountNameAvailableStatement;

	SQLiteStatementPtr getRecentMessageHistoryStatement;
	SQLiteStatementPtr setGameRoomStatusStatement;
	SQLiteStatementPtr setGameRoomPlayerLimitStatement;
	SQLiteStatementPtr insertPlayerIntoGameRoomStatement;
	SQLiteStatementPtr removePlayerFromGameRoomStatement;

	void initializeDatabase();
	void prepareStatements();

	void createTableChatMessages();
	void createTableGameRoomPlayers();
	void createTableGameRooms();
	void createTableAccounts();
	void createTableAccountCookies();

public:
	struct GameRoom
	{
		std::string roomUUID;
		std::string roomStatus;
		std::chrono::seconds age;
		uint32_t playersCount;
		uint32_t playersLimit;
	};

	struct ChatMessage
	{
		std::string sender;
		std::string messageText;
		std::chrono::seconds age;
	};

	explicit LobbyDatabase(const std::string & databasePath);
	~LobbyDatabase();

	void setGameRoomStatus(const std::string & roomUUID, const std::string & roomStatus);
	void setGameRoomPlayerLimit(const std::string & roomUUID, uint32_t playerLimit);

	void insertPlayerIntoGameRoom(const std::string & accountName, const std::string & roomUUID);
	void removePlayerFromGameRoom(const std::string & accountName, const std::string & roomUUID);

	void insertGameRoom(const std::string & roomUUID, const std::string & hostAccountName);
	void insertAccount(const std::string & accountName);
	void insertAccessCookie(const std::string & accountName, const std::string & accessCookieUUID, std::chrono::seconds cookieLifetime);
	void insertChatMessage(const std::string & sender, const std::string & roomType, const std::string & roomName, const std::string & messageText);

	std::vector<GameRoom> getActiveGameRooms();
	std::vector<ChatMessage> getRecentMessageHistory();

	bool checkAccessCookie(const std::string & accountName,const std::string & accessCookieUUID);
	bool isPlayerInGameRoom(const std::string & accountName);
	bool isAccountNameAvailable(const std::string & accountName);
};
