/*
 * CMap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapDefines.h"
#include "CMapHeader.h"

#include "../mapObjects/CGObjectInstance.h"
#include "../callback/GameCallbackHolder.h"
#include "../networkPacks/TradeItem.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactInstance;
class CArtifactSet;
class CGObjectInstance;
class CGHeroInstance;
class CCommanderInstance;
class CGameState;
class CGCreature;
class CQuest;
class CGTownInstance;
class IModableArt;
class IQuestObject;
class CInputStream;
class CMapEditManager;
class JsonSerializeFormat;
class IGameSettings;
class GameSettings;
struct TeleportChannel;
enum class EGameSettings;

/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name;
	MetaString text;

	Rumor() = default;
	~Rumor() = default;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & text;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

/// The map contains the map header, the tiles of the terrain, objects, heroes, towns, rumors...
class DLL_LINKAGE CMap : public CMapHeader, public GameCallbackHolder
{
	friend class CSerializer;

	std::unique_ptr<GameSettings> gameSettings;

	/// All artifacts that exists on map, whether on map, in hero inventory, or stored in some object
	std::vector<std::shared_ptr<CArtifactInstance>> artInstances;
	/// All heroes that are currently free for recruitment in taverns and are not present on map
	std::vector<std::shared_ptr<CGHeroInstance> > heroesPool;

	/// Precomputed indices of all towns on map
	std::vector<ObjectInstanceID> towns;

	/// Precomputed indices of all heroes on map. Does not includes heroes in prisons
	std::vector<ObjectInstanceID> heroesOnMap;

public:
	/// Central lists of items in game. Position of item in the vectors below is their (instance) id.
	/// TODO: make private
	std::vector<std::shared_ptr<CGObjectInstance>> objects;

	explicit CMap(IGameInfoCallback *cb);
	~CMap();
	void initTerrain();

	CMapEditManager * getEditManager();
	inline TerrainTile & getTile(const int3 & tile);
	inline const TerrainTile & getTile(const int3 & tile) const;
	bool isCoastalTile(const int3 & pos) const;
	inline bool isInTheMap(const int3 & pos) const;

	bool canMoveBetween(const int3 &src, const int3 &dst) const;
	bool checkForVisitableDir(const int3 & src, const TerrainTile * pom, const int3 & dst) const;
	int3 guardingCreaturePosition (int3 pos) const;

	void calculateGuardingGreaturePositions();

	void saveCompatibilityAddMissingArtifact(std::shared_ptr<CArtifactInstance> artifact);

	/// Creates instance of spell scroll artifact with provided spell
	CArtifactInstance * createScroll(const SpellID & spellId);

	/// Creates instance of requested artifact
	/// For combined artifact this method will also create alll required components
	/// For scrolls this method will also initialize its spell
	CArtifactInstance * createArtifact(const ArtifactID & artId, const SpellID & spellId = SpellID::NONE);

	/// Creates single instance of requested artifact
	/// Does NOT creates components for combined artifacts
	/// Does NOT initializes spell when spell scroll artifact is created
	CArtifactInstance * createArtifactComponent(const ArtifactID & artId);

	/// Returns pointer to requested Artifact Instance. Throws on invalid ID
	CArtifactInstance * getArtifactInstance(const ArtifactInstanceID & artifactID);
	/// Returns pointer to requested Artifact Instance. Throws on invalid ID
	const CArtifactInstance * getArtifactInstance(const ArtifactInstanceID & artifactID) const;

	/// Completely removes artifact instance from the game
	void eraseArtifactInstance(const ArtifactInstanceID art);

	void moveArtifactInstance(CArtifactSet & srcSet, const ArtifactPosition & srcSlot, CArtifactSet & dstSet, const ArtifactPosition & dstSlot);
	void putArtifactInstance(CArtifactSet & set, const ArtifactInstanceID art, const ArtifactPosition & slot);
	void removeArtifactInstance(CArtifactSet & set, const ArtifactPosition & slot);

	/// Generates unique string identifier for provided object instance
	void generateUniqueInstanceName(CGObjectInstance * target);

	/// Generates new, unique numeric identifier that can be used for creation of a new object
	ObjectInstanceID allocateUniqueInstanceID();

	/// Adds provided object to the map
	/// Throws on error, for example if object is already on map
	void addNewObject(std::shared_ptr<CGObjectInstance> obj);

	/// Moves anchor position of requested object to specified coordinates and updates map state
	/// Throws in invalid object instance ID
	void moveObject(ObjectInstanceID target, const int3 & dst);

	/// Hides object from map without actually removing it from object list
	void hideObject(CGObjectInstance * obj);

	/// Shows previously hiiden object on map
	void showObject(CGObjectInstance * obj);

	/// Remove objects and shifts object indicies.
	/// Only for use in map editor / RMG
	std::shared_ptr<CGObjectInstance> removeObject(ObjectInstanceID oldObject);

	/// Replaced map object with specified ID with new object
	/// Old object must exist and will be removed from map
	/// Returns pointer to old object, which can be manipulated or dropped
	std::shared_ptr<CGObjectInstance> replaceObject(ObjectInstanceID oldObject, const std::shared_ptr<CGObjectInstance> & newObject);

	/// Erases object from map without shifting indices
	/// Returns pointer to old object, which can be manipulated or dropped
	std::shared_ptr<CGObjectInstance> eraseObject(ObjectInstanceID oldObject);

	void heroAddedToMap(const CGHeroInstance * hero);
	void heroRemovedFromMap(const CGHeroInstance * hero);
	void townAddedToMap(const CGTownInstance * town);
	void townRemovedFromMap(const CGTownInstance * town);

	/// Adds provided hero to map pool. Hero with same identity must not exist
	void addToHeroPool(std::shared_ptr<CGHeroInstance> hero);

	/// Attempts to take hero of specified identity from pool. Returns nullptr on failure
	/// Hero is removed from pool on success
	std::shared_ptr<CGHeroInstance> tryTakeFromHeroPool(HeroTypeID hero);

	/// Attempts to access hero of specified identity in pool. Returns nullptr on failure
	CGHeroInstance * tryGetFromHeroPool(HeroTypeID hero);

	/// Returns list of identities of heroes currently present in pool
	std::vector<HeroTypeID> getHeroesInPool() const;

	CGObjectInstance * getObject(ObjectInstanceID obj);
	const CGObjectInstance * getObject(ObjectInstanceID obj) const;

	void attachToBonusSystem(CGameState & gs);

	/// Returns all valid objects of specified class present on map
	template<typename ObjectType = CGObjectInstance>
	std::vector<const ObjectType *> getObjects() const
	{
		std::vector<const ObjectType *> result;
		for (const auto & object : objects)
		{
			auto casted = dynamic_cast<const ObjectType*>(object.get());
			if (casted)
				result.push_back(casted);
		}
		return result;
	}

	/// Returns all valid objects of specified class present on map
	template<typename ObjectType = CGObjectInstance>
	std::vector<ObjectType *> getObjects()
	{
		std::vector<ObjectType *> result;
		for (const auto & object : objects)
		{
			auto casted = dynamic_cast<ObjectType*>(object.get());
			if (casted)
				result.push_back(casted);
		}
		return result;
	}

	/// Returns all valid artifacts present on map
	std::vector<CArtifactInstance *> getArtifacts()
	{
		std::vector<CArtifactInstance *> result;
		for (const auto & art : artInstances)
			if (art)
				result.push_back(art.get());

		return result;
	}

	bool isWaterMap() const;
	bool calculateWaterContent();
	void banWaterArtifacts();
	void banWaterHeroes();
	void banHero(const HeroTypeID& id);
	void unbanHero(const HeroTypeID & id);
	void banWaterSpells();
	void banWaterSkills();
	void banWaterContent();

	/// Gets object of specified type on requested position
	const CGObjectInstance * getObjectiveObjectFrom(const int3 & pos, Obj type);

	/// Returns pointer to hero of specified type if hero is present on map
	CGHeroInstance * getHero(HeroTypeID heroId);
	const CGHeroInstance * getHero(HeroTypeID heroId) const;

	/// Returns ID's of all heroes that are currently present on map
	/// Includes all garrisoned and imprisoned heroes
	const std::vector<ObjectInstanceID> & getHeroesOnMap() const;

	/// Returns ID's of all towns present on map
	const std::vector<ObjectInstanceID> & getAllTowns() const;

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

	void resolveQuestIdentifiers();

	void reindexObjects();

	std::vector<Rumor> rumors;
	std::set<SpellID> allowedSpells;
	std::set<ArtifactID> allowedArtifact;
	std::set<SecondarySkill> allowedAbilities;
	std::vector<CMapEvent> events;
	int3 grailPos;
	int grailRadius;

	//Helper lists
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > teleportChannels;


	std::unique_ptr<CMapEditManager> editManager;
	boost::multi_array<int3, 3> guardingCreaturePositions;

	std::map<std::string, std::shared_ptr<CGObjectInstance> > instanceNames;

	bool waterMap;

	ui8 obeliskCount = 0; //how many obelisks are on map
	std::map<TeamID, ui8> obelisksVisited; //map: team_id => how many obelisks has been visited

	std::vector<ArtifactID> townMerchantArtifacts;

	void overrideGameSettings(const JsonNode & input);
	void overrideGameSetting(EGameSettings option, const JsonNode & input);
	const IGameSettings & getSettings() const;

	void saveCompatibilityStoreAllocatedArtifactID();
	void parseUidCounter();

private:

	/// a 3-dimensional array of terrain tiles, access is as follows: x, y, level. where level=1 is underground
	boost::multi_array<TerrainTile, 3> terrain;

	si32 uidCounter; 

public:
	template <typename Handler>
	void serialize(Handler &h)
	{
		h & static_cast<CMapHeader&>(*this);
		h & triggeredEvents; //from CMapHeader
		h & rumors;
		h & allowedSpells;
		h & allowedAbilities;
		h & allowedArtifact;
		h & events;
		h & grailPos;
		h & artInstances;

		if (!h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			saveCompatibilityStoreAllocatedArtifactID();
			std::vector< std::shared_ptr<CQuest> > quests;
			h & quests;
		}
		h & heroesPool;

		//TODO: viccondetails
		h & terrain;
		h & guardingCreaturePositions;

		h & objects;
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			h & heroesOnMap;
		else
		{
			std::vector<std::shared_ptr<CGObjectInstance>> objectPtrs;
			h & objectPtrs;
			for (const auto & ptr : objectPtrs)
				heroesOnMap.push_back(ptr->id);

			for (auto & ptr : heroesPool)
				if (vstd::contains(objects, ptr))
					ptr = nullptr;
		}

		h & teleportChannels;
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			h & towns;
		else
		{
			std::vector<std::shared_ptr<CGObjectInstance>> objectPtrs;
			h & objectPtrs;
			for (const auto & ptr : objectPtrs)
				towns.push_back(ptr->id);
		}
		h & artInstances;

		// static members
		h & obeliskCount;
		h & obelisksVisited;
		h & townMerchantArtifacts;
		if (!h.hasFeature(Handler::Version::UNIVERSITY_CONFIG))
		{
			std::vector<TradeItemBuy> townUniversitySkills;
			h & townUniversitySkills;
		}

		h & instanceNames;
		h & *gameSettings;
		if (!h.hasFeature(Handler::Version::STORE_UID_COUNTER_IN_CMAP))
		{
			if (!h.saving)
				parseUidCounter();
		}
		else
		{
			h & uidCounter;
		}
	}
};

inline bool CMap::isInTheMap(const int3 & pos) const
{
	// Check whether coord < 0 is done implicitly. Negative signed int overflows to unsigned number larger than all signed ints.
	return
		static_cast<uint32_t>(pos.x) < static_cast<uint32_t>(width) &&
		static_cast<uint32_t>(pos.y) < static_cast<uint32_t>(height) &&
		static_cast<uint32_t>(pos.z) <= (twoLevel ? 1 : 0);
}

inline TerrainTile & CMap::getTile(const int3 & tile)
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

inline const TerrainTile & CMap::getTile(const int3 & tile) const
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

VCMI_LIB_NAMESPACE_END
