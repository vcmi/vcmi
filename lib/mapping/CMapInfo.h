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
class CMapGenOptions;
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
	/// Builds header of a map that is generated from these options on game start
	void randomMapInit(const CMapGenOptions & options);
	
	std::string getNameTranslated(const ITranslator * translator) const;
	std::string getDescriptionTranslated(const ITranslator * translator) const;
	int getMapSizeIconId() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & mapHeader;
		h & campaign;
		h & scenarioOptionsOfSave;
		h & fileURI;
		h & date;
		h & isRandomMap;
	}
};
