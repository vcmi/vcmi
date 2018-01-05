/*
 * CMapInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapInfo.h"

#include "../filesystem/ResourceID.h"
#include "../StartInfo.h"
#include "../GameConstants.h"
#include "CMapService.h"

CMapInfo::CMapInfo() : scenarioOpts(nullptr), playerAmnt(0), humanPlayers(0),
	actualHumanPlayers(0), isRandomMap(false), isSaveGame(false)
{

}

CMapInfo::~CMapInfo()
{
	vstd::clear_pointer(scenarioOpts);
}

void CMapInfo::mapInit(const std::string & fname)
{
	fileURI = fname;
	CMapService mapService;
	mapHeader = mapService.loadMapHeader(ResourceID(fname, EResType::MAP));
	countPlayers();
}

void CMapInfo::campaignInit()
{
	campaignHeader = std::unique_ptr<CCampaignHeader>(new CCampaignHeader(CCampaignHandler::getHeader(fileURI)));
}

void CMapInfo::countPlayers()
{
	actualHumanPlayers = playerAmnt = humanPlayers = 0;
	for(int i=0; i<PlayerColor::PLAYER_LIMIT_I; i++)
	{
		if(mapHeader->players[i].canHumanPlay)
		{
			playerAmnt++;
			humanPlayers++;
		}
		else if(mapHeader->players[i].canComputerPlay)
		{
			playerAmnt++;
		}
	}

	if(scenarioOpts)
		for (auto i = scenarioOpts->playerInfos.cbegin(); i != scenarioOpts->playerInfos.cend(); i++)
			if(i->second.isControlledByHuman())
				actualHumanPlayers++;
}

std::string CMapInfo::getName() const
{
	std::string name;
	if(campaignHeader)
		name = campaignHeader->name;
	else
		name = mapHeader->name;

	return name.length() ? name : "Unnamed";
}

std::string CMapInfo::getNameForList() const
{
	if(isSaveGame)
	{
		// TODO: this could be handled differently
		std::vector<std::string> path;
		boost::split(path, fileURI, boost::is_any_of("\\/"));
		return path[path.size()-1];
	}
	else
	{
		return getName();
	}
}

std::string CMapInfo::getDescription() const
{
	if(campaignHeader)
		return campaignHeader->description;
	else
		return mapHeader->description;
}
