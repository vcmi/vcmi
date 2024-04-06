/*
 * LobbyDefines.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

enum class LobbyCookieStatus : int32_t
{
	INVALID,
	VALID
};

enum class LobbyInviteStatus : int32_t
{
	NOT_INVITED,
	INVITED,
	DECLINED
};

enum class LobbyRoomState : int32_t
{
	IDLE = 0, // server is ready but no players are in the room
	PUBLIC = 1, // host has joined and allows anybody to join
	PRIVATE = 2, // host has joined but only allows those he invited to join
	BUSY = 3, // match is ongoing and no longer accepts players
	CANCELLED = 4, // game room was cancelled without starting the game
	CLOSED = 5, // game room was closed after playing for some time
};

struct LobbyAccount
{
	std::string accountID;
	std::string displayName;
};

struct LobbyGameRoom
{
	std::string roomID;
	std::string hostAccountID;
	std::string hostAccountDisplayName;
	std::string description;
	std::vector<LobbyAccount> participants;
	LobbyRoomState roomState;
	uint32_t playerLimit;
	std::chrono::seconds age;
};

struct LobbyChatMessage
{
	std::string accountID;
	std::string displayName;
	std::string messageText;
	std::chrono::seconds age;
};
