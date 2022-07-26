/*
 * CMapInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
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

VCMI_LIB_NAMESPACE_BEGIN

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
	StartInfo * scenarioOptionsOfSave; // Options with which scenario has been started (used only with saved games)
	std::string fileURI;
	std::string date;
	int amountOfPlayersOnMap;
	int amountOfHumanControllablePlayers;
	int amountOfHumanPlayersInSave;
	bool isRandomMap;

	CMapInfo();
	virtual ~CMapInfo();

	CMapInfo &operator=(CMapInfo &&other);

	void mapInit(const std::string & fname);
	void saveInit(ResourceID file);
	void campaignInit();
	void countPlayers();
	// TODO: Those must be on client-side
	std::string getName() const;
	std::string getNameForList() const;
	std::string getDescription() const;
	int getMapSizeIconId() const;
	std::pair<int, int> getMapSizeFormatIconId() const;
	std::string getMapSizeName() const;

	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & mapHeader;
		h & campaignHeader;
		h & scenarioOptionsOfSave;
		h & fileURI;
		h & date;
		h & amountOfPlayersOnMap;
		h & amountOfHumanControllablePlayers;
		h & amountOfHumanPlayersInSave;
		h & isRandomMap;
	}
};

VCMI_LIB_NAMESPACE_END
