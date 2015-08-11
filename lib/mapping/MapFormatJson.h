/*
 * MapFormatJSON.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapService.h"
#include "../JsonNode.h"

#include "../filesystem/CZipSaver.h"
#include "../filesystem/CZipLoader.h"

class TriggeredEvent;


class DLL_LINKAGE CMapFormatJson
{
public:	
	static const std::string HEADER_FILE_NAME;

protected:
	
	/** ptr to the map object which gets filled by data from the buffer or written to buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer or written to buffer.
	 * (when loading map and mapHeader point to the same object)
	 */
	std::unique_ptr<CMapHeader> mapHeader;	
	
	/**
	 * Reads triggered events, including victory/loss conditions
	 */
	void readTriggeredEvents(const JsonNode & input);

	/**
	 * Reads one of triggered events
	 */
	void readTriggeredEvent(TriggeredEvent & event, const JsonNode & source);		
};

class DLL_LINKAGE CMapPatcher : public CMapFormatJson, public IMapPatcher
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream. A stream containing the map data.
	 */
	CMapPatcher(JsonNode stream);
		
public: //IMapPatcher
	/**
	 * Modifies supplied map header using Json data
	 *
	 */
	void patchMapHeader(std::unique_ptr<CMapHeader> & header) override;
	
private:
	/**
	 * Reads subset of header that can be replaced by patching.
	 */
	void readPatchData();


	const JsonNode input;	
};

class DLL_LINKAGE CMapLoaderJson : public CMapFormatJson, public IMapLoader
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream containing the map data
	 */
	CMapLoaderJson(CInputStream * stream);

	/**
	 * Loads the VCMI/Json map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	std::unique_ptr<CMap> loadMap() override;

	/**
	 * Loads the VCMI/Json map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader() override;

private:
	/**
	 * Reads complete map.
	 */
	void readMap();

	/**
	 * Reads the map header.
	 */
	void readHeader();

	/**
	 * Reads player information.
	 */
	void readPlayerInfo();


	CInputStream * input;
};

class DLL_LINKAGE CMapSaverJson : public CMapFormatJson, public IMapSaver
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream to save the map to
	 */
	CMapSaverJson(CInputOutputStream * stream);	
	
	~CMapSaverJson();
	
	/**
	 * Actually saves the VCMI/Json map into stream.
	 *
	 */	
	void saveMap(const std::unique_ptr<CMap> & map) override;	
private:
	void saveHeader();
	
	CInputOutputStream * output;
	std::shared_ptr<CIOApi> ioApi;		
	CZipSaver saver;	
};
