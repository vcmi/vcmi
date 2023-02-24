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
#include "../GameConstants.h"
#include "../ResourceSet.h"
#include "../mapObjects/ObjectTemplate.h"

#include "../int3.h"


VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CBinaryReader;
class CArtifactInstance;
class CGObjectInstance;
class CGSeerHut;
class IQuestObject;
class CGTownInstance;
class CCreatureSet;
class CInputStream;


class DLL_LINKAGE CMapLoaderH3M : public IMapLoader
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream containing the map data
	 */
	CMapLoaderH3M(const std::string & mapName, const std::string & encodingName, CInputStream * stream);

	/**
	 * Destructor.
	 */
	~CMapLoaderH3M();

	/**
	 * Loads the VCMI/H3 map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	std::unique_ptr<CMap> loadMap() override;

	/**
	 * Loads the VCMI/H3 map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader() override;

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
	 */
	void readCreatureSet(CCreatureSet * out, int number);

	/**
	 * Reads a hero.
	 *
	 * @param idToBeGiven the object id which should be set for the hero
	 * @return a object instance
	 */
	CGObjectInstance * readHero(const ObjectInstanceID & idToBeGiven, const int3 & initialPos);

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
	std::set<BuildingID> convertBuildings(const std::set<BuildingID> & h3m, int castleID, bool addAuxiliary = true) const;

	/**
	 * Reads events.
	 */
	void readEvents();

	/**
	* read optional message and optional guards
	*/
	void readMessageAndGuards(std::string& message, CCreatureSet * guards);

	void readSpells(std::set<SpellID> & dest);

	void readResourses(TResources& resources);

	template <class Indenifier>
	void readBitmask(std::set<Indenifier> &dest, const int byteCount, const int limit, bool negate = true);

	/** Reads bitmask to boolean vector
	* @param dest destination vector, shall be filed with "true" values
	* @param byteCount size in bytes of bimask
	* @param limit max count of vector elements to alter
	* @param negate if true then set bit in mask means clear flag in vertor
	*/
	void readBitmask(std::vector<bool> & dest, const int byteCount, const int limit, bool negate = true);

	/**
	 * Reverses the input argument.
	 *
	 * @param arg the input argument
	 * @return the reversed 8-bit integer
	 */
	ui8 reverse(ui8 arg) const;

	/**
	* Helper to read map position
	*/
	int3 readInt3();

	/// reads string from input stream and converts it to unicode
	std::string readLocalizedString();

	void afterRead();

	/** List of templates loaded from the map, used on later stage to create
	 *  objects but not needed for fully functional CMap */
	std::vector<std::shared_ptr<const ObjectTemplate>> templates;

	/** ptr to the map object which gets filled by data from the buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer.
	 * (when loading a map then the mapHeader ptr points to the same object)
	 */
	std::unique_ptr<CMapHeader> mapHeader;
	std::unique_ptr<CBinaryReader> reader;
	CInputStream * inputStream;

};

VCMI_LIB_NAMESPACE_END
