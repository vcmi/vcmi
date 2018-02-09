/*
 * StartInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StartInfo.h"

#include "rmg/CMapGenOptions.h"

#include "mapping/CMapInfo.h"

PlayerSettings::PlayerSettings()
	: bonus(RANDOM), castle(NONE), hero(RANDOM), heroPortrait(RANDOM), color(0), handicap(NO_HANDICAP), team(0), compOnly(false)
{

}

bool PlayerSettings::isControlledByAI() const
{
	return !connectedPlayerIDs.size();
}

bool PlayerSettings::isControlledByHuman() const
{
	return connectedPlayerIDs.size();
}

PlayerSettings & StartInfo::getIthPlayersSettings(PlayerColor no)
{
	if(playerInfos.find(no) != playerInfos.end())
		return playerInfos[no];
	logGlobal->error("Cannot find info about player %s. Throwing...", no.getStr());
	throw std::runtime_error("Cannot find info about player");
}
const PlayerSettings & StartInfo::getIthPlayersSettings(PlayerColor no) const
{
	return const_cast<StartInfo&>(*this).getIthPlayersSettings(no);
}

PlayerSettings * StartInfo::getPlayersSettings(const ui8 connectedPlayerId)
{
	for(auto & elem : playerInfos)
	{
		if(vstd::contains(elem.second.connectedPlayerIDs, connectedPlayerId))
			return &elem.second;
	}

	return nullptr;
}


void LobbyInfo::verifyStateBeforeStart(bool ignoreNoHuman) const
{
	if(!mi)
		throw ExceptionMapMissing();

	//there must be at least one human player before game can be started
	std::map<PlayerColor, PlayerSettings>::const_iterator i;
	for(i = si->playerInfos.cbegin(); i != si->playerInfos.cend(); i++)
		if(i->second.isControlledByHuman())
			break;

	if(i == si->playerInfos.cend() && !ignoreNoHuman)
		throw ExceptionNoHuman();

	if(si->mapGenOptions && si->mode == StartInfo::NEW_GAME)
	{
		if(!si->mapGenOptions->checkOptions())
			throw ExceptionNoTemplate();
	}
}

bool LobbyInfo::isClientHost(int clientId) const
{
	return clientId == hostClientId;
}

std::set<PlayerColor> LobbyInfo::getAllClientPlayers(int clientId) //MPTODO: this function has dupe i suppose
{
	std::set<PlayerColor> players;
	for(auto & elem : si->playerInfos)
	{
		if(isClientHost(clientId) && elem.second.isControlledByAI())
			players.insert(elem.first);

		for(ui8 id : elem.second.connectedPlayerIDs)
		{
			if(vstd::contains(getConnectedPlayerIdsForClient(clientId), id))
				players.insert(elem.first);
		}
	}
	if(isClientHost(clientId))
		players.insert(PlayerColor::NEUTRAL);

	return players;
}

std::vector<ui8> LobbyInfo::getConnectedPlayerIdsForClient(int clientId) const
{
	std::vector<ui8> ids;

	for(auto & pair : playerNames)
	{
		if(pair.second.connection == clientId)
		{
			for(auto & elem : si->playerInfos)
			{
				if(vstd::contains(elem.second.connectedPlayerIDs, pair.first))
					ids.push_back(pair.first);
			}
		}
	}
	return ids;
}

std::set<PlayerColor> LobbyInfo::clientHumanColors(int clientId)
{
	std::set<PlayerColor> players;
	for(auto & elem : si->playerInfos)
	{
		for(ui8 id : elem.second.connectedPlayerIDs)
		{
			if(vstd::contains(getConnectedPlayerIdsForClient(clientId), id))
			{
				players.insert(elem.first);
			}
		}
	}

	return players;
}


PlayerColor LobbyInfo::clientFirstColor(int clientId) const
{
	for(auto & pair : si->playerInfos)
	{
		if(isClientColor(clientId, pair.first))
			return pair.first;
	}

	return PlayerColor::CANNOT_DETERMINE;
}

bool LobbyInfo::isClientColor(int clientId, PlayerColor color) const
{
	if(si->playerInfos.find(color) != si->playerInfos.end())
	{
		for(ui8 id : si->playerInfos.find(color)->second.connectedPlayerIDs)
		{
			if(playerNames.find(id) != playerNames.end())
			{
				if(playerNames.find(id)->second.connection == clientId)
					return true;
			}
		}
	}
	return false;
}

ui8 LobbyInfo::clientFirstId(int clientId) const
{
	for(auto & pair : playerNames)
	{
		if(pair.second.connection == clientId)
			return pair.first;
	}

	return 0;
}

PlayerInfo & LobbyInfo::getPlayerInfo(int color)
{
	return mi->mapHeader->players[color];
}
