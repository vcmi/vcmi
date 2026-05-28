/*
 * LobbyDefines.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LobbyDefines.h"

#include "../lib/json/JsonNode.h"

JsonNode LobbyAccount::toJson() const
{
	JsonNode jsonEntry;
	jsonEntry["accountID"].String() = accountID;
	jsonEntry["displayName"].String() = displayName;
	return jsonEntry;
}

JsonNode LobbyGameRoomMod::toJson() const
{
	JsonNode modJson;
	modJson["modId"].String() = ID;
	modJson["name"].String() = name;
	modJson["version"].String() = version;
	if (ID.find('.') != std::string::npos)
		modJson["parent"].String() = ID.substr(ID.find('.'));

	return modJson;
}

JsonNode LobbyGameRoom::toJsonShort() const
{
	static constexpr std::array LOBBY_ROOM_STATE_NAMES = {
		"idle",
		"public",
		"private",
		"busy",
		"cancelled",
		"closed"
	};

	JsonNode jsonEntry;
	jsonEntry["description"].String() = description;
	jsonEntry["version"].String() = version;
	jsonEntry["status"].String() = LOBBY_ROOM_STATE_NAMES[vstd::to_underlying(roomState)];
	jsonEntry["playerLimit"].Integer() = playerLimit;
	jsonEntry["ageSeconds"].Integer() = age.count();
	for(const auto & mod : mods)
		jsonEntry["mods"].Vector().push_back(mod.toJson());

	return jsonEntry;
}

JsonNode LobbyGameRoom::toJsonFull() const
{
	JsonNode jsonEntry = toJsonShort();
	jsonEntry["gameRoomID"].String() = roomID;
	jsonEntry["hostAccountID"].String() = hostAccountID;
	jsonEntry["hostAccountDisplayName"].String() = hostAccountDisplayName;

	for(const auto & account : participants)
		jsonEntry["participants"].Vector().push_back(account.toJson());

	for(const auto & account : invited)
		jsonEntry["invited"].Vector().push_back(account.toJson());

	return jsonEntry;
}
