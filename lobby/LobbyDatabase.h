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
	SQLiteStatementPtr updateAccountLoginTimeStatement;
	SQLiteStatementPtr updateRoomDescriptionStatement;
	SQLiteStatementPtr updateRoomPlayerLimitStatement;

	SQLiteStatementPtr getRecentMessageHistoryStatement;
	SQLiteStatementPtr getFullMessageHistoryStatement;
	SQLiteStatementPtr getIdleGameRoomStatement;
	SQLiteStatementPtr getGameRoomStatusStatement;
	SQLiteStatementPtr getAccountGameHistoryStatement;
	SQLiteStatementPtr getActiveGameRoomsStatement;
	SQLiteStatementPtr getActiveAccountsStatement;
	SQLiteStatementPtr getAccountInviteStatusStatement;
	SQLiteStatementPtr getAccountGameRoomStatement;
	SQLiteStatementPtr getAccountDisplayNameStatement;
	SQLiteStatementPtr countRoomUsedSlotsStatement;
	SQLiteStatementPtr countRoomTotalSlotsStatement;

	SQLiteStatementPtr isAccountCookieValidStatement;
	SQLiteStatementPtr isGameRoomCookieValidStatement;
	SQLiteStatementPtr isPlayerInGameRoomStatement;
	SQLiteStatementPtr isPlayerInAnyGameRoomStatement;
	SQLiteStatementPtr isAccountIDExistsStatement;
	SQLiteStatementPtr isAccountNameExistsStatement;

	void prepareStatements();
	void createTables();
	void clearOldData();

public:
	explicit LobbyDatabase(const boost::filesystem::path & databasePath);
	~LobbyDatabase();

	void setAccountOnline(const std::string & accountID, bool isOnline);
	void setGameRoomStatus(const std::string & roomID, LobbyRoomState roomStatus);

	void insertPlayerIntoGameRoom(const std::string & accountID, const std::string & roomID);
	void deletePlayerFromGameRoom(const std::string & accountID, const std::string & roomID);

	void deleteGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);
	void insertGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);

	void insertGameRoom(const std::string & roomID, const std::string & hostAccountID);
	void insertAccount(const std::string & accountID, const std::string & displayName);
	void insertAccessCookie(const std::string & accountID, const std::string & accessCookieUUID);
	void insertChatMessage(const std::string & sender, const std::string & channelType, const std::string & roomID, const std::string & messageText);

	void updateAccountLoginTime(const std::string & accountID);
	void updateRoomPlayerLimit(const std::string & gameRoomID, int playerLimit);
	void updateRoomDescription(const std::string & gameRoomID, const std::string & description);

	std::vector<LobbyGameRoom> getAccountGameHistory(const std::string & accountID);
	std::vector<LobbyGameRoom> getActiveGameRooms();
	std::vector<LobbyAccount> getActiveAccounts();
	std::vector<LobbyChatMessage> getRecentMessageHistory(const std::string & channelType, const std::string & channelName);
	std::vector<LobbyChatMessage> getFullMessageHistory(const std::string & channelType, const std::string & channelName);

	std::string getIdleGameRoom(const std::string & hostAccountID);
	std::string getAccountGameRoom(const std::string & accountID);
	std::string getAccountDisplayName(const std::string & accountID);

	LobbyCookieStatus getAccountCookieStatus(const std::string & accountID, const std::string & accessCookieUUID);
	LobbyInviteStatus getAccountInviteStatus(const std::string & accountID, const std::string & roomID);
	LobbyRoomState getGameRoomStatus(const std::string & roomID);
	uint32_t getGameRoomFreeSlots(const std::string & roomID);

	bool isPlayerInGameRoom(const std::string & accountID);
	bool isPlayerInGameRoom(const std::string & accountID, const std::string & roomID);
	bool isAccountNameExists(const std::string & displayName);
	bool isAccountIDExists(const std::string & accountID);
};
