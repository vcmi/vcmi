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

#include "../serializer/Serializeable.h"

struct StartInfo;

class CMapHeader;
class ITranslator;
class Campaign;
class ResourcePath;

/**
 * A class which stores the count of human players and all players, the filename,
 * scenario options, the map header information,...
 */
class DLL_LINKAGE CMapInfo : public Serializeable
{
public:
	std::unique_ptr<CMapHeader> mapHeader; //may be nullptr if campaign
	std::unique_ptr<Campaign> campaign; //may be nullptr if scenario
	std::unique_ptr<StartInfo> scenarioOptionsOfSave; // Options with which scenario has been started (used only with saved games)
	std::string fileURI;
	std::string originalFileURI; // no need to serialize
	std::time_t lastWrite; // no need to serialize
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
	void saveInit(const ResourcePath & file);
	void campaignInit();
	void countPlayers();
	
	std::string getNameTranslated(const ITranslator * translator) const;
	std::string getNameForList(const ITranslator * translator) const;
	std::string getDescriptionTranslated(const ITranslator * translator) const;
	int getMapSizeIconId() const;
	int getMapSizeFormatIconId() const;
	std::string getMapSizeName() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & mapHeader;
		h & campaign;
		h & scenarioOptionsOfSave;
		h & fileURI;
		h & date;
		h & amountOfPlayersOnMap;
		h & amountOfHumanControllablePlayers;
		h & amountOfHumanPlayersInSave;
		h & isRandomMap;
	}
};
