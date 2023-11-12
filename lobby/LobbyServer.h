/*
 * LobbyServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/network/NetworkServer.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class SQLiteInstance;
class SQLiteStatement;

class LobbyDatabase
{
	std::unique_ptr<SQLiteInstance> database;
	std::unique_ptr<SQLiteStatement> insertChatMessageStatement;
	std::unique_ptr<SQLiteStatement> getRecentMessageHistoryStatement;

	void initializeDatabase();
	void prepareStatements();
	void createTableChatMessages();
public:
	struct ChatMessage
	{
		std::string sender;
		std::string messageText;
		int messageAgeSeconds;
	};

	LobbyDatabase();

	void insertChatMessage(const std::string & sender, const std::string & messageText);
	std::vector<ChatMessage> getRecentMessageHistory();
};

class LobbyServer : public NetworkServer
{
	struct AccountState
	{
		std::string accountName;
	};

	std::map<std::shared_ptr<NetworkConnection>, AccountState> activeAccounts;

	std::unique_ptr<LobbyDatabase> database;

	void onNewConnection(const std::shared_ptr<NetworkConnection> &) override;
	void onConnectionLost(const std::shared_ptr<NetworkConnection> &) override;
	void onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message) override;

	void sendMessage(const std::shared_ptr<NetworkConnection> & target, const JsonNode & json);
public:
	LobbyServer();
};
