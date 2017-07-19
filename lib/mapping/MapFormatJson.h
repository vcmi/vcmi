/*
 * MapFormatJson.h, part of VCMI engine
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
#include "../GameConstants.h"

#include "../serializer/JsonSerializeFormat.h"

struct TriggeredEvent;
struct TerrainTile;
struct PlayerInfo;
class CGObjectInstance;
class AObjectTypeHandler;

class JsonSerializeFormat;
class JsonDeserializer;
class JsonSerializer;

class DLL_LINKAGE CMapFormatJson
{
public:
	static const int VERSION_MAJOR;
	static const int VERSION_MINOR;

	static const std::string HEADER_FILE_NAME;
	static const std::string OBJECTS_FILE_NAME;

	int fileVersionMajor;
	int fileVersionMinor;

protected:
	friend class MapObjectResolver;
	std::unique_ptr<IInstanceResolver> mapObjectResolver;

	/** ptr to the map object which gets filled by data from the buffer or written to buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer.
	 * (when loading map and mapHeader point to the same object)
	 */
	CMapHeader * mapHeader;

	CMapFormatJson();

	void serializeAllowedFactions(JsonSerializeFormat & handler, std::set<TFaction> & value);

	///common part of header saving/loading
	void serializeHeader(JsonSerializeFormat & handler);

	///player information saving/loading
	void serializePlayerInfo(JsonSerializeFormat & handler);

	/**
	 * Reads team settings to header
	 */
	void readTeams(JsonDeserializer & handler);

	/**
	 * Saves team settings to header
	 */
	void writeTeams(JsonSerializer & handler);

	/**
	 * Reads triggered events, including victory/loss conditions
	 */
	void readTriggeredEvents(JsonDeserializer & handler);

	/**
	 * Writes triggered events, including victory/loss conditions
	 */
	void writeTriggeredEvents(JsonSerializer & handler);

	/**
	 * Reads one of triggered events
	 */
	void readTriggeredEvent(TriggeredEvent & event, const JsonNode & source);

	/**
	 * Writes one of triggered events
	 */
	void writeTriggeredEvent(const TriggeredEvent & event, JsonNode & dest);

	void writeDisposedHeroes(JsonSerializeFormat & handler);

	void readDisposedHeroes(JsonSerializeFormat & handler);

	void serializePredefinedHeroes(JsonSerializeFormat & handler);

	void serializeRumors(JsonSerializeFormat & handler);

	///common part of map attributes saving/loading
	void serializeOptions(JsonSerializeFormat & handler);

	/**
	 * Loads map attributes except header ones
	 */
	void readOptions(JsonDeserializer & handler);

	/**
	 * Saves map attributes except header ones
	 */
	void writeOptions(JsonSerializer & handler);
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

	JsonNode input;
};

class DLL_LINKAGE CMapLoaderJson : public CMapFormatJson, public IMapLoader
{
public:
	/**
	 * Constructor.
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

	struct MapObjectLoader
	{
		MapObjectLoader(CMapLoaderJson * _owner, JsonMap::value_type & json);
		CMapLoaderJson * owner;
		CGObjectInstance * instance;
		ObjectInstanceID id;
		std::string jsonKey; //full id defined by map creator
		JsonNode & configuration;

		///constructs object (without configuration)
		void construct();

		///configures object
		void configure();
	};

	/**
	 * Reads the map header.
	 */
	void readHeader(const bool complete);

	/**
	 * Reads complete map.
	 */
	void readMap();

	void readTerrainTile(const std::string & src, TerrainTile & tile);

	void readTerrainLevel(const JsonNode & src, const int index);

	void readTerrain();

	/**
	 * Loads all map objects from zip archive
	 */
	void readObjects();

	JsonNode getFromArchive(const std::string & archiveFilename);

	CInputStream * buffer;
	std::shared_ptr<CIOApi> ioApi;

	CZipLoader loader; ///< object to handle zip archive operations
};

class DLL_LINKAGE CMapSaverJson : public CMapFormatJson, public IMapSaver
{
public:
	/**
	 * Constructor.
	 *
	 * @param stream a stream to save the map to, will contain zip archive
	 */
	CMapSaverJson(CInputOutputStream * stream);

	~CMapSaverJson();

	/**
	 * Actually saves the VCMI/Json map into stream.
	 */
	void saveMap(const std::unique_ptr<CMap> & map) override;

private:

	/**
	 * Saves @data as json file with specified @filename
	 */
	void addToArchive(const JsonNode & data, const std::string & filename);

	/**
	 * Saves header to zip archive
	 */
	void writeHeader();

	/**
	 * Encodes one tile into string
	 * @param tile tile to serialize
	 */
	const std::string writeTerrainTile(const TerrainTile & tile);

	/**
	 * Saves map level into json
	 * @param index z coordinate
	 */
	JsonNode writeTerrainLevel(const int index);

	/**
	 * Saves all terrain into zip archive
	 */
	void writeTerrain();

	/**
	 * Saves all map objects into zip archive
	 */
	void writeObjects();

	CInputOutputStream * buffer;
	std::shared_ptr<CIOApi> ioApi;
	CZipSaver saver; ///< object to handle zip archive operations
};
