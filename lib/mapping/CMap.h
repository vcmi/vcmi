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

#include "../GameCallbackHolder.h"
#include "../networkPacks/TradeItem.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactInstance;
class CArtifactSet;
class CGObjectInstance;
class CGHeroInstance;
class CCommanderInstance;
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

	std::vector< std::shared_ptr<CQuest> > quests;
	std::vector< std::shared_ptr<CArtifactInstance> > artInstances;

public:
	explicit CMap(IGameCallback *cb);
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

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);
	void calculateGuardingGreaturePositions();

	CArtifactInstance * createScroll(const SpellID & spellId);
	CArtifactInstance * createArtifact(const ArtifactID & artId, const SpellID & spellId = SpellID::NONE);
	CArtifactInstance * createSingleArtifact(const ArtifactID & artId, const SpellID & spellId = SpellID::NONE);

	CArtifactInstance * getArtifactInstance(const ArtifactInstanceID & artifactID);
	const CArtifactInstance * getArtifactInstance(const ArtifactInstanceID & artifactID) const;

	void eraseArtifactInstance(const ArtifactInstanceID art);
	void moveArtifactInstance(CArtifactSet & srcSet, const ArtifactPosition & srcSlot, CArtifactSet & dstSet, const ArtifactPosition & dstSlot);
	void putArtifactInstance(CArtifactSet & set, const ArtifactInstanceID art, const ArtifactPosition & slot);
	void removeArtifactInstance(CArtifactSet & set, const ArtifactPosition & slot);

	void addNewQuestInstance(std::shared_ptr<CQuest> quest);
	void removeQuestInstance(std::shared_ptr<CQuest> quest);
	void clearQuestInstance(const CQuest * quest);

	void setUniqueInstanceName(ObjectInstanceID target){};
	///Use only this method when creating new map object instances
	void addNewObject(std::shared_ptr<CGObjectInstance> obj){}
	void moveObject(ObjectInstanceID target, const int3 & dst){}

	/// Remove objects and shifts object indicies.
	/// Only for use in map editor / RMG
	std::shared_ptr<CGObjectInstance> removeObject(ObjectInstanceID oldObject){return nullptr;}

	/// Replaced map object with specified ID with new object
	/// Old object must exist and will be removed from map
	/// Returns pointer to old object, which can be manipulated or dropped
	std::shared_ptr<CGObjectInstance> replaceObject(ObjectInstanceID oldObject, const std::shared_ptr<CGObjectInstance> & newObject){return nullptr;}

	/// Erases object from map without shifting indices
	/// Returns pointer to old object, which can be manipulated or dropped
	std::shared_ptr<CGObjectInstance> eraseObject(ObjectInstanceID oldObject){return nullptr;}

	void heroAddedToMap(const CGHeroInstance * hero) {}
	void heroRemovedFromMap(const CGHeroInstance * hero) {}
	void townAddedToMap(const CGTownInstance * town) {}
	void townRemovedFromMap(const CGTownInstance * town) {}

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
	CGHeroInstance * getHero(HeroTypeID heroId);

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

	void resetStaticData();
	void resolveQuestIdentifiers();

	void reindexObjects();

	std::vector<Rumor> rumors;
	std::set<SpellID> allowedSpells;
	std::set<ArtifactID> allowedArtifact;
	std::set<SecondarySkill> allowedAbilities;
	std::vector<CMapEvent> events;
	int3 grailPos;
	int grailRadius;

	//Central lists of items in game. Position of item in the vectors below is their (instance) id.
	std::vector< std::shared_ptr<CGObjectInstance> > objects;
	std::vector< std::shared_ptr<CGTownInstance> > towns;
	std::vector< std::shared_ptr<CGHeroInstance> > allHeroes; //indexed by [hero_type_id]; on map, disposed, prisons, etc.

	//Helper lists
	std::vector< std::shared_ptr<CGHeroInstance> > heroesOnMap;
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > teleportChannels;
	std::vector<std::shared_ptr<CGHeroInstance> > predefinedHeroes;

	/// associative list to identify which hero/creature id belongs to which object id(index for objects)
	std::map<si32, ObjectInstanceID> questIdentifierToId;

	std::unique_ptr<CMapEditManager> editManager;
	boost::multi_array<int3, 3> guardingCreaturePositions;

	std::map<std::string, std::shared_ptr<CGObjectInstance> > instanceNames;

	bool waterMap;

	ui8 obeliskCount = 0; //how many obelisks are on map
	std::map<TeamID, ui8> obelisksVisited; //map: team_id => how many obelisks has been visited

	std::vector<ArtifactID> townMerchantArtifacts;
	std::vector<TradeItemBuy> townUniversitySkills;

	void overrideGameSettings(const JsonNode & input);
	void overrideGameSetting(EGameSettings option, const JsonNode & input);
	const IGameSettings & getSettings() const;

private:
	/// a 3-dimensional array of terrain tiles, access is as follows: x, y, level. where level=1 is underground
	boost::multi_array<TerrainTile, 3> terrain;

	si32 uidCounter; //TODO: initialize when loading an old map

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
		h & quests;
		h & allHeroes;

		//TODO: viccondetails
		h & terrain;
		h & guardingCreaturePositions;

		h & objects;
		h & heroesOnMap;
		h & teleportChannels;
		h & towns;
		h & artInstances;

		// static members
		h & obeliskCount;
		h & obelisksVisited;
		h & townMerchantArtifacts;
		h & townUniversitySkills;

		h & instanceNames;
		h & *gameSettings;
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
