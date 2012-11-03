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
    for(int i=0;i<GameConstants::PLAYER_LIMIT;i++)
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
            if(i->second.human)
                actualHumanPlayers++;
}

CMapInfo::CMapInfo() : mapHeader(nullptr), campaignHeader(nullptr),
    scenarioOpts(nullptr)
{

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

