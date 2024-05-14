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

#include "../../lib/modding/ModVerificationInfo.h"

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
	std::string gameVersion;
	std::string statusID;
	std::string startDateFormatted;
	ModCompatibilityInfo modList;
	std::vector<GlobalLobbyAccount> participants;
	std::vector<GlobalLobbyAccount> invited;
	int playerLimit;
};

struct GlobalLobbyChannelMessage
{
	std::string timeFormatted;
	std::string accountID;
	std::string displayName;
	std::string messageText;
};
