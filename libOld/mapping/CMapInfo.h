#pragma once

// Forward class declarations aren't enough here. The compiler
// generated CMapInfo d-tor, generates the std::unique_ptr d-tor as well here
// as a inline method. The std::unique_ptr d-tor requires a complete type. Defining
// the CMapInfo d-tor to let the compiler add the d-tor stuff in the .cpp file
// would work with one exception. It prevents the generation of the move
// constructor which is needed. (Writing such a c-tor is nasty.) With the
// new c++11 keyword "default" for constructors this problem could be solved. But it isn't
// available for Visual Studio for now. (Empty d-tor in .cpp would be required anyway)
#include "CMap.h"
#include "CCampaignHandler.h"

struct StartInfo;

/**
 * A class which stores the count of human players and all players, the filename,
 * scenario options, the map header information,...
 */
class DLL_LINKAGE CMapInfo
{
public:
	std::unique_ptr<CMapHeader> mapHeader; //may be nullptr if campaign
	std::unique_ptr<CCampaignHeader> campaignHeader; //may be nullptr if scenario
	StartInfo * scenarioOpts; //options with which scenario has been started (used only with saved games)
	std::string fileURI;
	std::string date;
	int playerAmnt; //players in map
	int humanPlayers; //players ALLOWED to be controlled by human
	int actualHumanPlayers; // >1 if multiplayer game
	bool isRandomMap; // true if the map will be created randomly, false if not

	CMapInfo();
	CMapInfo(CMapInfo && tmp);

	CMapInfo &operator=(CMapInfo &&other);

	void mapInit(const std::string & fname);
	void campaignInit();
	void countPlayers();

	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & mapHeader & campaignHeader & scenarioOpts & fileURI & date & playerAmnt & humanPlayers;
		h & actualHumanPlayers & isRandomMap;
	}
};
