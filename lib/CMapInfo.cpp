#define VCMI_DLL

#include "CMapInfo.h"
#include "..\StartInfo.h"
#include "map.h"
#include "..\hch\CCampaignHandler.h"

void CMapInfo::countPlayers()
{
	actualHumanPlayers = playerAmnt = humenPlayers = 0;
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		if(mapHeader->players[i].canHumanPlay)
		{
			playerAmnt++;
			humenPlayers++;
		}
		else if(mapHeader->players[i].canComputerPlay)
		{
			playerAmnt++;
		}
	}

	if(scenarioOpts)
		for (std::map<int, PlayerSettings>::const_iterator i = scenarioOpts->playerInfos.begin(); i != scenarioOpts->playerInfos.end(); i++)
			if(i->second.human)
				actualHumanPlayers++;
}

CMapInfo::CMapInfo(bool map)
	: mapHeader(NULL), campaignHeader(NULL), scenarioOpts(NULL)
{
}

void CMapInfo::mapInit(const std::string &fname, const unsigned char *map )
{
	filename = fname;
	int i = 0;
	mapHeader = new CMapHeader();
	mapHeader->version = CMapHeader::invalid;

	try
	{
		mapHeader->initFromMemory(map, i);
		countPlayers();
	}
	catch (const std::string &e)
	{
		tlog1 << "\t\tWarning: evil map file: " << fname << ": " << e << std::endl; 
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
	campaignHeader = new CCampaignHeader( CCampaignHandler::getHeader(filename, lodCmpgn) );
}

void CMapInfo::setHeader(CMapHeader *header)
{
	mapHeader = header;
}

