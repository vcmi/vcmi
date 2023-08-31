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

#include "NumericConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class Artifact;
class ArtifactService;
class Creature;
class CreatureService;

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
class CGameInfoCallback;
class CNonConstInfoCallback;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IdentifierBase
{
protected:
	constexpr IdentifierBase(int32_t value = -1 ):
		num(value)
	{}

	~IdentifierBase() = default;
public:
	int32_t num;

	constexpr int32_t getNum() const
	{
		return num;
	}

	struct hash
	{
		size_t operator()(const IdentifierBase & id) const
		{
			return std::hash<int>()(id.num);
		}
	};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & num;
	}

	constexpr void advance(int change)
	{
		num += change;
	}

	constexpr operator int32_t () const
	{
		return num;
	}

	friend std::ostream& operator<<(std::ostream& os, const IdentifierBase& dt)
	{
		return os << dt.num;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Note: use template to force different type, blocking any Identifier<A> <=> Identifier<B> comparisons
template<typename FinalClass>
class Identifier : public IdentifierBase
{
	using BaseClass = IdentifierBase;
public:
	constexpr Identifier(int32_t _num = -1)
		:IdentifierBase(_num)
	{}

	constexpr bool operator == (const Identifier & b) const { return BaseClass::num == b.num; }
	constexpr bool operator <= (const Identifier & b) const { return BaseClass::num <= b.num; }
	constexpr bool operator >= (const Identifier & b) const { return BaseClass::num >= b.num; }
	constexpr bool operator != (const Identifier & b) const { return BaseClass::num != b.num; }
	constexpr bool operator <  (const Identifier & b) const { return BaseClass::num <  b.num; }
	constexpr bool operator >  (const Identifier & b) const { return BaseClass::num >  b.num; }

	constexpr FinalClass & operator++()
	{
		++BaseClass::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass operator++(int)
	{
		FinalClass ret(num);
		++BaseClass::num;
		return ret;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename FinalClass, typename BaseClass>
class IdentifierWithEnum : public BaseClass
{
	using EnumType = typename BaseClass::Type;

	static_assert(std::is_same_v<std::underlying_type_t<EnumType>, int32_t>, "Entity Identifier must use int32_t");
public:
	constexpr EnumType toEnum() const
	{
		return static_cast<EnumType>(BaseClass::num);
	}

	constexpr IdentifierWithEnum(const EnumType & enumValue)
	{
		BaseClass::num = static_cast<int32_t>(enumValue);
	}

	constexpr IdentifierWithEnum(int32_t _num = -1)
	{
		BaseClass::num = _num;
	}

	constexpr bool operator == (const EnumType & b) const { return BaseClass::num == static_cast<int32_t>(b); }
	constexpr bool operator <= (const EnumType & b) const { return BaseClass::num <= static_cast<int32_t>(b); }
	constexpr bool operator >= (const EnumType & b) const { return BaseClass::num >= static_cast<int32_t>(b); }
	constexpr bool operator != (const EnumType & b) const { return BaseClass::num != static_cast<int32_t>(b); }
	constexpr bool operator <  (const EnumType & b) const { return BaseClass::num <  static_cast<int32_t>(b); }
	constexpr bool operator >  (const EnumType & b) const { return BaseClass::num >  static_cast<int32_t>(b); }

	constexpr bool operator == (const IdentifierWithEnum & b) const { return BaseClass::num == b.num; }
	constexpr bool operator <= (const IdentifierWithEnum & b) const { return BaseClass::num <= b.num; }
	constexpr bool operator >= (const IdentifierWithEnum & b) const { return BaseClass::num >= b.num; }
	constexpr bool operator != (const IdentifierWithEnum & b) const { return BaseClass::num != b.num; }
	constexpr bool operator <  (const IdentifierWithEnum & b) const { return BaseClass::num <  b.num; }
	constexpr bool operator >  (const IdentifierWithEnum & b) const { return BaseClass::num >  b.num; }

	constexpr FinalClass & operator++()
	{
		++BaseClass::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass operator++(int)
	{
		FinalClass ret(BaseClass::num);
		++BaseClass::num;
		return ret;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ArtifactInstanceID : public Identifier<ArtifactInstanceID>
{
public:
	using Identifier<ArtifactInstanceID>::Identifier;
};

class QueryID : public Identifier<QueryID>
{
public:
	using Identifier<QueryID>::Identifier;
	DLL_LINKAGE static const QueryID NONE;
};

class BattleID : public Identifier<BattleID>
{
public:
	using Identifier<BattleID>::Identifier;
	DLL_LINKAGE static const BattleID NONE;
};
class ObjectInstanceID : public Identifier<ObjectInstanceID>
{
public:
	using Identifier<ObjectInstanceID>::Identifier;
	DLL_LINKAGE static const ObjectInstanceID NONE;
};

class HeroClassID : public Identifier<HeroClassID>
{
public:
	using Identifier<HeroClassID>::Identifier;
};

class HeroTypeID : public Identifier<HeroTypeID>
{
public:
	using Identifier<HeroTypeID>::Identifier;
	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();

	DLL_LINKAGE static const HeroTypeID NONE;
};

class SlotID : public Identifier<SlotID>
{
public:
	using Identifier<SlotID>::Identifier;

	DLL_LINKAGE static const SlotID COMMANDER_SLOT_PLACEHOLDER;
	DLL_LINKAGE static const SlotID SUMMONED_SLOT_PLACEHOLDER; ///<for all summoned creatures, only during battle
	DLL_LINKAGE static const SlotID WAR_MACHINES_SLOT; ///<for all war machines during battle
	DLL_LINKAGE static const SlotID ARROW_TOWERS_SLOT; ///<for all arrow towers during battle

	bool validSlot() const
	{
		return getNum() >= 0  &&  getNum() < GameConstants::ARMY_SIZE;
	}
};

class DLL_LINKAGE PlayerColor : public Identifier<PlayerColor>
{
public:
	using Identifier<PlayerColor>::Identifier;

	enum EPlayerColor
	{
		PLAYER_LIMIT_I = 8,
	};

	static const PlayerColor SPECTATOR; //252
	static const PlayerColor CANNOT_DETERMINE; //253
	static const PlayerColor UNFLAGGABLE; //254 - neutral objects (pandora, banks)
	static const PlayerColor NEUTRAL; //255
	static const PlayerColor PLAYER_LIMIT; //player limit per map

	bool isValidPlayer() const; //valid means < PLAYER_LIMIT (especially non-neutral)
	bool isSpectator() const;

	std::string toString() const;

	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class TeamID : public Identifier<TeamID>
{
public:
	using Identifier<TeamID>::Identifier;

	DLL_LINKAGE static const TeamID NO_TEAM;
};

class TeleportChannelID : public Identifier<TeleportChannelID>
{
public:
	using Identifier<TeleportChannelID>::Identifier;
};

class SecondarySkillBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		WRONG = -2,
		DEFAULT = -1,
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

class SecondarySkill : public IdentifierWithEnum<SecondarySkill, SecondarySkillBase>
{
public:
	using IdentifierWithEnum<SecondarySkill, SecondarySkillBase>::IdentifierWithEnum;
};

class DLL_LINKAGE FactionID : public Identifier<FactionID>
{
public:
	using Identifier<FactionID>::Identifier;

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
	static std::string entityType();
};

class BuildingIDBase : public IdentifierBase
{
public:
	//Quite useful as long as most of building mechanics hardcoded
	// NOTE: all building with completely configurable mechanics will be removed from list
	enum Type
	{
		DEFAULT = -50,
		HORDE_PLACEHOLDER7 = -36,
		HORDE_PLACEHOLDER6 = -35,
		HORDE_PLACEHOLDER5 = -34,
		HORDE_PLACEHOLDER4 = -33,
		HORDE_PLACEHOLDER3 = -32,
		HORDE_PLACEHOLDER2 = -31,
		HORDE_PLACEHOLDER1 = -30,
		NONE = -1,
		FIRST_REGULAR_ID = 0,
		MAGES_GUILD_1 = 0,  MAGES_GUILD_2, MAGES_GUILD_3,     MAGES_GUILD_4,   MAGES_GUILD_5,
		TAVERN,         SHIPYARD,      FORT,              CITADEL,         CASTLE,
		VILLAGE_HALL,   TOWN_HALL,     CITY_HALL,         CAPITOL,         MARKETPLACE,
		RESOURCE_SILO,  BLACKSMITH,    SPECIAL_1,         HORDE_1,         HORDE_1_UPGR,
		SHIP,           SPECIAL_2,     SPECIAL_3,         SPECIAL_4,       HORDE_2,
		HORDE_2_UPGR,   GRAIL,         EXTRA_TOWN_HALL,   EXTRA_CITY_HALL, EXTRA_CAPITOL,
		DWELL_FIRST=30, DWELL_LVL_2, DWELL_LVL_3, DWELL_LVL_4, DWELL_LVL_5, DWELL_LVL_6, DWELL_LAST=36,
		DWELL_UP_FIRST=37,  DWELL_LVL_2_UP, DWELL_LVL_3_UP, DWELL_LVL_4_UP, DWELL_LVL_5_UP,
		DWELL_LVL_6_UP, DWELL_UP_LAST=43,

		DWELL_LVL_1 = DWELL_FIRST,
		DWELL_LVL_7 = DWELL_LAST,
		DWELL_LVL_1_UP = DWELL_UP_FIRST,
		DWELL_LVL_7_UP = DWELL_UP_LAST,

		DWELL_UP2_FIRST = DWELL_LVL_7_UP + 1,

//		//Special buildings for towns.
		CASTLE_GATE   = SPECIAL_3, //Inferno
		FREELANCERS_GUILD = SPECIAL_2, //Stronghold
		ARTIFACT_MERCHANT = SPECIAL_1,

	};

	bool IsSpecialOrGrail() const
	{
		return num == SPECIAL_1 || num == SPECIAL_2 || num == SPECIAL_3 || num == SPECIAL_4 || num == GRAIL;
	}
};

class BuildingID : public IdentifierWithEnum<BuildingID, BuildingIDBase>
{
public:
	using IdentifierWithEnum<BuildingID, BuildingIDBase>::IdentifierWithEnum;
};

class ObjBase : public IdentifierBase
{
public:
	enum Type
	{
		NO_OBJ = -1,
		ALTAR_OF_SACRIFICE [[deprecated]] = 2,
		ANCHOR_POINT = 3,
		ARENA = 4,
		ARTIFACT = 5,
		PANDORAS_BOX = 6,
		BLACK_MARKET [[deprecated]] = 7,
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
		TRADING_POST [[deprecated]] = 99,
		LEARNING_STONE = 100,
		TREASURE_CHEST = 101,
		TREE_OF_KNOWLEDGE = 102,
		SUBTERRANEAN_GATE = 103,
		UNIVERSITY [[deprecated]] = 104,
		WAGON = 105,
		WAR_MACHINE_FACTORY = 106,
		SCHOOL_OF_WAR = 107,
		WARRIORS_TOMB = 108,
		WATER_WHEEL = 109,
		WATERING_HOLE = 110,
		WHIRLPOOL = 111,
		WINDMILL = 112,
		WITCH_HUT = 113,
		HOLE = 124,
		RANDOM_MONSTER_L5 = 162,
		RANDOM_MONSTER_L6 = 163,
		RANDOM_MONSTER_L7 = 164,
		BORDER_GATE = 212,
		FREELANCERS_GUILD [[deprecated]] = 213,
		HERO_PLACEHOLDER = 214,
		QUEST_GUARD = 215,
		RANDOM_DWELLING = 216,
		RANDOM_DWELLING_LVL = 217, //subtype = creature level
		RANDOM_DWELLING_FACTION = 218, //subtype = faction
		GARRISON2 = 219,
		ABANDONED_MINE = 220,
		TRADING_POST_SNOW [[deprecated]] = 221,
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

class Obj : public IdentifierWithEnum<Obj, ObjBase>
{
public:
	using IdentifierWithEnum<Obj, ObjBase>::IdentifierWithEnum;
};

class DLL_LINKAGE RoadId : public Identifier<RoadId>
{
public:
	using Identifier<RoadId>::Identifier;

	static const RoadId NO_ROAD;
	static const RoadId DIRT_ROAD;
	static const RoadId GRAVEL_ROAD;
	static const RoadId COBBLESTONE_ROAD;
};

class DLL_LINKAGE RiverId : public Identifier<RiverId>
{
public:
	using Identifier<RiverId>::Identifier;

	static const RiverId NO_RIVER;
	static const RiverId WATER_RIVER;
	static const RiverId ICY_RIVER;
	static const RiverId MUD_RIVER;
	static const RiverId LAVA_RIVER;
};

class DLL_LINKAGE EPathfindingLayerBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		LAND = 0, SAIL = 1, WATER, AIR, NUM_LAYERS, WRONG, AUTO
	};
};

class EPathfindingLayer : public IdentifierWithEnum<EPathfindingLayer, EPathfindingLayerBase>
{
public:
	using IdentifierWithEnum<EPathfindingLayer, EPathfindingLayerBase>::IdentifierWithEnum;
};

class ArtifactPositionBase : public IdentifierBase
{
public:
	enum Type
	{
		TRANSITION_POS = -3,
		FIRST_AVAILABLE = -2,
		PRE_FIRST = -1, //sometimes used as error, sometimes as first free in backpack
		HEAD, SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, //5
		RIGHT_RING, LEFT_RING, FEET, //8
		MISC1, MISC2, MISC3, MISC4, //12
		MACH1, MACH2, MACH3, MACH4, //16
		SPELLBOOK, MISC5, //18
		AFTER_LAST,
		//cres
		CREATURE_SLOT = 0,
		COMMANDER1 = 0, COMMANDER2, COMMANDER3, COMMANDER4, COMMANDER5, COMMANDER6, COMMANDER_AFTER_LAST,

		BACKPACK_START = 19
	};

	static_assert (AFTER_LAST == BACKPACK_START, "incorrect number of artifact slots");

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class ArtifactPosition : public IdentifierWithEnum<ArtifactPosition, ArtifactPositionBase>
{
public:
	using IdentifierWithEnum<ArtifactPosition, ArtifactPositionBase>::IdentifierWithEnum;
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
		TITANS_THUNDER = 135,
		ART_SELECTION = 144,
		ART_LOCK = 145, // FIXME: We must get rid of this one since it's conflict with artifact from mods. See issue 2455
	};

	DLL_LINKAGE const CArtifact * toArtifact() const;
	DLL_LINKAGE const Artifact * toArtifact(const ArtifactService * service) const;
};

class ArtifactID : public IdentifierWithEnum<ArtifactID, ArtifactIDBase>
{
public:
	using IdentifierWithEnum<ArtifactID, ArtifactIDBase>::IdentifierWithEnum;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
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
		SKELETON = 56, // for Skeleton Transformer
		BONE_DRAGON = 68, // for Skeleton Transformer
		TROGLODYTES = 70, // for Abandoned Mine
		MEDUSA = 76, // for Siege UI workaround
		HYDRA = 110, // for Skeleton Transformer
		CHAOS_HYDRA = 111, // for Skeleton Transformer
		AIR_ELEMENTAL = 112, // for tests
		FIRE_ELEMENTAL = 114, // for tests
		PSYCHIC_ELEMENTAL = 120, // for hardcoded ability
		MAGIC_ELEMENTAL = 121, // for hardcoded ability
		CATAPULT = 145,
		BALLISTA = 146,
		FIRST_AID_TENT = 147,
		AMMO_CART = 148,
		ARROW_TOWERS = 149
	};

	DLL_LINKAGE const CCreature * toCreature() const;
	DLL_LINKAGE const Creature * toCreature(const CreatureService * creatures) const;
};

class CreatureID : public IdentifierWithEnum<CreatureID, CreatureIDBase>
{
public:
	using IdentifierWithEnum<CreatureID, CreatureIDBase>::IdentifierWithEnum;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class SpellIDBase : public IdentifierBase
{
public:
	enum Type
	{
		// Special ID's
		SPELLBOOK_PRESET = -3,
		PRESET = -2,
		NONE = -1,

		// Adventure map spells
		SUMMON_BOAT = 0,
		SCUTTLE_BOAT = 1,
		VISIONS = 2,
		VIEW_EARTH = 3,
		DISGUISE = 4,
		VIEW_AIR = 5,
		FLY = 6,
		WATER_WALK = 7,
		DIMENSION_DOOR = 8,
		TOWN_PORTAL = 9,

		// Combar spells
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

	DLL_LINKAGE const CSpell * toSpell() const; //deprecated
	DLL_LINKAGE const spells::Spell * toSpell(const spells::Service * service) const;
};

class SpellID : public IdentifierWithEnum<SpellID, SpellIDBase>
{
public:
	using IdentifierWithEnum<SpellID, SpellIDBase>::IdentifierWithEnum;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class BattleFieldInfo;
class BattleField : public Identifier<BattleField>
{
public:
	using Identifier<BattleField>::Identifier;

	DLL_LINKAGE static const BattleField NONE;
	DLL_LINKAGE const BattleFieldInfo * getInfo() const;
};

class DLL_LINKAGE BoatId : public Identifier<BoatId>
{
public:
	using Identifier<BoatId>::Identifier;

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

class TerrainId : public IdentifierWithEnum<TerrainId, TerrainIdBase>
{
public:
	using IdentifierWithEnum<TerrainId, TerrainIdBase>::IdentifierWithEnum;

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class ObstacleInfo;
class Obstacle : public Identifier<Obstacle>
{
public:
	using Identifier<Obstacle>::Identifier;
	DLL_LINKAGE const ObstacleInfo * getInfo() const;
};

class DLL_LINKAGE SpellSchool : public Identifier<SpellSchool>
{
public:
	using Identifier<SpellSchool>::Identifier;

	static const SpellSchool ANY;
	static const SpellSchool AIR;
	static const SpellSchool FIRE;
	static const SpellSchool WATER;
	static const SpellSchool EARTH;
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
		MITHRIL,
		COUNT,

		WOOD_AND_ORE = 127,  // special case for town bonus resource
		INVALID = -1
	};
};

class GameResID : public IdentifierWithEnum<GameResID, GameResIDBase>
{
public:
	using IdentifierWithEnum<GameResID, GameResIDBase>::IdentifierWithEnum;
};

// Deprecated
// TODO: remove
using ESpellSchool = SpellSchool;
using ETownType = FactionID;
using EGameResID = GameResID;
using River = RiverId;
using Road = RoadId;
using ETerrainId = TerrainId;

VCMI_LIB_NAMESPACE_END
