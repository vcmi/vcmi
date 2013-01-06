
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

class CMap;
class CMapHeader;
class CInputStream;
class CGHeroInstance;
class CArtifactInstance;
class CGObjectInstance;
class CGSeerHut;
class IQuestObject;
class CGTownInstance;
class CCreatureSet;
class CInputStream;
class IMapLoader;

/**
 * The map service provides loading of VCMI/H3 map files. It can
 * be extended to save maps later as well.
 */
class DLL_LINKAGE CMapService
{
public:
	/**
	 * Loads the VCMI/H3 map file specified by the name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the loaded map class
	 */
	static std::unique_ptr<CMap> loadMap(const std::string & name);

	/**
	 * Loads the VCMI/H3 map header specified by the name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the loaded map header class
	 */
	static std::unique_ptr<CMapHeader> loadMapHeader(const std::string & name);

	/**
	 * Loads the VCMI/H3 map file from a buffer. This method is temporarily
	 * in use to ease the transition to use the new map service.
	 *
	 * TODO Replace method params with a CampaignMapInfo struct which contains
	 * a campaign loading object + name of map.
	 *
	 * @param buffer a pointer to a buffer containing the map data
	 * @param size the size of the buffer
	 * @return a unique ptr to the loaded map class
	 */
	static std::unique_ptr<CMap> loadMap(const ui8 * buffer, int size);

	/**
	 * Loads the VCMI/H3 map header from a buffer. This method is temporarily
	 * in use to ease the transition to use the new map service.
	 *
	 * TODO Replace method params with a CampaignMapInfo struct which contains
	 * a campaign loading object + name of map.
	 *
	 * @param buffer a pointer to a buffer containing the map header data
	 * @param size the size of the buffer
	 * @return a unique ptr to the loaded map class
	 */
	static std::unique_ptr<CMapHeader> loadMapHeader(const ui8 * buffer, int size);

private:
	/**
	 * Gets a map input stream object specified by a map name.
	 *
	 * @param name the name of the map
	 * @return a unique ptr to the input stream class
	 */
	static std::unique_ptr<CInputStream> getStreamFromFS(const std::string & name);

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
	static std::unique_ptr<IMapLoader> getMapLoader(std::unique_ptr<CInputStream> & stream);
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
};

class DLL_LINKAGE CMapLoaderH3M : public IMapLoader
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream containing the map data
	 */
	CMapLoaderH3M(CInputStream * stream);

	/**
	 * Destructor.
	 */
	~CMapLoaderH3M();

	/**
	 * Loads the VCMI/H3 map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	std::unique_ptr<CMap> loadMap();

	/**
	 * Loads the VCMI/H3 map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader();

	/** true if you want to enable the map loader profiler to see how long a specific part took; default=false */
	static const bool IS_PROFILING_ENABLED;

private:
	/**
	 * Initializes the map object from parsing the input buffer.
	 */
	void init();

	/**
	 * Reads the map header.
	 */
	void readHeader();

	/**
	 * Reads player information.
	 */
	void readPlayerInfo();

	/**
	 * Reads victory/loss conditions.
	 */
	void readVictoryLossConditions();

	/**
	 * Reads team information.
	 */
	void readTeamInfo();

	/**
	 * Reads the list of allowed heroes.
	 */
	void readAllowedHeroes();

	/**
	 * Reads the list of disposed heroes.
	 */
	void readDisposedHeroes();

	/**
	 * Reads the list of allowed artifacts.
	 */
	void readAllowedArtifacts();

	/**
	 * Reads the list of allowed spells and abilities.
	 */
	void readAllowedSpellsAbilities();

	/**
	 * Loads artifacts of a hero.
	 *
	 * @param hero the hero which should hold those artifacts
	 */
	void loadArtifactsOfHero(CGHeroInstance * hero);

	/**
	 * Loads an artifact to the given slot of the specified hero.
	 *
	 * @param hero the hero which should hold that artifact
	 * @param slot the artifact slot where to place that artifact
	 * @return true if it loaded an artifact
	 */
	bool loadArtifactToSlot(CGHeroInstance * hero, int slot);

	/**
	 * Creates an artifact instance.
	 *
	 * @param aid the id of the artifact
	 * @param spellID optional. the id of a spell if a spell scroll object should be created
	 * @return the created artifact instance
	 */
	CArtifactInstance * createArtifact(int aid, int spellID = -1);

	/**
	 * Read rumors.
	 */
	void readRumors();

	/**
	 * Reads predefined heroes.
	 */
	void readPredefinedHeroes();

	/**
	 * Reads terrain data.
	 */
	void readTerrain();

	/**
	 * Reads custom(map) def information.
	 */
	void readDefInfo();

	/**
	 * Reads objects(towns, mines,...).
	 */
	void readObjects();

	/**
	 * Reads a creature set.
	 *
	 * @param out the loaded creature set
	 * @param number the count of creatures to read
	 * @param version true for > ROE maps
	 */
	void readCreatureSet(CCreatureSet * out, int number, bool version);

	/**
	 * Reads a hero.
	 *
	 * @param idToBeGiven the object id which should be set for the hero
	 * @return a object instance
	 */
	CGObjectInstance * readHero(int idToBeGiven);

	/**
	 * Reads a seer hut.
	 *
	 * @return the initialized seer hut object
	 */
	CGSeerHut * readSeerHut();

	/**
	 * Reads a quest for the given quest guard.
	 *
	 * @param guard the quest guard where that quest should be applied to
	 */
	void readQuest(IQuestObject * guard);

	/**
	 * Reads a town.
	 *
	 * @param castleID the id of the castle type
	 * @return the loaded town object
	 */
	CGTownInstance * readTown(int castleID);

	/**
	 * Converts buildings to the specified castle id.
	 *
	 * @param h3m the ids of the buildings
	 * @param castleID the castle id
	 * @param addAuxiliary true if the village hall should be added
	 * @return the converted buildings
	 */
	std::set<si32> convertBuildings(const std::set<si32> h3m, int castleID, bool addAuxiliary = true);

	/**
	 * Reads events.
	 */
	void readEvents();

	/**
	 * Reverses the input argument.
	 *
	 * @param arg the input argument
	 * @return the reversed 8-bit integer
	 */
	ui8 reverse(ui8 arg);

	/**
	 * Init buffer / size.
	 *
	 * @param stream the stream which serves as the data input
	 */
	void initBuffer(CInputStream * stream);

	/** ptr to the map object which gets filled by data from the buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer.
	 * (when loading a map then the mapHeader ptr points to the same object)
	 */
	std::unique_ptr<CMapHeader> mapHeader;

	/** pointer to the array containing the map data;
	 * TODO replace with CBinaryReader later (this makes pos & size redundant) */
	ui8 * buffer;

	/** current buffer reading position */
	int pos;

	/** size of the map in bytes */
	int size;
};
