/*
 * GlobalLobbyDefines.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct GlobalLobbyAccount
{
	std::string accountID;
	std::string displayName;
	std::string status;
};

struct GlobalLobbyRoom
{
	std::string gameRoomID;
	std::string hostAccountID;
	std::string hostAccountDisplayName;
	std::string description;
	std::string statusID;
	int playersCount;
	int playerLimit;
};

struct GlobalLobbyChannelMessage
{
	std::string timeFormatted;
	std::string accountID;
	std::string displayName;
	std::string messageText;
};

struct GlobalLobbyHistoryMatch
{
	std::string gameRoomID;
	std::string startDateFormatted;
	std::vector<std::string> opponentDisplayNames;
};
