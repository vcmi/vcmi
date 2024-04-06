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

void LobbyDatabase::createTables()
{
	static const std::string createChatMessages = R"(
		CREATE TABLE IF NOT EXISTS chatMessages (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			senderName TEXT,
			channelType TEXT,
			channelName TEXT,
			messageText TEXT,
			creationTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
		);
	)";

	static const std::string createTableGameRooms = R"(
		CREATE TABLE IF NOT EXISTS gameRooms (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			roomID TEXT,
			hostAccountID TEXT,
			description TEXT NOT NULL DEFAULT '',
			status INTEGER NOT NULL DEFAULT 0,
			playerLimit INTEGER NOT NULL,
			creationTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
		);
	)";

	static const std::string createTableGameRoomPlayers = R"(
		CREATE TABLE IF NOT EXISTS gameRoomPlayers (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			roomID TEXT,
			accountID TEXT
		);
	)";

	static const std::string createTableAccounts = R"(
		CREATE TABLE IF NOT EXISTS accounts (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			accountID TEXT,
			displayName TEXT,
			online INTEGER NOT NULL,
			lastLoginTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
			creationTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
		);
	)";

	static const std::string createTableAccountCookies = R"(
		CREATE TABLE IF NOT EXISTS accountCookies (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			accountID TEXT,
			cookieUUID TEXT,
			creationTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
		);
	)";

	static const std::string createTableGameRoomInvites = R"(
		CREATE TABLE IF NOT EXISTS gameRoomInvites (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			roomID TEXT,
			accountID TEXT
		);
	)";

	database->prepare(createChatMessages)->execute();
	database->prepare(createTableGameRoomPlayers)->execute();
	database->prepare(createTableGameRooms)->execute();
	database->prepare(createTableAccounts)->execute();
	database->prepare(createTableAccountCookies)->execute();
	database->prepare(createTableGameRoomInvites)->execute();
}

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
		INSERT INTO gameRooms(roomID, hostAccountID, status, playerLimit) VALUES(?, ?, 0, 8);
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
		WHERE accountID = ? AND status IN (1, 2, 3)
		LIMIT 1
	)");

	getActiveAccountsStatement = database->prepare(R"(
		SELECT accountID, displayName
		FROM accounts
		WHERE online = 1
	)");

	getActiveGameRoomsStatement = database->prepare(R"(
		SELECT roomID, hostAccountID, displayName, description, status, playerLimit, strftime('%s',CURRENT_TIMESTAMP)- strftime('%s',gr.creationTime)  AS secondsElapsed
		FROM gameRooms gr
		LEFT JOIN accounts a ON gr.hostAccountID = a.accountID
		WHERE status IN (1, 2, 3)
		ORDER BY secondsElapsed ASC
	)");

	countRoomUsedSlotsStatement = database->prepare(R"(
		SELECT a.accountID, a.displayName
		FROM gameRoomPlayers grp
		LEFT JOIN accounts a ON a.accountID = grp.accountID
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
		WHERE accountID = ? AND status IN (1, 2, 3)
	)");

	isAccountIDExistsStatement = database->prepare(R"(
		SELECT COUNT(accountID)
		FROM accounts
		WHERE accountID = ?
	)");

	isAccountNameExistsStatement = database->prepare(R"(
		SELECT COUNT(displayName)
		FROM accounts
		WHERE displayName = ?
	)");
}

LobbyDatabase::~LobbyDatabase() = default;

LobbyDatabase::LobbyDatabase(const boost::filesystem::path & databasePath)
{
	database = SQLiteInstance::open(databasePath, true);
	createTables();
	clearOldData();
	prepareStatements();
}

void LobbyDatabase::insertChatMessage(const std::string & sender, const std::string & channelType, const std::string & channelName, const std::string & messageText)
{
	insertChatMessageStatement->executeOnce(sender, messageText, channelType, channelName);
}

bool LobbyDatabase::isPlayerInGameRoom(const std::string & accountID)
{
	bool result = false;

	isPlayerInAnyGameRoomStatement->setBinds(accountID);
	if(isPlayerInAnyGameRoomStatement->execute())
		isPlayerInAnyGameRoomStatement->getColumns(result);
	isPlayerInAnyGameRoomStatement->reset();

	return result;
}

bool LobbyDatabase::isPlayerInGameRoom(const std::string & accountID, const std::string & roomID)
{
	bool result = false;

	isPlayerInGameRoomStatement->setBinds(accountID, roomID);
	if(isPlayerInGameRoomStatement->execute())
		isPlayerInGameRoomStatement->getColumns(result);
	isPlayerInGameRoomStatement->reset();

	return result;
}

std::vector<LobbyChatMessage> LobbyDatabase::getRecentMessageHistory(const std::string & channelType, const std::string & channelName)
{
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
	setAccountOnlineStatement->executeOnce(isOnline ? 1 : 0, accountID);
}

void LobbyDatabase::setGameRoomStatus(const std::string & roomID, LobbyRoomState roomStatus)
{
	setGameRoomStatusStatement->executeOnce(vstd::to_underlying(roomStatus), roomID);
}

void LobbyDatabase::insertPlayerIntoGameRoom(const std::string & accountID, const std::string & roomID)
{
	insertGameRoomPlayersStatement->executeOnce(roomID, accountID);
}

void LobbyDatabase::deletePlayerFromGameRoom(const std::string & accountID, const std::string & roomID)
{
	deleteGameRoomPlayersStatement->executeOnce(roomID, accountID);
}

void LobbyDatabase::deleteGameRoomInvite(const std::string & targetAccountID, const std::string & roomID)
{
	deleteGameRoomInvitesStatement->executeOnce(roomID, targetAccountID);
}

void LobbyDatabase::insertGameRoomInvite(const std::string & targetAccountID, const std::string & roomID)
{
	insertGameRoomInvitesStatement->executeOnce(roomID, targetAccountID);
}

void LobbyDatabase::insertGameRoom(const std::string & roomID, const std::string & hostAccountID)
{
	insertGameRoomStatement->executeOnce(roomID, hostAccountID);
}

void LobbyDatabase::insertAccount(const std::string & accountID, const std::string & displayName)
{
	insertAccountStatement->executeOnce(accountID, displayName);
}

void LobbyDatabase::insertAccessCookie(const std::string & accountID, const std::string & accessCookieUUID)
{
	insertAccessCookieStatement->executeOnce(accountID, accessCookieUUID);
}

void LobbyDatabase::updateAccountLoginTime(const std::string & accountID)
{
	updateAccountLoginTimeStatement->executeOnce(accountID);
}

void LobbyDatabase::updateRoomPlayerLimit(const std::string & gameRoomID, int playerLimit)
{
	updateRoomPlayerLimitStatement->executeOnce(playerLimit, gameRoomID);
}

void LobbyDatabase::updateRoomDescription(const std::string & gameRoomID, const std::string & description)
{
	updateRoomDescriptionStatement->executeOnce(description, gameRoomID);
}

std::string LobbyDatabase::getAccountDisplayName(const std::string & accountID)
{
	std::string result;

	getAccountDisplayNameStatement->setBinds(accountID);
	if(getAccountDisplayNameStatement->execute())
		getAccountDisplayNameStatement->getColumns(result);
	getAccountDisplayNameStatement->reset();

	return result;
}

LobbyCookieStatus LobbyDatabase::getAccountCookieStatus(const std::string & accountID, const std::string & accessCookieUUID)
{
	bool result = false;

	isAccountCookieValidStatement->setBinds(accountID, accessCookieUUID);
	if(isAccountCookieValidStatement->execute())
		isAccountCookieValidStatement->getColumns(result);
	isAccountCookieValidStatement->reset();

	return result ? LobbyCookieStatus::VALID : LobbyCookieStatus::INVALID;
}

LobbyInviteStatus LobbyDatabase::getAccountInviteStatus(const std::string & accountID, const std::string & roomID)
{
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
	bool result = false;

	isAccountNameExistsStatement->setBinds(displayName);
	if(isAccountNameExistsStatement->execute())
		isAccountNameExistsStatement->getColumns(result);
	isAccountNameExistsStatement->reset();
	return result;
}

bool LobbyDatabase::isAccountIDExists(const std::string & accountID)
{
	bool result = false;

	isAccountIDExistsStatement->setBinds(accountID);
	if(isAccountIDExistsStatement->execute())
		isAccountIDExistsStatement->getColumns(result);
	isAccountIDExistsStatement->reset();
	return result;
}

std::vector<LobbyGameRoom> LobbyDatabase::getActiveGameRooms()
{
	std::vector<LobbyGameRoom> result;

	while(getActiveGameRoomsStatement->execute())
	{
		LobbyGameRoom entry;
		getActiveGameRoomsStatement->getColumns(entry.roomID, entry.hostAccountID, entry.hostAccountDisplayName, entry.description, entry.roomState, entry.playerLimit, entry.age);
		result.push_back(entry);
	}
	getActiveGameRoomsStatement->reset();

	for (auto & room : result)
	{
		countRoomUsedSlotsStatement->setBinds(room.roomID);
		while(countRoomUsedSlotsStatement->execute())
		{
			LobbyAccount account;
			countRoomUsedSlotsStatement->getColumns(account.accountID, account.displayName);
			room.participants.push_back(account);
		}
		countRoomUsedSlotsStatement->reset();
	}
	return result;
}

std::vector<LobbyGameRoom> LobbyDatabase::getAccountGameHistory(const std::string & accountID)
{
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
	{
		countRoomUsedSlotsStatement->setBinds(room.roomID);
		while(countRoomUsedSlotsStatement->execute())
		{
			LobbyAccount account;
			countRoomUsedSlotsStatement->getColumns(account.accountID, account.displayName);
			room.participants.push_back(account);
		}
		countRoomUsedSlotsStatement->reset();
	}
	return result;
}

std::vector<LobbyAccount> LobbyDatabase::getActiveAccounts()
{
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

std::string LobbyDatabase::getIdleGameRoom(const std::string & hostAccountID)
{
	std::string result;

	getIdleGameRoomStatement->setBinds(hostAccountID);
	if(getIdleGameRoomStatement->execute())
		getIdleGameRoomStatement->getColumns(result);

	getIdleGameRoomStatement->reset();
	return result;
}

std::string LobbyDatabase::getAccountGameRoom(const std::string & accountID)
{
	std::string result;

	getAccountGameRoomStatement->setBinds(accountID);
	if(getAccountGameRoomStatement->execute())
		getAccountGameRoomStatement->getColumns(result);

	getAccountGameRoomStatement->reset();
	return result;
}
