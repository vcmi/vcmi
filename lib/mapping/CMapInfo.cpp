#include "StdInc.h"
#include "CMapInfo.h"

#include "../StartInfo.h"
#include "CMap.h"
#include "CCampaignHandler.h"
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

CMapInfo::CMapInfo(CMapInfo && tmp)
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
}


void CMapInfo::mapInit(const std::string & fname)
{
	fileURI = fname;
	mapHeader = CMapService::loadMapHeader(fname);
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

