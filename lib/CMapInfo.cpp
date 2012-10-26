#include "StdInc.h"
#include "CMapInfo.h"

#include "StartInfo.h"
#include "map.h"
#include "CCampaignHandler.h"
#include "GameConstants.h"

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

CMapInfo::CMapInfo(bool map)
	: mapHeader(NULL), campaignHeader(NULL), scenarioOpts(NULL)
{
}

void CMapInfo::mapInit(const std::string &fname, const ui8 *map )
{
	fileURI = fname;
	int i = 0;
	mapHeader = new CMapHeader();
    mapHeader->version = EMapFormat::INVALID;

	try
	{
		mapHeader->initFromMemory(map, i);
		countPlayers();
	}
	catch (const std::exception &e)
	{
		tlog1 << "\t\tWarning: evil map file: " << fname << ": " << e.what() << std::endl; 
		delete mapHeader;
		mapHeader = NULL;
	}
}

CMapInfo::~CMapInfo()
{
	delete mapHeader;
	delete campaignHeader;
}

void CMapInfo::campaignInit()
{
	campaignHeader = new CCampaignHeader( CCampaignHandler::getHeader(fileURI) );
}

void CMapInfo::setHeader(CMapHeader *header)
{
	mapHeader = header;
}

