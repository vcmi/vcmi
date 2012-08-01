#pragma once



class CMapHeader;
class CCampaignHeader;
struct StartInfo;

/// A class which stores the count of human players and all players, the filename,
/// scenario options, the map header information,...
class DLL_LINKAGE CMapInfo
{
public:
	CMapHeader * mapHeader; //may be NULL if campaign
	CCampaignHeader * campaignHeader; //may be NULL if scenario
	StartInfo *scenarioOpts; //options with which scenario has been started (used only with saved games)
	std::string fileURI;
	std::string date;
	int playerAmnt, //players in map
		humanPlayers; //players ALLOWED to be controlled by human
	int actualHumanPlayers; // >1 if multiplayer game
	CMapInfo(bool map = true);
	~CMapInfo();
	//CMapInfo(const std::string &fname, const ui8 *map);
	void setHeader(CMapHeader *header);
	void mapInit(const std::string &fname, const ui8 *map);
	void campaignInit();
	void countPlayers();

	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & mapHeader & campaignHeader & scenarioOpts & fileURI & date & playerAmnt & humanPlayers;
		h & actualHumanPlayers;
	}
};
