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
#include "MapFeaturesH3M.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class MapReaderH3M;
class MetaString;
class CArtifactInstance;
class CArmedInstance;
class CGObjectInstance;
class CGSeerHut;
class IQuestObject;
class CGTownInstance;
class CCreatureSet;
class CInputStream;
class TextIdentifier;
class CGPandoraBox;
class CMapEvent;

class ObjectInstanceID;
class BuildingID;
class ObjectTemplate;
class SpellID;
class PlayerColor;
class int3;

enum class EQuestMission;

enum class EVictoryConditionType : int8_t
{
	WINSTANDARD = -1,
	ARTIFACT = 0,
	GATHERTROOP = 1,
	GATHERRESOURCE = 2,
	BUILDCITY = 3,
	BUILDGRAIL = 4,
	BEATHERO = 5,
	CAPTURECITY = 6,
	BEATMONSTER = 7,
	TAKEDWELLINGS = 8,
	TAKEMINES = 9,
	TRANSPORTITEM = 10,
	HOTA_ELIMINATE_ALL_MONSTERS = 11,
	HOTA_SURVIVE_FOR_DAYS = 12
};

enum class ELossConditionType : int8_t
{
	LOSSSTANDARD = -1,
	LOSSCASTLE = 0,
	LOSSHERO = 1,
	TIMEEXPIRES = 2
};

class DLL_LINKAGE CMapLoaderH3M : public IMapLoader
{
public:
	/**
	 * Default constructor.
	 *
	 * @param stream a stream containing the map data
	 */
	CMapLoaderH3M(const std::string & mapName, const std::string & modName, const std::string & encodingName, CInputStream * stream);

	/**
	 * Destructor.
	 */
	~CMapLoaderH3M();

	/**
	 * Loads the VCMI/H3 map file.
	 *
	 * @return a unique ptr of the loaded map class
	 */
	std::unique_ptr<CMap> loadMap(IGameCallback * cb) override;

	/**
	 * Loads the VCMI/H3 map header.
	 *
	 * @return a unique ptr of the loaded map header class
	 */
	std::unique_ptr<CMapHeader> loadMapHeader() override;

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
	 * Reads the list of map flags.
	 */
	void readMapOptions();

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
	void readObjectTemplates();

	/**
	 * Reads objects(towns, mines,...).
	 */
	void readObjects();

	/// Reads single object from input stream based on template
	std::shared_ptr<CGObjectInstance> readObject(MapObjectID id, MapObjectSubID subid, std::shared_ptr<const ObjectTemplate> objectTemplate, const int3 & objectPosition, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readEvent(const int3 & objectPosition, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readMonster(const int3 & objectPosition, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readHero(const int3 & initialPos, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readSeerHut(const int3 & initialPos, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readTown(const int3 & position, std::shared_ptr<const ObjectTemplate> objTempl, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readSign(const int3 & position);
	std::shared_ptr<CGObjectInstance> readWitchHut(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readScholar(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readGarrison(const int3 & mapPosition, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readArtifact(const int3 & position, std::shared_ptr<const ObjectTemplate> objTempl, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readScroll(const int3 & position, std::shared_ptr<const ObjectTemplate> objTempl, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readResource(const int3 & position, std::shared_ptr<const ObjectTemplate> objTempl, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readMine(const int3 & position);
	std::shared_ptr<CGObjectInstance> readAbandonedMine(const int3 & position);
	std::shared_ptr<CGObjectInstance> readPandora(const int3 & position, const ObjectInstanceID & idToBeGiven);
	std::shared_ptr<CGObjectInstance> readDwelling(const int3 & position);
	std::shared_ptr<CGObjectInstance> readDwellingRandom(const int3 & position, std::shared_ptr<const ObjectTemplate> objTempl);
	std::shared_ptr<CGObjectInstance> readShrine(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readHeroPlaceholder(const int3 & position);
	std::shared_ptr<CGObjectInstance> readGrail(const int3 & position);
	std::shared_ptr<CGObjectInstance> readHotaBattleLocation(const int3 & position);
	std::shared_ptr<CGObjectInstance> readQuestGuard(const int3 & position);
	std::shared_ptr<CGObjectInstance> readShipyard(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readLighthouse(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readGeneric(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readBank(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readRewardWithArtifact(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readRewardWithArtifactAndResources(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readBlackMarket(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);
	std::shared_ptr<CGObjectInstance> readUniversity(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate);

	/**
	 * Reads a creature set.
	 *
	 * @param out the loaded creature set
	 * @param number the count of creatures to read
	 */
	void readCreatureSet(CArmedInstance * out, const ObjectInstanceID & idToBeGiven);

	void readBoxContent(CGPandoraBox * object, const int3 & position, const ObjectInstanceID & idToBeGiven);
	void readBoxHotaContent(CGPandoraBox * object, const int3 & position, const ObjectInstanceID & idToBeGiven);

	void readEventCommon(CMapEvent & object, const TextIdentifier & messageID);

	/**
	 * Reads a quest for the given quest guard.
	 *
	 * @param guard the quest guard where that quest should be applied to
	 */
	EQuestMission readQuest(IQuestObject * guard, const int3 & position);

	void readSeerHutQuest(CGSeerHut * hut, const int3 & position, const ObjectInstanceID & idToBeGiven);

	/**
	 * Reads events.
	 */
	void readEvents();

	/**
	* read optional message and optional guards
	*/
	void readMessageAndGuards(MetaString & message, CArmedInstance * guards, const int3 & position, const ObjectInstanceID & idToBeGiven);

	/// reads string from input stream and converts it to unicode
	std::string readBasicString();

	/// reads string from input stream, converts it to unicode and attempts to translate it
	std::string readLocalizedString(const TextIdentifier & identifier);

	void setOwnerAndValidate(const int3 & mapPosition, CGObjectInstance * object, const PlayerColor & owner);

	void afterRead();

	MapFormatFeaturesH3M features;

	/** List of templates loaded from the map, used on later stage to create
	 *  objects but not needed for fully functional CMap */
	std::vector<std::shared_ptr<ObjectTemplate>> originalTemplates;
	std::vector<std::shared_ptr<ObjectTemplate>> remappedTemplates;

	/// associative list to identify which hero/creature id belongs to which object id(index for objects)
	std::map<si32, ObjectInstanceID> questIdentifierToId;
	std::map<IQuestObject*, si32> questsToResolve;

	/** ptr to the map object which gets filled by data from the buffer */
	CMap * map;

	/**
	 * ptr to the map header object which gets filled by data from the buffer.
	 * (when loading a map then the mapHeader ptr points to the same object)
	 */
	std::unique_ptr<CMapHeader> mapHeader;
	std::unique_ptr<MapReaderH3M> reader;
	CInputStream * inputStream;

	std::string mapName;
	std::string modName;
	std::string fileEncoding;

};

VCMI_LIB_NAMESPACE_END
