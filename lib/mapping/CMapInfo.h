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

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;

class CMapHeader;
class Campaign;
class ResourceID;

/**
 * A class which stores the count of human players and all players, the filename,
 * scenario options, the map header information,...
 */
class DLL_LINKAGE CMapInfo
{
public:
	std::unique_ptr<CMapHeader> mapHeader; //may be nullptr if campaign
	std::unique_ptr<Campaign> campaign; //may be nullptr if scenario
	StartInfo * scenarioOptionsOfSave; // Options with which scenario has been started (used only with saved games)
	std::string fileURI;
	std::string fullFileURI;
	std::string date;
	int amountOfPlayersOnMap;
	int amountOfHumanControllablePlayers;
	int amountOfHumanPlayersInSave;
	bool isRandomMap;

	CMapInfo();
	virtual ~CMapInfo();

	CMapInfo(CMapInfo &&other) = delete;
	CMapInfo(const CMapInfo &other) = delete;

	CMapInfo &operator=(CMapInfo &&other) = delete;
	CMapInfo &operator=(const CMapInfo &other) = delete;

	void mapInit(const std::string & fname);
	void saveInit(const ResourceID & file);
	void campaignInit();
	void countPlayers();
	// TODO: Those must be on client-side
	std::string getName() const;
	std::string getNameForList() const;
	std::string getDescription() const;
	int getMapSizeIconId() const;
	int getMapSizeFormatIconId() const;
	std::string getMapSizeName() const;

	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & mapHeader;
		h & campaign;
		h & scenarioOptionsOfSave;
		h & fileURI;
		h & fullFileURI;
		h & date;
		h & amountOfPlayersOnMap;
		h & amountOfHumanControllablePlayers;
		h & amountOfHumanPlayersInSave;
		h & isRandomMap;
	}
};

VCMI_LIB_NAMESPACE_END
