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

void LobbyDatabase::clearOldData()
{
	static const std::string removeActiveAccounts = R"(
		UPDATE accounts
		SET online = 0
		WHERE online <> 0
	)";

	static const std::string removeActiveLobbyRooms = R"(
		UPDATE gameRooms
		SET status = 4
		WHERE status IN (0,1,2)
	)";
	static const std::string removeActiveGameRooms = R"(
		UPDATE gameRooms
		SET status = 5
		WHERE status = 3
	)";

	database->prepare(removeActiveAccounts)->execute();
	database->prepare(removeActiveLobbyRooms)->execute();
	database->prepare(removeActiveGameRooms)->execute();
}

void LobbyDatabase::prepareStatements()
{
	// INSERT INTO

	insertChatMessageStatement = database->prepare(R"(
		INSERT INTO chatMessages(senderName, messageText, channelType, channelName) VALUES( ?, ?, ?, ?);
	)");

	insertAccountStatement = database->prepare(R"(
		INSERT INTO accounts(accountID, displayName, online) VALUES(?,?,0);
	)");

	insertAccessCookieStatement = database->prepare(R"(
		INSERT INTO accountCookies(accountID, cookieUUID) VALUES(?,?);
	)");

	insertGameRoomStatement = database->prepare(R"(
		INSERT INTO gameRooms(roomID, hostAccountID, status, playerLimit, version) VALUES(?, ?, 0, 8, ?);
	)");

	insertGameRoomModStatement = database->prepare(R"(
		INSERT INTO gameRoomMods(roomID, modID, modName, modVersion) VALUES(?, ?, ?, ?);
	)");
	insertGameRoomPlayersStatement = database->prepare(R"(
		INSERT INTO gameRoomPlayers(roomID, accountID) VALUES(?,?);
	)");

	insertGameRoomInvitesStatement = database->prepare(R"(
		INSERT INTO gameRoomInvites(roomID, accountID) VALUES(?,?);
	)");

	// DELETE FROM

	deleteGameRoomPlayersStatement = database->prepare(R"(
		 DELETE FROM gameRoomPlayers WHERE roomID = ? AND accountID = ?
	)");

	// UPDATE

	setAccountOnlineStatement = database->prepare(R"(
		UPDATE accounts
		SET online = ?
		WHERE accountID = ?
	)");

	setGameRoomStatusStatement = database->prepare(R"(
		UPDATE gameRooms
		SET status = ?
		WHERE roomID = ?
	)");

	updateAccountLoginTimeStatement = database->prepare(R"(
		UPDATE accounts
		SET lastLoginTime = CURRENT_TIMESTAMP
		WHERE accountID = ?
	)");

	updateRoomDescriptionStatement = database->prepare(R"(
		UPDATE gameRooms
		SET description = ?
		WHERE roomID  = ?
	)");

	updateRoomPlayerLimitStatement = database->prepare(R"(
		UPDATE gameRooms
		SET playerLimit = ?
		WHERE roomID  = ?
	)");

	// SELECT FROM

	getRecentMessageHistoryStatement = database->prepare(R"(
		SELECT senderName, displayName, messageText, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',cm.creationTime)  AS secondsElapsed
		FROM chatMessages cm
		LEFT JOIN accounts on accountID = senderName
		WHERE secondsElapsed < 60*60*18 AND channelType = ? AND channelName = ?
		ORDER BY cm.creationTime DESC
		LIMIT 100
	)");

	getFullMessageHistoryStatement = database->prepare(R"(
		SELECT senderName, displayName, messageText, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',cm.creationTime) AS secondsElapsed
		FROM chatMessages cm
		LEFT JOIN accounts on accountID = senderName
		WHERE channelType = ? AND channelName = ?
		ORDER BY cm.creationTime DESC
	)");

	getIdleGameRoomStatement = database->prepare(R"(
		SELECT roomID
		FROM gameRooms
		WHERE hostAccountID = ? AND status = 0
		LIMIT 1
	)");

	getGameRoomStatusStatement = database->prepare(R"(
		SELECT status
		FROM gameRooms
		WHERE roomID = ?
	)");

	getAccountInviteStatusStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM gameRoomInvites
		WHERE accountID = ? AND roomID = ?
	)");

	getAccountGameHistoryStatement = database->prepare(R"(
		SELECT gr.roomID, hostAccountID, displayName, description, status, playerLimit, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',gr.creationTime)  AS secondsElapsed
		FROM gameRoomPlayers grp
		LEFT JOIN gameRooms gr ON gr.roomID = grp.roomID
		LEFT JOIN accounts a ON gr.hostAccountID = a.accountID
		WHERE grp.accountID = ? AND status = 5
		ORDER BY secondsElapsed ASC
	)");

	getAccountGameRoomStatement = database->prepare(R"(
		SELECT grp.roomID
		FROM gameRoomPlayers grp
		LEFT JOIN gameRooms gr ON gr.roomID = grp.roomID
		WHERE accountID = ? AND status >= 1 AND status <= 3
		LIMIT 1
	)");

	getActiveAccountsStatement = database->prepare(R"(
		SELECT accountID, displayName
		FROM accounts
		WHERE online = 1
	)");

	getActiveGameRoomsStatement = database->prepare(R"(
		SELECT roomID, hostAccountID, displayName, description, status, playerLimit, version, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',gr.creationTime)  AS secondsElapsed
		FROM gameRooms gr
		LEFT JOIN accounts a ON gr.hostAccountID = a.accountID
		WHERE status >= 1 AND status <= 3
		ORDER BY secondsElapsed ASC
	)");

	getGameRoomStatement = database->prepare(R"(
		SELECT hostAccountID, displayName, description, status, playerLimit, version, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',gr.creationTime)  AS secondsElapsed
		FROM gameRooms gr
		LEFT JOIN accounts a ON gr.hostAccountID = a.accountID
		WHERE roomID = ?
	)");

	getRoomsStatement = database->prepare(R"(
		SELECT roomID, hostAccountID, displayName, description, status, playerLimit, version, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',gr.creationTime) AS secondsElapsed
		FROM gameRooms gr
		LEFT JOIN accounts a ON gr.hostAccountID = a.accountID
		WHERE (? = -1 OR strftime('%s',CURRENT_TIMESTAMP) - strftime('%s',gr.creationTime) < ? * 3600)
		ORDER BY gr.creationTime DESC
		LIMIT ?
	)");

	getGameRoomInvitesStatement = database->prepare(R"(
		SELECT a.accountID, a.displayName
		FROM gameRoomInvites gri
		LEFT JOIN accounts a ON a.accountID = gri.accountID
		WHERE roomID = ?
	)");

	getGameRoomPlayersStatement = database->prepare(R"(
		SELECT a.accountID, a.displayName
		FROM gameRoomPlayers grp
		LEFT JOIN accounts a ON a.accountID = grp.accountID
		WHERE roomID = ?
	)");

	getGameRoomModsStatement = database->prepare(R"(
		SELECT modID, modName, modVersion
		FROM gameRoomMods grm
		WHERE roomID = ?
	)");

	countRoomUsedSlotsStatement = database->prepare(R"(
		SELECT COUNT(grp.accountID)
		FROM gameRoomPlayers grp
		WHERE roomID = ?
	)");

	countRoomTotalSlotsStatement = database->prepare(R"(
		SELECT playerLimit
		FROM gameRooms
		WHERE roomID = ?
	)");

	getAccountDisplayNameStatement = database->prepare(R"(
		SELECT displayName
		FROM accounts
		WHERE accountID = ?
	)");

	getActiveAccountsCountsBatchStatement = database->prepare(R"(
		SELECT
			SUM(lastLoginTime >= datetime('now', '-1 hours')),
			SUM(lastLoginTime >= datetime('now', '-24 hours')),
			SUM(lastLoginTime >= datetime('now', '-168 hours')),
			SUM(lastLoginTime >= datetime('now', '-720 hours')),
			SUM(lastLoginTime >= datetime('now', '-8760 hours'))
		FROM accounts
	)");

	getRegisteredAccountsCountsBatchStatement = database->prepare(R"(
		SELECT
			COUNT(*),
			SUM(creationTime >= datetime('now', '-24 hours')),
			SUM(creationTime >= datetime('now', '-168 hours')),
			SUM(creationTime >= datetime('now', '-720 hours')),
			SUM(creationTime >= datetime('now', '-8760 hours'))
		FROM accounts
	)");

	getClosedGameRoomsCountsBatchStatement = database->prepare(R"(
		SELECT
			COUNT(*),
			SUM(creationTime >= datetime('now', '-24 hours')),
			SUM(creationTime >= datetime('now', '-168 hours')),
			SUM(creationTime >= datetime('now', '-720 hours')),
			SUM(creationTime >= datetime('now', '-8760 hours'))
		FROM gameRooms WHERE status = 5
	)");

	isAccountCookieValidStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM accountCookies
		WHERE accountID = ? AND cookieUUID = ?
	)");

	isPlayerInGameRoomStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM gameRoomPlayers grp
		LEFT JOIN gameRooms gr ON gr.roomID = grp.roomID
		WHERE accountID = ? AND grp.roomID = ?
	)");

	isPlayerInAnyGameRoomStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM gameRoomPlayers grp
		LEFT JOIN gameRooms gr ON gr.roomID = grp.roomID
		WHERE accountID = ? AND status >= 1 AND status <= 3
	)");

	isAccountIDExistsStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM accounts
		WHERE accountID = ?
	)");

	isAccountNameExistsStatement = database->prepare(R"(
		SELECT COUNT(displayName)
		FROM accounts
		WHERE displayName = ? COLLATE NOCASE
	)");
}

LobbyDatabase::~LobbyDatabase() = default;

LobbyDatabase::LobbyDatabase(const boost::filesystem::path & databasePath)
{
	// how long to wait before aborting with "SQLite Busy" error code
	// can happen for example due to accessing same database via sqlite3 command line interface
	static constexpr int sqliteBusyTimeout = 5000;

	database = SQLiteInstance::open(databasePath, true);
	database->setBusyTimeout(sqliteBusyTimeout);
	clearOldData();
	prepareStatements();
}

void LobbyDatabase::insertChatMessage(const std::string & sender, const std::string & channelType, const std::string & channelName, const std::string & messageText)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertChatMessageStatement->executeOnce(sender, messageText, channelType, channelName);
}

bool LobbyDatabase::isPlayerInGameRoom(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	bool result = false;

	isPlayerInAnyGameRoomStatement->setBinds(accountID);
	if(isPlayerInAnyGameRoomStatement->execute())
		isPlayerInAnyGameRoomStatement->getColumns(result);
	isPlayerInAnyGameRoomStatement->reset();

	return result;
}

bool LobbyDatabase::isPlayerInGameRoom(const std::string & accountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	bool result = false;

	isPlayerInGameRoomStatement->setBinds(accountID, roomID);
	if(isPlayerInGameRoomStatement->execute())
		isPlayerInGameRoomStatement->getColumns(result);
	isPlayerInGameRoomStatement->reset();

	return result;
}

std::vector<LobbyChatMessage> LobbyDatabase::getRecentMessageHistory(const std::string & channelType, const std::string & channelName)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyChatMessage> result;

	getRecentMessageHistoryStatement->setBinds(channelType, channelName);
	while(getRecentMessageHistoryStatement->execute())
	{
		LobbyChatMessage message;
		getRecentMessageHistoryStatement->getColumns(message.accountID, message.displayName, message.messageText, message.age);
		result.push_back(message);
	}
	getRecentMessageHistoryStatement->reset();

	return result;
}

std::vector<LobbyChatMessage> LobbyDatabase::getFullMessageHistory(const std::string & channelType, const std::string & channelName)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyChatMessage> result;

	getFullMessageHistoryStatement->setBinds(channelType, channelName);
	while(getFullMessageHistoryStatement->execute())
	{
		LobbyChatMessage message;
		getFullMessageHistoryStatement->getColumns(message.accountID, message.displayName, message.messageText, message.age);
		result.push_back(message);
	}
	getFullMessageHistoryStatement->reset();

	return result;
}

void LobbyDatabase::setAccountOnline(const std::string & accountID, bool isOnline)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	setAccountOnlineStatement->executeOnce(isOnline ? 1 : 0, accountID);
}

void LobbyDatabase::setGameRoomStatus(const std::string & roomID, LobbyRoomState roomStatus)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	setGameRoomStatusStatement->executeOnce(vstd::to_underlying(roomStatus), roomID);
}

void LobbyDatabase::insertPlayerIntoGameRoom(const std::string & accountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertGameRoomPlayersStatement->executeOnce(roomID, accountID);
}

void LobbyDatabase::deletePlayerFromGameRoom(const std::string & accountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	deleteGameRoomPlayersStatement->executeOnce(roomID, accountID);
}

void LobbyDatabase::deleteGameRoomInvite(const std::string & targetAccountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	deleteGameRoomInvitesStatement->executeOnce(roomID, targetAccountID);
}

void LobbyDatabase::insertGameRoomInvite(const std::string & targetAccountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertGameRoomInvitesStatement->executeOnce(roomID, targetAccountID);
}

void LobbyDatabase::insertGameRoom(const std::string & roomID, const std::string & hostAccountID, const std::string & serverVersion)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertGameRoomStatement->executeOnce(roomID, hostAccountID, serverVersion);
}

void LobbyDatabase::insertGameRoomMod(const std::string & roomID, const std::string & modID, const std::string & modName, const std::string & modVersion)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertGameRoomModStatement->executeOnce(roomID, modID, modName, modVersion);
}

void LobbyDatabase::insertAccount(const std::string & accountID, const std::string & displayName)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertAccountStatement->executeOnce(accountID, displayName);
}

void LobbyDatabase::insertAccessCookie(const std::string & accountID, const std::string & accessCookieUUID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	insertAccessCookieStatement->executeOnce(accountID, accessCookieUUID);
}

void LobbyDatabase::updateAccountLoginTime(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	updateAccountLoginTimeStatement->executeOnce(accountID);
}

void LobbyDatabase::updateRoomPlayerLimit(const std::string & gameRoomID, int playerLimit)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	updateRoomPlayerLimitStatement->executeOnce(playerLimit, gameRoomID);
}

void LobbyDatabase::updateRoomDescription(const std::string & gameRoomID, const std::string & description)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	updateRoomDescriptionStatement->executeOnce(description, gameRoomID);
}

std::string LobbyDatabase::getAccountDisplayName(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::string result;

	getAccountDisplayNameStatement->setBinds(accountID);
	if(getAccountDisplayNameStatement->execute())
		getAccountDisplayNameStatement->getColumns(result);
	getAccountDisplayNameStatement->reset();

	return result;
}

LobbyDatabase::ActiveAccountsCounts LobbyDatabase::getActiveAccountsCounts()
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	ActiveAccountsCounts result{};

	getActiveAccountsCountsBatchStatement->reset();

	if(getActiveAccountsCountsBatchStatement->execute())
		getActiveAccountsCountsBatchStatement->getColumns(result.h1, result.h24, result.h168, result.h720, result.h8760);

	getActiveAccountsCountsBatchStatement->reset();

	return result;
}

LobbyDatabase::RegisteredAccountsCounts LobbyDatabase::getRegisteredAccountsCounts()
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	RegisteredAccountsCounts result{};

	getRegisteredAccountsCountsBatchStatement->reset();

	if(getRegisteredAccountsCountsBatchStatement->execute())
		getRegisteredAccountsCountsBatchStatement->getColumns(result.total, result.h24, result.h168, result.h720, result.h8760);

	getRegisteredAccountsCountsBatchStatement->reset();

	return result;
}

LobbyDatabase::ClosedGameRoomsCounts LobbyDatabase::getClosedGameRoomsCounts()
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	ClosedGameRoomsCounts result{};

	getClosedGameRoomsCountsBatchStatement->reset();

	if(getClosedGameRoomsCountsBatchStatement->execute())
		getClosedGameRoomsCountsBatchStatement->getColumns(result.total, result.h24, result.h168, result.h720, result.h8760);

	getClosedGameRoomsCountsBatchStatement->reset();

	return result;
}

LobbyCookieStatus LobbyDatabase::getAccountCookieStatus(const std::string & accountID, const std::string & accessCookieUUID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	bool result = false;

	isAccountCookieValidStatement->setBinds(accountID, accessCookieUUID);
	if(isAccountCookieValidStatement->execute())
		isAccountCookieValidStatement->getColumns(result);
	isAccountCookieValidStatement->reset();

	return result ? LobbyCookieStatus::VALID : LobbyCookieStatus::INVALID;
}

LobbyInviteStatus LobbyDatabase::getAccountInviteStatus(const std::string & accountID, const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	int result = 0;

	getAccountInviteStatusStatement->setBinds(accountID, roomID);
	if(getAccountInviteStatusStatement->execute())
		getAccountInviteStatusStatement->getColumns(result);
	getAccountInviteStatusStatement->reset();

	if (result > 0)
		return LobbyInviteStatus::INVITED;
	else
		return LobbyInviteStatus::NOT_INVITED;
}

LobbyRoomState LobbyDatabase::getGameRoomStatus(const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	LobbyRoomState result;

	getGameRoomStatusStatement->setBinds(roomID);
	if(getGameRoomStatusStatement->execute())
		getGameRoomStatusStatement->getColumns(result);
	else
		result = LobbyRoomState::CLOSED;

	getGameRoomStatusStatement->reset();
	return result;
}

uint32_t LobbyDatabase::getGameRoomFreeSlots(const std::string & roomID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	uint32_t usedSlots = 0;
	uint32_t totalSlots = 0;

	countRoomUsedSlotsStatement->setBinds(roomID);
	if(countRoomUsedSlotsStatement->execute())
		countRoomUsedSlotsStatement->getColumns(usedSlots);
	countRoomUsedSlotsStatement->reset();

	countRoomTotalSlotsStatement->setBinds(roomID);
	if(countRoomTotalSlotsStatement->execute())
		countRoomTotalSlotsStatement->getColumns(totalSlots);
	countRoomTotalSlotsStatement->reset();


	if (totalSlots > usedSlots)
		return totalSlots - usedSlots;
	return 0;
}

bool LobbyDatabase::isAccountNameExists(const std::string & displayName)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	bool result = false;

	isAccountNameExistsStatement->setBinds(displayName);
	if(isAccountNameExistsStatement->execute())
		isAccountNameExistsStatement->getColumns(result);
	isAccountNameExistsStatement->reset();
	return result;
}

bool LobbyDatabase::isAccountIDExists(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	bool result = false;

	isAccountIDExistsStatement->setBinds(accountID);
	if(isAccountIDExistsStatement->execute())
		isAccountIDExistsStatement->getColumns(result);
	isAccountIDExistsStatement->reset();
	return result;
}

std::vector<LobbyGameRoom> LobbyDatabase::getActiveGameRooms()
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyGameRoom> result;

	while(getActiveGameRoomsStatement->execute())
	{
		LobbyGameRoom entry;
		getActiveGameRoomsStatement->getColumns(entry.roomID, entry.hostAccountID, entry.hostAccountDisplayName, entry.description, entry.roomState, entry.playerLimit, entry.version, entry.age);
		entry.participants = getGameRoomPlayers(entry.roomID);
		entry.invited = getGameRoomInvites(entry.roomID);
		entry.mods = getGameRoomMods(entry.roomID);
		result.push_back(entry);
	}
	getActiveGameRoomsStatement->reset();

	return result;
}

LobbyGameRoom LobbyDatabase::getGameRoom(const std::string & roomID)
{
	LobbyGameRoom entry;

	entry.roomID = roomID;

	getGameRoomStatement->setBinds(roomID);
	if(getGameRoomStatement->execute())
		getGameRoomStatement->getColumns(entry.hostAccountID, entry.hostAccountDisplayName, entry.description, entry.roomState, entry.playerLimit, entry.version, entry.age);

	getGameRoomStatement->reset();

	entry.participants = getGameRoomPlayers(entry.roomID);
	entry.invited = getGameRoomInvites(entry.roomID);
	entry.mods = getGameRoomMods(entry.roomID);

	return entry;
}

std::vector<LobbyAccount> LobbyDatabase::getGameRoomPlayers(const std::string & room)
{
	std::vector<LobbyAccount> result;

	getGameRoomPlayersStatement->setBinds(room);
	while(getGameRoomPlayersStatement->execute())
	{
		LobbyAccount account;
		getGameRoomPlayersStatement->getColumns(account.accountID, account.displayName);
		result.push_back(account);
	}
	getGameRoomPlayersStatement->reset();

	return result;
}

std::vector<LobbyAccount> LobbyDatabase::getGameRoomInvites(const std::string & room)
{
	std::vector<LobbyAccount> result;

	getGameRoomInvitesStatement->setBinds(room);
	while(getGameRoomInvitesStatement->execute())
	{
		LobbyAccount account;
		getGameRoomInvitesStatement->getColumns(account.accountID, account.displayName);
		result.push_back(account);
	}
	getGameRoomInvitesStatement->reset();

	return result;
}

std::vector<LobbyGameRoomMod> LobbyDatabase::getGameRoomMods(const std::string & room)
{
	std::vector<LobbyGameRoomMod> result;

	getGameRoomModsStatement->setBinds(room);
	while(getGameRoomModsStatement->execute())
	{
		LobbyGameRoomMod mod;
		getGameRoomModsStatement->getColumns(mod.ID, mod.name, mod.version);
		result.push_back(mod);
	}
	getGameRoomModsStatement->reset();

	return result;
}

std::vector<LobbyGameRoom> LobbyDatabase::getAccountGameHistory(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyGameRoom> result;

	getAccountGameHistoryStatement->setBinds(accountID);
	while(getAccountGameHistoryStatement->execute())
	{
		LobbyGameRoom entry;
		getAccountGameHistoryStatement->getColumns(entry.roomID, entry.hostAccountID, entry.hostAccountDisplayName, entry.description, entry.roomState, entry.playerLimit, entry.age);
		result.push_back(entry);
	}
	getAccountGameHistoryStatement->reset();

	for (auto & room : result)
		room.participants = getGameRoomPlayers(room.roomID);

	return result;
}

std::vector<LobbyAccount> LobbyDatabase::getActiveAccounts()
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyAccount> result;

	while(getActiveAccountsStatement->execute())
	{
		LobbyAccount entry;
		getActiveAccountsStatement->getColumns(entry.accountID, entry.displayName);
		result.push_back(entry);
	}
	getActiveAccountsStatement->reset();
	return result;
}

std::vector<LobbyGameRoom> LobbyDatabase::getRooms(int hours, int limit)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::vector<LobbyGameRoom> result;

	getRoomsStatement->reset();
	getRoomsStatement->setBinds(hours, hours, limit);

	while(getRoomsStatement->execute())
	{
		LobbyGameRoom entry;
		std::string hostAccountDisplayName;
		int64_t secondsElapsed;
		getRoomsStatement->getColumns(entry.roomID, entry.hostAccountID, hostAccountDisplayName, entry.description, entry.roomState, entry.playerLimit, entry.version, secondsElapsed);
		entry.age = std::chrono::seconds(secondsElapsed);
		
		LobbyAccount hostAccount;
		hostAccount.accountID = entry.hostAccountID;
		hostAccount.displayName = hostAccountDisplayName;
		entry.participants.push_back(hostAccount);
		
		result.push_back(entry);
	}
	getRoomsStatement->reset();

	for (auto & room : result)
		room.participants = getGameRoomPlayers(room.roomID);

	for (auto & room : result)
		room.mods = getGameRoomMods(room.roomID);

	return result;
}

std::string LobbyDatabase::getIdleGameRoom(const std::string & hostAccountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::string result;

	getIdleGameRoomStatement->setBinds(hostAccountID);
	if(getIdleGameRoomStatement->execute())
		getIdleGameRoomStatement->getColumns(result);

	getIdleGameRoomStatement->reset();
	return result;
}

std::string LobbyDatabase::getAccountGameRoom(const std::string & accountID)
{
	TimeGuard timeGuard(this->timeTracker, __FUNCTION__);
	std::string result;

	getAccountGameRoomStatement->setBinds(accountID);
	if(getAccountGameRoomStatement->execute())
		getAccountGameRoomStatement->getColumns(result);

	getAccountGameRoomStatement->reset();
	return result;
}

void LobbyDatabase::printPerformanceStatistics()
{
	timeTracker.dumpToLog();
	database->printMemoryStats();
}
