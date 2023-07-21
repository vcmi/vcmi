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

#include "CMapHeader.h"
#include "../mapObjects/MiscObjects.h" // To serialize static props
#include "../mapObjects/CQuest.h" // To serialize static props
#include "../mapObjects/CGTownInstance.h" // To serialize static props
#include "CMapDefines.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactInstance;
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
struct TeleportChannel;

/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name;
	std::string text;

	Rumor() = default;
	~Rumor() = default;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name;
		h & text;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

/// The disposed hero struct describes which hero can be hired from which player.
struct DLL_LINKAGE DisposedHero
{
	DisposedHero();

	HeroTypeID heroId;
	HeroTypeID portrait; /// The portrait id of the hero, -1 is default.
	std::string name;
	PlayerColor::Mask players; /// Who can hire this hero (bitfield).

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & heroId;
		h & portrait;
		h & name;
		h & players;
	}
};

/// The map contains the map header, the tiles of the terrain, objects, heroes, towns, rumors...
class DLL_LINKAGE CMap : public CMapHeader
{
public:
	CMap();
	~CMap();
	void initTerrain();

	CMapEditManager * getEditManager();
	TerrainTile & getTile(const int3 & tile);
	const TerrainTile & getTile(const int3 & tile) const;
	bool isCoastalTile(const int3 & pos) const;
	bool isInTheMap(const int3 & pos) const;
	bool isWaterTile(const int3 & pos) const;

	bool canMoveBetween(const int3 &src, const int3 &dst) const;
	bool checkForVisitableDir(const int3 & src, const TerrainTile * pom, const int3 & dst) const;
	int3 guardingCreaturePosition (int3 pos) const;

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);
	void calculateGuardingGreaturePositions();

	void addNewArtifactInstance(CArtifactInstance * art);
	void eraseArtifactInstance(CArtifactInstance * art);

	void addNewQuestInstance(CQuest * quest);
	void removeQuestInstance(CQuest * quest);

	void setUniqueInstanceName(CGObjectInstance * obj);
	///Use only this method when creating new map object instances
	void addNewObject(CGObjectInstance * obj);
	void moveObject(CGObjectInstance * obj, const int3 & dst);
	void removeObject(CGObjectInstance * obj);

	bool isWaterMap() const;
	bool calculateWaterContent();
	void banWaterArtifacts();
	void banWaterHeroes();
	void banHero(const HeroTypeID& id);
	void banWaterSpells();
	void banWaterSkills();
	void banWaterContent();

	/// Gets object of specified type on requested position
	const CGObjectInstance * getObjectiveObjectFrom(const int3 & pos, Obj::EObj type);
	CGHeroInstance * getHero(int heroId);

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

	void resetStaticData();

	ui32 checksum;
	std::vector<Rumor> rumors;
	std::vector<DisposedHero> disposedHeroes;
	std::vector<ConstTransitivePtr<CGHeroInstance> > predefinedHeroes;
	std::vector<bool> allowedSpells;
	std::vector<bool> allowedArtifact;
	std::vector<bool> allowedAbilities;
	std::list<CMapEvent> events;
	int3 grailPos;
	int grailRadius;

	//Central lists of items in game. Position of item in the vectors below is their (instance) id.
	std::vector< ConstTransitivePtr<CGObjectInstance> > objects;
	std::vector< ConstTransitivePtr<CGTownInstance> > towns;
	std::vector< ConstTransitivePtr<CArtifactInstance> > artInstances;
	std::vector< ConstTransitivePtr<CQuest> > quests;
	std::vector< ConstTransitivePtr<CGHeroInstance> > allHeroes; //indexed by [hero_type_id]; on map, disposed, prisons, etc.

	//Helper lists
	std::vector< ConstTransitivePtr<CGHeroInstance> > heroesOnMap;
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > teleportChannels;

	/// associative list to identify which hero/creature id belongs to which object id(index for objects)
	std::map<si32, ObjectInstanceID> questIdentifierToId;

	std::unique_ptr<CMapEditManager> editManager;
	boost::multi_array<int3, 3> guardingCreaturePositions;

	std::map<std::string, ConstTransitivePtr<CGObjectInstance> > instanceNames;

	bool waterMap;

private:
	/// a 3-dimensional array of terrain tiles, access is as follows: x, y, level. where level=1 is underground
	boost::multi_array<TerrainTile, 3> terrain;

	si32 uidCounter; //TODO: initialize when loading an old map

public:
	template <typename Handler>
	void serialize(Handler &h, const int formatVersion)
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
		h & questIdentifierToId;

		//TODO: viccondetails
		h & terrain;
		h & guardingCreaturePositions;

		h & objects;
		h & heroesOnMap;
		h & teleportChannels;
		h & towns;
		h & artInstances;

		// static members
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGObelisk::obeliskCount;
		h & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;
		h & CGTownInstance::universitySkills;

		h & instanceNames;
	}
};

VCMI_LIB_NAMESPACE_END
