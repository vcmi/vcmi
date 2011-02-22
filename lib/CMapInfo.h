#pragma once

#include "../global.h"

class CMapHeader;
class CCampaignHeader;
struct StartInfo;

/// A class which stores the count of human players and all players, the filename,
/// scenario options, the map header information,...
class DLL_EXPORT CMapInfo
{
public:
	CMapHeader * mapHeader; //may be NULL if campaign
	CCampaignHeader * campaignHeader; //may be NULL if scenario
	StartInfo *scenarioOpts; //options with which scenario has been started (used only with saved games)
	std::string filename;
	bool lodCmpgn; //tells if this campaign is located in Lod file
	std::string date;
	int playerAmnt, //players in map
		humenPlayers; //players ALLOWED to be controlled by human
	int actualHumanPlayers; // >1 if multiplayer game
	CMapInfo(bool map = true);
	~CMapInfo();
	//CMapInfo(const std::string &fname, const unsigned char *map);
	void setHeader(CMapHeader *header);
	void mapInit(const std::string &fname, const unsigned char *map);
	void campaignInit();
	void countPlayers();

	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & mapHeader & campaignHeader & scenarioOpts & filename & lodCmpgn & date & playerAmnt & humenPlayers;
		h & actualHumanPlayers;
	}
};
