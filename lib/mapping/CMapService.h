/*
 * CMapService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class ResourceID;

class CMap;
class CMapHeader;
class CInputStream;

class IMapLoader;
class IMapPatcher;

class ModCompatibilityInfo;

/**
 * The map service provides loading and saving of VCMI/H3 map files.
 */
class DLL_LINKAGE CMapService
{
public:
	CMapService() = default;
	virtual ~CMapService() = default;

	/**
	 * Loads the VCMI/H3 map file specified by the name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the loaded map class
	 */
	std::unique_ptr<CMap> loadMap(const ResourceID & name) const;
	
	/**
	 * Loads the VCMI/H3 map header specified by the name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader(const ResourceID & name) const;
	
	/**
	 * Loads the VCMI/H3 map file from a buffer. This method is temporarily
	 * in use to ease the transition to use the new map service.
	 *
	 * TODO Replace method params with a CampaignMapInfo struct which contains
	 * a campaign loading object + name of map.
	 *
	 * @param buffer a pointer to a buffer containing the map data
	 * @param size the size of the buffer
	 * @param name indicates name of file that will be used during map header patching
	 * @return a unique ptr to the loaded map class
	 */
	std::unique_ptr<CMap> loadMap(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const;
	
	/**
	 * Loads the VCMI/H3 map header from a buffer. This method is temporarily
	 * in use to ease the transition to use the new map service.
	 *
	 * TODO Replace method params with a CampaignMapInfo struct which contains
	 * a campaign loading object + name of map.
	 *
	 * @param buffer a pointer to a buffer containing the map header data
	 * @param size the size of the buffer
	 * @param name indicates name of file that will be used during map header patching
	 * @return a unique ptr to the loaded map class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const;
	
	/**
	 * Tests if mods used in the map are currently loaded
	 * @param map const reference to map header
	 * @return data structure representing missing or incompatible mods (those which are needed from map but not loaded)
	 */
	static ModCompatibilityInfo verifyMapHeaderMods(const CMapHeader & map);
	
	void saveMap(const std::unique_ptr<CMap> & map, boost::filesystem::path fullPath) const;
	
private:
	/**
	 * Gets a map input stream object specified by a map name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the input stream class
	 */
	static std::unique_ptr<CInputStream> getStreamFromFS(const ResourceID & name);

	/**
	 * Gets a map input stream from a buffer.
	 *
	 * @param buffer a pointer to a buffer containing the map data
	 * @param size the size of the buffer
	 * @return a unique ptr to the input stream class
	 */
	static std::unique_ptr<CInputStream> getStreamFromMem(const ui8 * buffer, int size);

	/**
	 * Gets a map loader from the given stream. It performs checks to test
	 * in which map format the map is.
	 *
	 * @param stream the input map stream
	 * @return the constructed map loader
	 */
	static std::unique_ptr<IMapLoader> getMapLoader(std::unique_ptr<CInputStream> & stream, std::string mapName, std::string modName, std::string encoding);

	/**
	 * Gets a map patcher for specified scenario
	 *
	 * @param scenarioName for patcher
	 * @return the constructed map patcher
	 */
	static std::unique_ptr<IMapPatcher> getMapPatcher(std::string scenarioName);
};

/**
 * Interface for loading a map.
 */
class DLL_LINKAGE IMapLoader
{
public:
	/**
	 * Loads the VCMI/H3 map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	virtual std::unique_ptr<CMap> loadMap() = 0;

	/**
	 * Loads the VCMI/H3 map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	virtual std::unique_ptr<CMapHeader> loadMapHeader() = 0;

	virtual ~IMapLoader(){}
};

class DLL_LINKAGE IMapPatcher
{
public:
	/**
	 * Modifies supplied map header using Json data
	 *
	 */
	virtual void patchMapHeader(std::unique_ptr<CMapHeader> & header) = 0;

	virtual ~IMapPatcher(){}
};

/**
 * Interface for saving a map.
 */
class DLL_LINKAGE IMapSaver
{
public:
	/**
	 * Saves the VCMI/H3 map file.
	 *
	 */
	 virtual void saveMap(const std::unique_ptr<CMap> & map) = 0;

	 virtual ~IMapSaver(){}
};

VCMI_LIB_NAMESPACE_END
