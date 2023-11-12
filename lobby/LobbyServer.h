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

class SQLiteInstance;

class LobbyServer : public NetworkServer
{
	std::unique_ptr<SQLiteInstance> database;

	void onNewConnection(const std::shared_ptr<NetworkConnection> &) override;
	void onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message) override;
public:
	LobbyServer();
};
