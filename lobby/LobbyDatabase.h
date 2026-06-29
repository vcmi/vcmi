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
#include "TimeTracker.h"

class SQLiteInstance;
class SQLiteStatement;

using SQLiteInstancePtr = std::unique_ptr<SQLiteInstance>;
using SQLiteStatementPtr = std::unique_ptr<SQLiteStatement>;

class LobbyDatabase
{
	SQLiteInstancePtr database;
	TimeTracker timeTracker;

	SQLiteStatementPtr insertChatMessageStatement;
	SQLiteStatementPtr insertAccountStatement;
	SQLiteStatementPtr insertAccessCookieStatement;
	SQLiteStatementPtr insertGameRoomStatement;
	SQLiteStatementPtr insertGameRoomModStatement;
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
	SQLiteStatementPtr getGameRoomStatement;
	SQLiteStatementPtr getActiveAccountsStatement;
	SQLiteStatementPtr getAccountInviteStatusStatement;
	SQLiteStatementPtr getAccountGameRoomStatement;
	SQLiteStatementPtr getAccountDisplayNameStatement;
	SQLiteStatementPtr getActiveAccountsCountsBatchStatement;
	SQLiteStatementPtr getRegisteredAccountsCountsBatchStatement;
	SQLiteStatementPtr getClosedGameRoomsCountsBatchStatement;
	SQLiteStatementPtr getRoomsStatement;
	SQLiteStatementPtr getGameRoomPlayersStatement;
	SQLiteStatementPtr getGameRoomModsStatement;
	SQLiteStatementPtr getGameRoomInvitesStatement;
	SQLiteStatementPtr countRoomUsedSlotsStatement;
	SQLiteStatementPtr countRoomTotalSlotsStatement;

	SQLiteStatementPtr isAccountCookieValidStatement;
	SQLiteStatementPtr isGameRoomCookieValidStatement;
	SQLiteStatementPtr isPlayerInGameRoomStatement;
	SQLiteStatementPtr isPlayerInAnyGameRoomStatement;
	SQLiteStatementPtr isAccountIDExistsStatement;
	SQLiteStatementPtr isAccountNameExistsStatement;

	void prepareStatements();
	void clearOldData();

	std::vector<LobbyAccount> getGameRoomPlayers(const std::string & gameRoomID);
	std::vector<LobbyAccount> getGameRoomInvites(const std::string & gameRoomID);
	std::vector<LobbyGameRoomMod> getGameRoomMods(const std::string & gameRoomID);

public:
	explicit LobbyDatabase(const boost::filesystem::path & databasePath);
	~LobbyDatabase();

	void setAccountOnline(const std::string & accountID, bool isOnline);
	void setGameRoomStatus(const std::string & roomID, LobbyRoomState roomStatus);

	void insertPlayerIntoGameRoom(const std::string & accountID, const std::string & roomID);
	void deletePlayerFromGameRoom(const std::string & accountID, const std::string & roomID);

	void deleteGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);
	void insertGameRoomInvite(const std::string & targetAccountID, const std::string & roomID);

	void insertGameRoom(const std::string & roomID, const std::string & hostAccountID, const std::string & serverVersion);
	void insertGameRoomMod(const std::string & roomID, const std::string & modID, const std::string & modName, const std::string & modVersion);
	void insertAccount(const std::string & accountID, const std::string & displayName);
	void insertAccessCookie(const std::string & accountID, const std::string & accessCookieUUID);
	void insertChatMessage(const std::string & sender, const std::string & channelType, const std::string & roomID, const std::string & messageText);

	void updateAccountLoginTime(const std::string & accountID);
	void updateRoomPlayerLimit(const std::string & gameRoomID, int playerLimit);
	void updateRoomDescription(const std::string & gameRoomID, const std::string & description);

	std::vector<LobbyGameRoom> getAccountGameHistory(const std::string & accountID);
	std::vector<LobbyGameRoom> getActiveGameRooms();
	LobbyGameRoom getGameRoom(const std::string & roomID);
	std::vector<LobbyAccount> getActiveAccounts();
	std::vector<LobbyGameRoom> getRooms(int hours, int limit);
	std::vector<LobbyChatMessage> getRecentMessageHistory(const std::string & channelType, const std::string & channelName);
	std::vector<LobbyChatMessage> getFullMessageHistory(const std::string & channelType, const std::string & channelName);

	std::string getIdleGameRoom(const std::string & hostAccountID);
	std::string getAccountGameRoom(const std::string & accountID);
	std::string getAccountDisplayName(const std::string & accountID);
	/// Batch account activity counts: total registered, plus active in last 1h/24h/1w/1m/1y
	struct ActiveAccountsCounts { int h1, h24, h168, h720, h8760; };
	ActiveAccountsCounts getActiveAccountsCounts();

	/// Batch registration counts: total, plus registered in last 24h/1w/1m/1y
	struct RegisteredAccountsCounts { int total, h24, h168, h720, h8760; };
	RegisteredAccountsCounts getRegisteredAccountsCounts();

	/// Batch closed game room counts: total, plus closed in last 24h/1w/1m/1y
	struct ClosedGameRoomsCounts { int total, h24, h168, h720, h8760; };
	ClosedGameRoomsCounts getClosedGameRoomsCounts();

	LobbyCookieStatus getAccountCookieStatus(const std::string & accountID, const std::string & accessCookieUUID);
	LobbyInviteStatus getAccountInviteStatus(const std::string & accountID, const std::string & roomID);
	LobbyRoomState getGameRoomStatus(const std::string & roomID);
	uint32_t getGameRoomFreeSlots(const std::string & roomID);

	bool isPlayerInGameRoom(const std::string & accountID);
	bool isPlayerInGameRoom(const std::string & accountID, const std::string & roomID);
	bool isAccountNameExists(const std::string & displayName);
	bool isAccountIDExists(const std::string & accountID);

	void printPerformanceStatistics();
};
