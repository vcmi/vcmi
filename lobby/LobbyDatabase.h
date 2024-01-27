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

#include "LobbyDefines.h"

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
	SQLiteStatementPtr insertGameRoomPlayersStatement;
	SQLiteStatementPtr insertGameRoomInvitesStatement;

	SQLiteStatementPtr deleteGameRoomPlayersStatement;
	SQLiteStatementPtr deleteGameRoomInvitesStatement;

	SQLiteStatementPtr setAccountOnlineStatement;
	SQLiteStatementPtr setGameRoomStatusStatement;
	SQLiteStatementPtr setGameRoomPlayerLimitStatement;

	SQLiteStatementPtr getRecentMessageHistoryStatement;
	SQLiteStatementPtr getIdleGameRoomStatement;
	SQLiteStatementPtr getActiveGameRoomsStatement;
	SQLiteStatementPtr getActiveAccountsStatement;
	SQLiteStatementPtr getAccountGameRoomStatement;
	SQLiteStatementPtr getAccountDisplayNameStatement;
	SQLiteStatementPtr countAccountsInRoomStatement;

	SQLiteStatementPtr isAccountCookieValidStatement;
	SQLiteStatementPtr isGameRoomCookieValidStatement;
	SQLiteStatementPtr isPlayerInGameRoomStatement;
	SQLiteStatementPtr isPlayerInAnyGameRoomStatement;
	SQLiteStatementPtr isAccountIDExistsStatement;
	SQLiteStatementPtr isAccountNameExistsStatement;

	void prepareStatements();
	void createTables();

public:
	explicit LobbyDatabase(const boost::filesystem::path & databasePath);
	~LobbyDatabase();

	void setAccountOnline(const std::string & accountID, bool isOnline);
	void setGameRoomStatus(const std::string & roomID, LobbyRoomState roomStatus);
	void setGameRoomPlayerLimit(const std::string & roomID, uint32_t playerLimit);

	void insertPlayerIntoGameRoom(const std::string & accountID, const std::string & roomID);
	void deletePlayerFromGameRoom(const std::string & accountID, const std::string & roomID);

	void deleteGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);
	void insertGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);

	void insertGameRoom(const std::string & roomID, const std::string & hostAccountID);
	void insertAccount(const std::string & accountID, const std::string & displayName);
	void insertAccessCookie(const std::string & accountID, const std::string & accessCookieUUID);
	void insertChatMessage(const std::string & sender, const std::string & roomType, const std::string & roomID, const std::string & messageText);

	void updateAccessCookie(const std::string & accountID, const std::string & accessCookieUUID);
	void updateAccountLoginTime(const std::string & accountID);
	void updateActiveAccount(const std::string & accountID, bool isActive);

	std::vector<LobbyGameRoom> getActiveGameRooms();
	std::vector<LobbyAccount> getActiveAccounts();
	std::vector<LobbyAccount> getAccountsInRoom(const std::string & roomID);
	std::vector<LobbyChatMessage> getRecentMessageHistory();

	std::string getIdleGameRoom(const std::string & hostAccountID);
	std::string getAccountGameRoom(const std::string & accountID);
	std::string getAccountDisplayName(const std::string & accountID);

	LobbyCookieStatus getGameRoomCookieStatus(const std::string & accountID, const std::string & accessCookieUUID, std::chrono::seconds cookieLifetime);
	LobbyCookieStatus getAccountCookieStatus(const std::string & accountID, const std::string & accessCookieUUID, std::chrono::seconds cookieLifetime);
	LobbyInviteStatus getAccountInviteStatus(const std::string & accountID, const std::string & roomID);
	LobbyRoomState getGameRoomStatus(const std::string & roomID);
	uint32_t getGameRoomFreeSlots(const std::string & roomID);

	bool isPlayerInGameRoom(const std::string & accountID);
	bool isPlayerInGameRoom(const std::string & accountID, const std::string & roomID);
	bool isAccountNameExists(const std::string & displayName);
	bool isAccountIDExists(const std::string & accountID);
};
