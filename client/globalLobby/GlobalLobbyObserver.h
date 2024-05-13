/*
 * GlobalLobbyObserver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct GlobalLobbyAccount;
struct GlobalLobbyRoom;

/// Interface for windows that want to receive updates whenever state of global lobby changes
class GlobalLobbyObserver
{
public:
	virtual void onActiveAccounts(const std::vector<GlobalLobbyAccount> & accounts) {}
	virtual void onActiveGameRooms(const std::vector<GlobalLobbyRoom> & rooms) {}

	virtual ~GlobalLobbyObserver() = default;
};
