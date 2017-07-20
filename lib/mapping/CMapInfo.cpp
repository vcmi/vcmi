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
			if(i->second.playerID != PlayerSettings::PLAYER_AI)
				actualHumanPlayers++;
}

CMapInfo::CMapInfo() : scenarioOpts(nullptr), playerAmnt(0), humanPlayers(0),
	actualHumanPlayers(0), isRandomMap(false)
{

}

#define STEAL(x) x = std::move(tmp.x)

CMapInfo::CMapInfo(CMapInfo && tmp):
	scenarioOpts(nullptr), playerAmnt(0), humanPlayers(0),
	actualHumanPlayers(0), isRandomMap(false)
{
	std::swap(scenarioOpts, tmp.scenarioOpts);
	STEAL(mapHeader);
	STEAL(campaignHeader);
	STEAL(fileURI);
	STEAL(date);
	STEAL(playerAmnt);
	STEAL(humanPlayers);
	STEAL(actualHumanPlayers);
	STEAL(isRandomMap);
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

CMapInfo & CMapInfo::operator=(CMapInfo &&tmp)
{
	STEAL(mapHeader);
	STEAL(campaignHeader);
	STEAL(scenarioOpts);
	STEAL(fileURI);
	STEAL(date);
	STEAL(playerAmnt);
	STEAL(humanPlayers);
	STEAL(actualHumanPlayers);
	STEAL(isRandomMap);
	return *this;
}

#undef STEAL
