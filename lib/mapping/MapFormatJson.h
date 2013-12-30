
/*
 * MapFormatH3M.h, part of VCMI engine
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

class TriggeredEvent;

class DLL_LINKAGE CMapLoaderJson : public IMapPatcher
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream containing the map data
	 */
	CMapLoaderJson(JsonNode stream);

	/**
	 * Loads the VCMI/Json map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	std::unique_ptr<CMap> loadMap();

	/**
	 * Loads the VCMI/Json map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader();

	/**
	 * Modifies supplied map header using Json data
	 *
	 */
	void patchMapHeader(std::unique_ptr<CMapHeader> & header);

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
	 * Reads subset of header that can be replaced by patching.
	 */
	void readPatchData();

	/**
	 * Reads player information.
	 */
	void readPlayerInfo();

	/**
	 * Reads triggered events, including victory/loss conditions
	 */
	void readTriggeredEvents();

	/**
	 * Reads one of triggered events
	 */
	void readTriggeredEvent(TriggeredEvent & event, const JsonNode & source);


	/** ptr to the map object which gets filled by data from the buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer.
	 * (when loading map and mapHeader point to the same object)
	 */
	std::unique_ptr<CMapHeader> mapHeader;

	const JsonNode input;
};
