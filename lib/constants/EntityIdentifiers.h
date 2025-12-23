/*
 * EntityIdentifiers.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Global.h"
#include "NumericConstants.h"
#include "IdentifierBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class Services;
class Artifact;
class ArtifactService;
class Creature;
class CreatureService;
class HeroType;
class CHero;
class CHeroClass;
class HeroClass;
class HeroTypeService;
class Resource;
class ResourceType;
class ResourceTypeService;
class CFaction;
class Faction;
class Skill;
class RoadType;
class RiverType;
class TerrainType;

namespace spells
{
	class Spell;
	class Service;
}

class CArtifact;
class CArtifactInstance;
class CCreature;
class CHero;
class CSpell;
class CSkill;
class IGameInfoCallback;
class CNonConstInfoCallback;

class ArtifactInstanceID : public StaticIdentifier<ArtifactInstanceID>
{
public:
	using StaticIdentifier<ArtifactInstanceID>::StaticIdentifier;

	DLL_LINKAGE static std::string encode(const si32 index);
};

class QueryID : public StaticIdentifier<QueryID>
{
public:
	using StaticIdentifier<QueryID>::StaticIdentifier;
	DLL_LINKAGE static const QueryID NONE;
	DLL_LINKAGE static const QueryID CLIENT;
};

class BattleID : public StaticIdentifier<BattleID>
{
public:
	using StaticIdentifier<BattleID>::StaticIdentifier;
	DLL_LINKAGE static const BattleID NONE;
};

class DLL_LINKAGE ObjectInstanceID : public StaticIdentifier<ObjectInstanceID>
{
public:
	using StaticIdentifier<ObjectInstanceID>::StaticIdentifier;
	static const ObjectInstanceID NONE;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class DLL_LINKAGE QuestInstanceID : public StaticIdentifier<QuestInstanceID>
{
public:
	using StaticIdentifier<QuestInstanceID>::StaticIdentifier;
	static const QuestInstanceID NONE;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class HeroClassID : public EntityIdentifier<HeroClassID>
{
public:
	using EntityIdentifier<HeroClassID>::EntityIdentifier;
	///json serialization helpers
	DLL_LINKAGE static si32 decode(const std::string & identifier);
	DLL_LINKAGE static std::string encode(const si32 index);
	static std::string entityType();

	const CHeroClass * toHeroClass() const;
	const HeroClass * toEntity(const Services * services) const;
};

class DLL_LINKAGE HeroTypeID : public EntityIdentifier<HeroTypeID>
{
public:
	using EntityIdentifier<HeroTypeID>::EntityIdentifier;
	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();

	const CHero * toHeroType() const;
	const HeroType * toEntity(const Services * services) const;

	static const HeroTypeID NONE;
	static const HeroTypeID RANDOM;
	static const HeroTypeID CAMP_STRONGEST;
	static const HeroTypeID CAMP_GENERATED;
	static const HeroTypeID CAMP_RANDOM;

	bool isValid() const
	{
		return getNum() >= 0;
	}
};

class SlotID : public StaticIdentifier<SlotID>
{
public:
	using StaticIdentifier<SlotID>::StaticIdentifier;

	DLL_LINKAGE static const SlotID COMMANDER_SLOT_PLACEHOLDER;
	DLL_LINKAGE static const SlotID SUMMONED_SLOT_PLACEHOLDER; ///<for all summoned creatures, only during battle
	DLL_LINKAGE static const SlotID WAR_MACHINES_SLOT; ///<for all war machines during battle
	DLL_LINKAGE static const SlotID ARROW_TOWERS_SLOT; ///<for all arrow towers during battle

	bool validSlot() const
	{
		return getNum() >= 0  &&  getNum() < GameConstants::ARMY_SIZE;
	}
};

class DLL_LINKAGE PlayerColor : public StaticIdentifier<PlayerColor>
{
public:
	using StaticIdentifier<PlayerColor>::StaticIdentifier;

	enum EPlayerColor
	{
		PLAYER_LIMIT_I = 8,
	};

	static const PlayerColor SPECTATOR; //252
	static const PlayerColor CANNOT_DETERMINE; //253
	static const PlayerColor UNFLAGGABLE; //254 - neutral objects (pandora, banks)
	static const PlayerColor NEUTRAL; //255
	static const PlayerColor PLAYER_LIMIT; //player limit per map

	static const std::array<PlayerColor, PLAYER_LIMIT_I> & ALL_PLAYERS();

	bool isValidPlayer() const; //valid means < PLAYER_LIMIT (especially non-neutral)
	bool isSpectator() const;

	std::string toString() const;

	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class TeamID : public StaticIdentifier<TeamID>
{
public:
	using StaticIdentifier<TeamID>::StaticIdentifier;

	DLL_LINKAGE static const TeamID NO_TEAM;
};

class TeleportChannelID : public StaticIdentifier<TeleportChannelID>
{
public:
	using StaticIdentifier<TeleportChannelID>::StaticIdentifier;
};

class SecondarySkillBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NONE = -1,
		PATHFINDING = 0,
		ARCHERY,
		LOGISTICS,
		SCOUTING,
		DIPLOMACY,
		NAVIGATION,
		LEADERSHIP,
		WISDOM,
		MYSTICISM,
		LUCK,
		BALLISTICS,
		EAGLE_EYE,
		NECROMANCY,
		ESTATES,
		FIRE_MAGIC,
		AIR_MAGIC,
		WATER_MAGIC,
		EARTH_MAGIC,
		SCHOLAR,
		TACTICS,
		ARTILLERY,
		LEARNING,
		OFFENCE,
		ARMORER,
		INTELLIGENCE,
		SORCERY,
		RESISTANCE,
		FIRST_AID,
		SKILL_SIZE
	};
	static_assert(GameConstants::SKILL_QUANTITY == SKILL_SIZE, "Incorrect number of skills");
};

class DLL_LINKAGE SecondarySkill : public EntityIdentifierWithEnum<SecondarySkill, SecondarySkillBase>
{
public:
	using EntityIdentifierWithEnum<SecondarySkill, SecondarySkillBase>::EntityIdentifierWithEnum;
	static std::string entityType();
	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);

	const CSkill * toSkill() const;
	const Skill * toEntity(const Services * services) const;
};

class DLL_LINKAGE PrimarySkill : public StaticIdentifier<PrimarySkill>
{
public:
	using StaticIdentifier<PrimarySkill>::StaticIdentifier;

	static const PrimarySkill NONE;
	static const PrimarySkill ATTACK;
	static const PrimarySkill DEFENSE;
	static const PrimarySkill SPELL_POWER;
	static const PrimarySkill KNOWLEDGE;

	static const std::array<PrimarySkill, 4> & ALL_SKILLS();

	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class DLL_LINKAGE FactionID : public EntityIdentifier<FactionID>
{
public:
	using EntityIdentifier<FactionID>::EntityIdentifier;

	static const FactionID NONE;
	static const FactionID DEFAULT;
	static const FactionID RANDOM;
	static const FactionID ANY;
	static const FactionID CASTLE;
	static const FactionID RAMPART;
	static const FactionID TOWER;
	static const FactionID INFERNO;
	static const FactionID NECROPOLIS;
	static const FactionID DUNGEON;
	static const FactionID STRONGHOLD;
	static const FactionID FORTRESS;
	static const FactionID CONFLUX;
	static const FactionID NEUTRAL;

	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);
	const CFaction * toFaction() const;
	const Faction * toEntity(const Services * service) const;
	static std::string entityType();

	bool isValid() const
	{
		return getNum() >= 0;
	}
};

class BuildingIDBase : public IdentifierBase
{
public:
	// Quite useful as long as most of building mechanics hardcoded
	// NOTE: all building with completely configurable mechanics will be removed from list
	enum Type
	{
		DEFAULT = -50,
		HORDE_PLACEHOLDER8 = -37,
		HORDE_PLACEHOLDER7 = -36,
		HORDE_PLACEHOLDER6 = -35,
		HORDE_PLACEHOLDER5 = -34,
		HORDE_PLACEHOLDER4 = -33,
		HORDE_PLACEHOLDER3 = -32,
		HORDE_PLACEHOLDER2 = -31,
		HORDE_PLACEHOLDER1 = -30,
		NONE = -1,
		FIRST_REGULAR_ID = 0,
		MAGES_GUILD_1 = 0,  MAGES_GUILD_2,   MAGES_GUILD_3,     MAGES_GUILD_4,   MAGES_GUILD_5,
		TAVERN,             SHIPYARD,        FORT,              CITADEL,         CASTLE,
		VILLAGE_HALL,       TOWN_HALL,       CITY_HALL,         CAPITOL,         MARKETPLACE,
		RESOURCE_SILO,      BLACKSMITH,      SPECIAL_1,         HORDE_1,         HORDE_1_UPGR,
		SHIP,               SPECIAL_2,       SPECIAL_3,         SPECIAL_4,       HORDE_2,
		HORDE_2_UPGR,       GRAIL,           EXTRA_TOWN_HALL,   EXTRA_CITY_HALL, EXTRA_CAPITOL,

		DWELL_LVL_1=30,     DWELL_LVL_2,     DWELL_LVL_3,       DWELL_LVL_4,     DWELL_LVL_5,	  DWELL_LVL_6,     DWELL_LVL_7=36,
		DWELL_LVL_1_UP=37,  DWELL_LVL_2_UP,  DWELL_LVL_3_UP,    DWELL_LVL_4_UP,  DWELL_LVL_5_UP,  DWELL_LVL_6_UP,  DWELL_LVL_7_UP=43,
		DWELL_LVL_1_UP2,    DWELL_LVL_2_UP2, DWELL_LVL_3_UP2,	DWELL_LVL_4_UP2, DWELL_LVL_5_UP2, DWELL_LVL_6_UP2, DWELL_LVL_7_UP2,
		DWELL_LVL_1_UP3,    DWELL_LVL_2_UP3, DWELL_LVL_3_UP3,	DWELL_LVL_4_UP3, DWELL_LVL_5_UP3, DWELL_LVL_6_UP3, DWELL_LVL_7_UP3,
		DWELL_LVL_1_UP4,    DWELL_LVL_2_UP4, DWELL_LVL_3_UP4,	DWELL_LVL_4_UP4, DWELL_LVL_5_UP4, DWELL_LVL_6_UP4, DWELL_LVL_7_UP4,
		DWELL_LVL_1_UP5,    DWELL_LVL_2_UP5, DWELL_LVL_3_UP5,	DWELL_LVL_4_UP5, DWELL_LVL_5_UP5, DWELL_LVL_6_UP5, DWELL_LVL_7_UP5,

		//150-155 reserved for 8. creature with potential upgrades
		DWELL_LVL_8=150, DWELL_LVL_8_UP=151, DWELL_LVL_8_UP2 = 152, DWELL_LVL_8_UP3 = 153, DWELL_LVL_8_UP4 = 154, DWELL_LVL_8_UP5 = 155,
	};
};

class DLL_LINKAGE BuildingID : public StaticIdentifierWithEnum<BuildingID, BuildingIDBase>
{
	static std::array<std::array<BuildingID, 8>, 6> getDwellings()
	{
		static const std::array<std::array<BuildingID, 8>, 6> allDwellings = {{
				{ DWELL_LVL_1, DWELL_LVL_2, DWELL_LVL_3, DWELL_LVL_4, DWELL_LVL_5, DWELL_LVL_6, DWELL_LVL_7, DWELL_LVL_8 },
				{ DWELL_LVL_1_UP, DWELL_LVL_2_UP, DWELL_LVL_3_UP, DWELL_LVL_4_UP, DWELL_LVL_5_UP, DWELL_LVL_6_UP, DWELL_LVL_7_UP, DWELL_LVL_8_UP },
				{ DWELL_LVL_1_UP2, DWELL_LVL_2_UP2, DWELL_LVL_3_UP2, DWELL_LVL_4_UP2, DWELL_LVL_5_UP2, DWELL_LVL_6_UP2, DWELL_LVL_7_UP2, DWELL_LVL_8_UP2 },
				{ DWELL_LVL_1_UP3, DWELL_LVL_2_UP3, DWELL_LVL_3_UP3, DWELL_LVL_4_UP3, DWELL_LVL_5_UP3, DWELL_LVL_6_UP3, DWELL_LVL_7_UP3, DWELL_LVL_8_UP3 },
				{ DWELL_LVL_1_UP4, DWELL_LVL_2_UP4, DWELL_LVL_3_UP4, DWELL_LVL_4_UP4, DWELL_LVL_5_UP4, DWELL_LVL_6_UP4, DWELL_LVL_7_UP4, DWELL_LVL_8_UP4 },
				{ DWELL_LVL_1_UP5, DWELL_LVL_2_UP5, DWELL_LVL_3_UP5, DWELL_LVL_4_UP5, DWELL_LVL_5_UP5, DWELL_LVL_6_UP5, DWELL_LVL_7_UP5, DWELL_LVL_8_UP5 }
			}};

		return allDwellings;
	}

public:
	using StaticIdentifierWithEnum<BuildingID, BuildingIDBase>::StaticIdentifierWithEnum;

	static BuildingID HALL_LEVEL(unsigned int level)
	{
		assert(level < 4);
		return BuildingID(Type::VILLAGE_HALL + level);
	}
	static BuildingID FORT_LEVEL(unsigned int level)
	{
		assert(level < 3);
		return BuildingID(Type::FORT + level);
	}

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	int getMagesGuildLevel() const
	{
		switch (toEnum())
		{
			case Type::MAGES_GUILD_1: return 1;
			case Type::MAGES_GUILD_2: return 2;
			case Type::MAGES_GUILD_3: return 3;
			case Type::MAGES_GUILD_4: return 4;
			case Type::MAGES_GUILD_5: return 5;
		}
		throw std::runtime_error("Call to getMageGuildLevel with building '" + std::to_string(getNum()) +"' that is not mages guild!");
	}

	static BuildingID getDwellingFromLevel(const int levelIndex, const int upgradeIndex)
	{
		if (upgradeIndex >= getDwellings().size() || levelIndex >= getDwellings()[upgradeIndex].size())
			return Type::NONE;

		return getDwellings().at(upgradeIndex).at(levelIndex);
	}

	/// @return 0 for the first one, going up to the supported no. of dwellings - 1
	static int getLevelIndexFromDwelling(BuildingID dwelling)
	{
		for (const auto & level : getDwellings())
		{
			auto it = std::find(level.begin(), level.end(), dwelling);
			if (it != level.end())
				return std::distance(level.begin(), it);
		}

		throw std::runtime_error("Call to getLevelFromDwelling with building '" + std::to_string(dwelling.num) +"' that is not dwelling!");
	}

	/// @return 0 for no upgrade, 1 for the first one, going up to the supported no. of upgrades
	static int getUpgradeNoFromDwelling(BuildingID dwelling)
	{
		const auto & dwellings = getDwellings();

		for(int i = 0; i < dwellings.size(); i++)
		{
			if (vstd::contains(dwellings[i], dwelling))
				return i;
		}

		throw std::runtime_error("Call to getUpgradedFromDwelling with building '" + std::to_string(dwelling.num) +"' that is not dwelling!");
	}

	static void advanceDwelling(BuildingID & dwelling)
	{
		int levelIndex = getLevelIndexFromDwelling(dwelling);
		int upgradeNo = getUpgradeNoFromDwelling(dwelling);
		dwelling = getDwellingFromLevel(levelIndex, upgradeNo + 1);
	}

	bool isDwelling() const
	{
		for (const auto & level : getDwellings())
		{
			if (vstd::contains(level, BuildingID(num)))
				return true;
		}
		return false;
	}
};

class MapObjectBaseID : public IdentifierBase
{
public:
	enum Type
	{
		NO_OBJ = -1,

		NOTHING = 0,
		ALTAR_OF_SACRIFICE = 2,
		ANCHOR_POINT = 3,
		ARENA = 4,
		ARTIFACT = 5,
		PANDORAS_BOX = 6,
		BLACK_MARKET = 7,
		BOAT = 8,
		BORDERGUARD = 9,
		KEYMASTER = 10,
		BUOY = 11,
		CAMPFIRE = 12,
		CARTOGRAPHER = 13,
		SWAN_POND = 14,
		COVER_OF_DARKNESS = 15,
		CREATURE_BANK = 16,
		CREATURE_GENERATOR1 = 17,
		CREATURE_GENERATOR2 = 18,
		CREATURE_GENERATOR3 = 19,
		CREATURE_GENERATOR4 = 20,
		CURSED_GROUND1 = 21,
		CORPSE = 22,
		MARLETTO_TOWER = 23,
		DERELICT_SHIP = 24,
		DRAGON_UTOPIA = 25,
		EVENT = 26,
		EYE_OF_MAGI = 27,
		FAERIE_RING = 28,
		FLOTSAM = 29,
		FOUNTAIN_OF_FORTUNE = 30,
		FOUNTAIN_OF_YOUTH = 31,
		GARDEN_OF_REVELATION = 32,
		GARRISON = 33,
		HERO = 34,
		HILL_FORT = 35,
		GRAIL = 36,
		HUT_OF_MAGI = 37,
		IDOL_OF_FORTUNE = 38,
		LEAN_TO = 39,
		LIBRARY_OF_ENLIGHTENMENT = 41,
		LIGHTHOUSE = 42,
		MONOLITH_ONE_WAY_ENTRANCE = 43,
		MONOLITH_ONE_WAY_EXIT = 44,
		MONOLITH_TWO_WAY = 45,
		MAGIC_PLAINS1 = 46,
		SCHOOL_OF_MAGIC = 47,
		MAGIC_SPRING = 48,
		MAGIC_WELL = 49,
		MARKET_OF_TIME = 50,
		MERCENARY_CAMP = 51,
		MERMAID = 52,
		MINE = 53,
		MONSTER = 54,
		MYSTICAL_GARDEN = 55,
		OASIS = 56,
		OBELISK = 57,
		REDWOOD_OBSERVATORY = 58,
		OCEAN_BOTTLE = 59,
		PILLAR_OF_FIRE = 60,
		STAR_AXIS = 61,
		PRISON = 62,
		PYRAMID = 63,//subtype 0
		WOG_OBJECT = 63,//subtype > 0
		RALLY_FLAG = 64,
		RANDOM_ART = 65,
		RANDOM_TREASURE_ART = 66,
		RANDOM_MINOR_ART = 67,
		RANDOM_MAJOR_ART = 68,
		RANDOM_RELIC_ART = 69,
		RANDOM_HERO = 70,
		RANDOM_MONSTER = 71,
		RANDOM_MONSTER_L1 = 72,
		RANDOM_MONSTER_L2 = 73,
		RANDOM_MONSTER_L3 = 74,
		RANDOM_MONSTER_L4 = 75,
		RANDOM_RESOURCE = 76,
		RANDOM_TOWN = 77,
		REFUGEE_CAMP = 78,
		RESOURCE = 79,
		SANCTUARY = 80,
		SCHOLAR = 81,
		SEA_CHEST = 82,
		SEER_HUT = 83,
		CRYPT = 84,
		SHIPWRECK = 85,
		SHIPWRECK_SURVIVOR = 86,
		SHIPYARD = 87,
		SHRINE_OF_MAGIC_INCANTATION = 88,
		SHRINE_OF_MAGIC_GESTURE = 89,
		SHRINE_OF_MAGIC_THOUGHT = 90,
		SIGN = 91,
		SIRENS = 92,
		SPELL_SCROLL = 93,
		STABLES = 94,
		TAVERN = 95,
		TEMPLE = 96,
		DEN_OF_THIEVES = 97,
		TOWN = 98,
		TRADING_POST = 99,
		LEARNING_STONE = 100,
		TREASURE_CHEST = 101,
		TREE_OF_KNOWLEDGE = 102,
		SUBTERRANEAN_GATE = 103,
		UNIVERSITY = 104,
		WAGON = 105,
		WAR_MACHINE_FACTORY = 106,
		SCHOOL_OF_WAR = 107,
		WARRIORS_TOMB = 108,
		WATER_WHEEL = 109,
		WATERING_HOLE = 110,
		WHIRLPOOL = 111,
		WINDMILL = 112,
		WITCH_HUT = 113,
		BRUSH = 114, // TODO: How does it look like?
		BUSH = 115,
		CACTUS = 116,
		CANYON = 117,
		CRATER = 118,
		DEAD_VEGETATION = 119,
		FLOWERS = 120,
		FROZEN_LAKE = 121,
		HEDGE = 122,
		HILL = 123,
		HOLE = 124,
		KELP = 125,
		LAKE = 126,
		LAVA_FLOW = 127,
		LAVA_LAKE = 128,
		MUSHROOMS = 129,
		LOG = 130,
		MANDRAKE = 131,
		MOSS = 132,
		MOUND = 133,
		MOUNTAIN = 134,
		OAK_TREES = 135,
		OUTCROPPING = 136,
		PINE_TREES = 137,
		PLANT = 138,
		RIVER_DELTA = 143,
		HOTA_CUSTOM_OBJECT_1 = 145,
		HOTA_CUSTOM_OBJECT_2 = 146,
		ROCK = 147,
		SAND_DUNE = 148,
		SAND_PIT = 149,
		SHRUB = 150,
		SKULL = 151,
		STALAGMITE = 152,
		STUMP = 153,
		TAR_PIT = 154,
		TREES = 155,
		VINE = 156,
		VOLCANIC_VENT = 157,
		VOLCANO = 158,
		WILLOW_TREES = 159,
		YUCCA_TREES = 160,
		REEF = 161,
		RANDOM_MONSTER_L5 = 162,
		RANDOM_MONSTER_L6 = 163,
		RANDOM_MONSTER_L7 = 164,
		BORDER_GATE = 212,
		FREELANCERS_GUILD = 213,
		HERO_PLACEHOLDER = 214,
		QUEST_GUARD = 215,
		RANDOM_DWELLING = 216,
		RANDOM_DWELLING_LVL = 217, //subtype = creature level
		RANDOM_DWELLING_FACTION = 218, //subtype = faction
		GARRISON2 = 219,
		ABANDONED_MINE = 220,
		TRADING_POST_SNOW = 221,
		CLOVER_FIELD = 222,
		CURSED_GROUND2 = 223,
		EVIL_FOG = 224,
		FAVORABLE_WINDS = 225,
		FIERY_FIELDS = 226,
		HOLY_GROUNDS = 227,
		LUCID_POOLS = 228,
		MAGIC_CLOUDS = 229,
		MAGIC_PLAINS2 = 230,
		ROCKLANDS = 231,
	};
};

class DLL_LINKAGE MapObjectID : public EntityIdentifierWithEnum<MapObjectID, MapObjectBaseID>
{
public:
	using EntityIdentifierWithEnum<MapObjectID, MapObjectBaseID>::EntityIdentifierWithEnum;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	// TODO: Remove
	constexpr operator int32_t () const
	{
		return num;
	}
};

class DLL_LINKAGE MapObjectSubID : public Identifier<MapObjectSubID>
{
public:
	constexpr MapObjectSubID(const IdentifierBase & value):
		Identifier<MapObjectSubID>(value.getNum())
	{}
	constexpr MapObjectSubID(int32_t value = -1):
		Identifier<MapObjectSubID>(value)
	{}

	MapObjectSubID & operator =(int32_t value)
	{
		this->num = value;
		return *this;
	}

	MapObjectSubID & operator =(const IdentifierBase & value)
	{
		this->num = value.getNum();
		return *this;
	}

	static si32 decode(MapObjectID primaryID, const std::string & identifier);
	static std::string encode(MapObjectID primaryID, si32 index);

	// TODO: Remove
	constexpr operator int32_t () const
	{
		return num;
	}

	template <typename Handler>
	void serializeIdentifier(Handler &h, const MapObjectID & primaryID)
	{
		std::string secondaryStringID;

		if (h.saving)
			secondaryStringID = encode(primaryID, num);

		h & secondaryStringID;

		if (!h.saving)
			num = decode(primaryID, secondaryStringID);
	}
};

class DLL_LINKAGE RoadId : public EntityIdentifier<RoadId>
{
public:
	using EntityIdentifier<RoadId>::EntityIdentifier;
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();

	static const RoadId NO_ROAD;
	static const RoadId DIRT_ROAD;
	static const RoadId GRAVEL_ROAD;
	static const RoadId COBBLESTONE_ROAD;

	const RoadType * toEntity(const Services * service) const;
};

class DLL_LINKAGE RiverId : public EntityIdentifier<RiverId>
{
public:
	using EntityIdentifier<RiverId>::EntityIdentifier;
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();

	static const RiverId NO_RIVER;
	static const RiverId WATER_RIVER;
	static const RiverId ICY_RIVER;
	static const RiverId MUD_RIVER;
	static const RiverId LAVA_RIVER;

	const RiverType * toEntity(const Services * service) const;
};

class DLL_LINKAGE EPathfindingLayerBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		LAND = 0, SAIL = 1, WATER, AIR, NUM_LAYERS, WRONG, AUTO
	};
};

class EPathfindingLayer : public StaticIdentifierWithEnum<EPathfindingLayer, EPathfindingLayerBase>
{
public:
	using StaticIdentifierWithEnum<EPathfindingLayer, EPathfindingLayerBase>::StaticIdentifierWithEnum;
};

class ArtifactPositionBase : public IdentifierBase
{
public:
	enum Type
	{
		TRANSITION_POS = -3,
		FIRST_AVAILABLE = -2,
		PRE_FIRST = -1, //sometimes used as error, sometimes as first free in backpack
		
		// Hero
		HEAD, SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, //5
		RIGHT_RING, LEFT_RING, FEET, //8
		MISC1, MISC2, MISC3, MISC4, //12
		MACH1, MACH2, MACH3, MACH4, //16
		SPELLBOOK, MISC5, //18
		BACKPACK_START = 19,
		
		// Creatures
		CREATURE_SLOT = 0,
		
		// Commander
		COMMANDER1 = 0, COMMANDER2, COMMANDER3, COMMANDER4, COMMANDER5, COMMANDER6, COMMANDER7, COMMANDER8, COMMANDER9,

		// Altar
		ALTAR = BACKPACK_START
	};

	static_assert(MISC5 < BACKPACK_START, "incorrect number of artifact slots");

	DLL_LINKAGE static si32 decode(const std::string & identifier);
	DLL_LINKAGE static std::string encode(const si32 index);
	DLL_LINKAGE static std::string entityType();
};

class ArtifactPosition : public StaticIdentifierWithEnum<ArtifactPosition, ArtifactPositionBase>
{
public:
	using StaticIdentifierWithEnum<ArtifactPosition, ArtifactPositionBase>::StaticIdentifierWithEnum;

	// TODO: Remove
	constexpr operator int32_t () const
	{
		return num;
	}
};

class ArtifactIDBase : public IdentifierBase
{
public:
	enum Type
	{
		NONE = -1,
		SPELLBOOK = 0,
		SPELL_SCROLL = 1,
		GRAIL = 2,
		CATAPULT = 3,
		BALLISTA = 4,
		AMMO_CART = 5,
		FIRST_AID_TENT = 6,
		VIAL_OF_DRAGON_BLOOD = 127,
		ARMAGEDDONS_BLADE = 128,
		ANGELIC_ALLIANCE = 129,
		TITANS_THUNDER = 135,
		ART_SELECTION = 144,
		ART_LOCK = 145, // FIXME: We must get rid of this one since it's conflict with artifact from mods. See issue 2455
	};

	DLL_LINKAGE const CArtifact * toArtifact() const;
	DLL_LINKAGE const Artifact * toEntity(const Services * service) const;
};

class ArtifactID : public EntityIdentifierWithEnum<ArtifactID, ArtifactIDBase>
{
public:
	using EntityIdentifierWithEnum<ArtifactID, ArtifactIDBase>::EntityIdentifierWithEnum;

	///json serialization helpers
	DLL_LINKAGE static si32 decode(const std::string & identifier);
	DLL_LINKAGE static std::string encode(const si32 index);
	static std::string entityType();
};

class CreatureIDBase : public IdentifierBase
{
public:
	enum Type
	{
		NONE = -1,
		ARCHER = 2, // for debug / fallback
		IMP = 42, // for Deity of Fire
		FAMILIAR = 43, // for Deity of Fire
		SKELETON = 56, // for Skeleton Transformer
		TROGLODYTES = 70, // for Abandoned Mine
		MEDUSA = 76, // for Siege UI workaround
		AIR_ELEMENTAL = 112, // for tests
		FIRE_ELEMENTAL = 114, // for tests
		PSYCHIC_ELEMENTAL = 120, // for hardcoded ability
		MAGIC_ELEMENTAL = 121, // for hardcoded ability
		AZURE_DRAGON = 132,
		CATAPULT = 145,
		BALLISTA = 146,
		FIRST_AID_TENT = 147,
		AMMO_CART = 148,
		ARROW_TOWERS = 149
	};

	DLL_LINKAGE const CCreature * toCreature() const;
	DLL_LINKAGE const Creature * toEntity(const Services * services) const;
	DLL_LINKAGE const Creature * toEntity(const CreatureService * creatures) const;
};

class DLL_LINKAGE CreatureID : public EntityIdentifierWithEnum<CreatureID, CreatureIDBase>
{
public:
	using EntityIdentifierWithEnum<CreatureID, CreatureIDBase>::EntityIdentifierWithEnum;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class DLL_LINKAGE SpellIDBase : public IdentifierBase
{
public:
	enum Type
	{
		// Special ID's
		SPELLBOOK_PRESET = -3,
		PRESET = -2,
		NONE = -1,

		// Adventure map spells
		SUMMON_BOAT [[deprecated("check for spell mechanics instead of spell ID")]] = 0,
		SCUTTLE_BOAT [[deprecated("check for spell mechanics instead of spell ID")]] = 1,
		VISIONS [[deprecated("check for spell mechanics instead of spell ID")]] = 2,
		VIEW_EARTH [[deprecated("check for spell mechanics instead of spell ID")]] = 3,
		DISGUISE [[deprecated("check for spell mechanics instead of spell ID")]] = 4,
		VIEW_AIR [[deprecated("check for spell mechanics instead of spell ID")]] = 5,
		FLY [[deprecated("check for spell mechanics instead of spell ID")]] = 6,
		WATER_WALK [[deprecated("check for spell mechanics instead of spell ID")]] = 7,
		DIMENSION_DOOR [[deprecated("check for spell mechanics instead of spell ID")]] = 8,
		TOWN_PORTAL [[deprecated("check for spell mechanics instead of spell ID")]] = 9,

		// Combat spells
		QUICKSAND = 10,
		LAND_MINE = 11,
		FORCE_FIELD = 12,
		FIRE_WALL = 13,
		EARTHQUAKE = 14,
		MAGIC_ARROW = 15,
		ICE_BOLT = 16,
		LIGHTNING_BOLT = 17,
		IMPLOSION = 18,
		CHAIN_LIGHTNING = 19,
		FROST_RING = 20,
		FIREBALL = 21,
		INFERNO = 22,
		METEOR_SHOWER = 23,
		DEATH_RIPPLE = 24,
		DESTROY_UNDEAD = 25,
		ARMAGEDDON = 26,
		SHIELD = 27,
		AIR_SHIELD = 28,
		FIRE_SHIELD = 29,
		PROTECTION_FROM_AIR = 30,
		PROTECTION_FROM_FIRE = 31,
		PROTECTION_FROM_WATER = 32,
		PROTECTION_FROM_EARTH = 33,
		ANTI_MAGIC = 34,
		DISPEL = 35,
		MAGIC_MIRROR = 36,
		CURE = 37,
		RESURRECTION = 38,
		ANIMATE_DEAD = 39,
		SACRIFICE = 40,
		BLESS = 41,
		CURSE = 42,
		BLOODLUST = 43,
		PRECISION = 44,
		WEAKNESS = 45,
		STONE_SKIN = 46,
		DISRUPTING_RAY = 47,
		PRAYER = 48,
		MIRTH = 49,
		SORROW = 50,
		FORTUNE = 51,
		MISFORTUNE = 52,
		HASTE = 53,
		SLOW = 54,
		SLAYER = 55,
		FRENZY = 56,
		TITANS_LIGHTNING_BOLT = 57,
		COUNTERSTRIKE = 58,
		BERSERK = 59,
		HYPNOTIZE = 60,
		FORGETFULNESS = 61,
		BLIND = 62,
		TELEPORT = 63,
		REMOVE_OBSTACLE = 64,
		CLONE = 65,
		SUMMON_FIRE_ELEMENTAL = 66,
		SUMMON_EARTH_ELEMENTAL = 67,
		SUMMON_WATER_ELEMENTAL = 68,
		SUMMON_AIR_ELEMENTAL = 69,

		// Creature abilities
		STONE_GAZE = 70,
		POISON = 71,
		BIND = 72,
		DISEASE = 73,
		PARALYZE = 74,
		AGE = 75,
		DEATH_CLOUD = 76,
		THUNDERBOLT = 77,
		DISPEL_HELPFUL_SPELLS = 78,
		DEATH_STARE = 79,
		ACID_BREATH_DEFENSE = 80,
		ACID_BREATH_DAMAGE = 81,

		// Special ID's
		FIRST_NON_SPELL = 70,
		AFTER_LAST = 82
	};

	const CSpell * toSpell() const; //deprecated
	const spells::Spell * toEntity(const Services * service) const;
	const spells::Spell * toEntity(const spells::Service * service) const;
};

class DLL_LINKAGE SpellID : public EntityIdentifierWithEnum<SpellID, SpellIDBase>
{
public:
	using EntityIdentifierWithEnum<SpellID, SpellIDBase>::EntityIdentifierWithEnum;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class BattleFieldInfo;
class DLL_LINKAGE BattleField : public EntityIdentifier<BattleField>
{
public:
	using EntityIdentifier<BattleField>::EntityIdentifier;

	static const BattleField NONE;
	const BattleFieldInfo * getInfo() const;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class DLL_LINKAGE BoatId : public EntityIdentifier<BoatId>
{
public:
	using EntityIdentifier<BoatId>::EntityIdentifier;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);

	static const BoatId NONE;
	static const BoatId NECROPOLIS;
	static const BoatId CASTLE;
	static const BoatId FORTRESS;
};

class TerrainIdBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NATIVE_TERRAIN = -4,
		ANY_TERRAIN = -3,
		NONE = -1,
		FIRST_REGULAR_TERRAIN = 0,
		DIRT = 0,
		SAND,
		GRASS,
		SNOW,
		SWAMP,
		ROUGH,
		SUBTERRANEAN,
		LAVA,
		WATER,
		ROCK,
		ORIGINAL_REGULAR_TERRAIN_COUNT = ROCK
	};
};

class DLL_LINKAGE TerrainId : public EntityIdentifierWithEnum<TerrainId, TerrainIdBase>
{
public:
	using EntityIdentifierWithEnum<TerrainId, TerrainIdBase>::EntityIdentifierWithEnum;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
	const TerrainType * toEntity(const Services * service) const;
};

class ObstacleInfo;
class Obstacle : public EntityIdentifier<Obstacle>
{
public:
	using EntityIdentifier<Obstacle>::EntityIdentifier;
	DLL_LINKAGE const ObstacleInfo * getInfo() const;
};

class DLL_LINKAGE SpellSchool : public StaticIdentifier<SpellSchool>
{
public:
	using StaticIdentifier<SpellSchool>::StaticIdentifier;

	static const SpellSchool ANY;
	static const SpellSchool AIR;
	static const SpellSchool FIRE;
	static const SpellSchool WATER;
	static const SpellSchool EARTH;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class GameResIDBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		WOOD = 0,
		MERCURY,
		ORE,
		SULFUR,
		CRYSTAL,
		GEMS,
		GOLD,
		COUNT,

		WOOD_AND_ORE = -4,  // special case for town bonus resource
		COMMON = -3, // campaign bonus
		RARE = -2, // campaign bonus
		NONE = -1
	};
};

class DLL_LINKAGE GameResID : public StaticIdentifierWithEnum<GameResID, GameResIDBase>
{
public:
	using StaticIdentifierWithEnum<GameResID, GameResIDBase>::StaticIdentifierWithEnum;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();

	const Resource * toResource() const;
	const ResourceType * toEntity(const Services * services) const;
};

class DLL_LINKAGE BuildingTypeUniqueID : public Identifier<BuildingTypeUniqueID>
{
public:
	BuildingTypeUniqueID(FactionID faction, BuildingID building );

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);

	BuildingID getBuilding() const;
	FactionID getFaction() const;

	using Identifier<BuildingTypeUniqueID>::Identifier;

	template <typename Handler>
	void serialize(Handler & h)
	{
		FactionID faction = getFaction();
		BuildingID building = getBuilding();

		h & faction;
		h & building;

		if (!h.saving)
			*this = BuildingTypeUniqueID(faction, building);
	}
};

class DLL_LINKAGE CampaignScenarioID : public StaticIdentifier<CampaignScenarioID>
{
public:
	using StaticIdentifier<CampaignScenarioID>::StaticIdentifier;

	static si32 decode(const std::string & identifier);
	static std::string encode(int32_t index);

	static const CampaignScenarioID NONE;
};

class DLL_LINKAGE CampaignRegionID : public StaticIdentifier<CampaignRegionID>
{
public:
	using StaticIdentifier<CampaignRegionID>::StaticIdentifier;
};


// Deprecated
// TODO: remove
using Obj = MapObjectID;
using ETownType = FactionID;
using EGameResID = GameResID;
using River = RiverId;
using Road = RoadId;
using ETerrainId = TerrainId;

VCMI_LIB_NAMESPACE_END
