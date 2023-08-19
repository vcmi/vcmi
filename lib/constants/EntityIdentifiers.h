/*
 * GameConstants.h, part of VCMI engine
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
	using T = IdentifierBase;
public:
	constexpr Identifier(int32_t _num = -1)
		:IdentifierBase(_num)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & T::num;
	}

	constexpr bool operator == (const Identifier & b) const { return T::num == b.num; }
	constexpr bool operator <= (const Identifier & b) const { return T::num <= b.num; }
	constexpr bool operator >= (const Identifier & b) const { return T::num >= b.num; }
	constexpr bool operator != (const Identifier & b) const { return T::num != b.num; }
	constexpr bool operator <  (const Identifier & b) const { return T::num <  b.num; }
	constexpr bool operator >  (const Identifier & b) const { return T::num >  b.num; }

	constexpr FinalClass & operator++()
	{
		++T::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass operator++(int)
	{
		FinalClass ret(num);
		++T::num;
		return ret;
	}

	constexpr operator int32_t () const
	{
		return T::num;
	}

	constexpr void advance(int change)
	{
		T::num += change;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class IdentifierWithEnum : public T
{
	using EnumType = typename T::Type;

	static_assert(std::is_same_v<std::underlying_type_t<EnumType>, int32_t>, "Entity Identifier must use int32_t");
public:
	constexpr int32_t getNum() const
	{
		return T::num;
	}

	constexpr EnumType toEnum() const
	{
		return static_cast<EnumType>(T::num);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & T::num;
	}

	constexpr IdentifierWithEnum(const EnumType & enumValue)
	{
		T::num = static_cast<int32_t>(enumValue);
	}

	constexpr IdentifierWithEnum(int32_t _num = -1)
	{
		T::num = _num;
	}

	constexpr void advance(int change)
	{
		T::num += change;
	}

	constexpr bool operator == (const EnumType & b) const { return T::num == static_cast<int32_t>(b); }
	constexpr bool operator <= (const EnumType & b) const { return T::num <= static_cast<int32_t>(b); }
	constexpr bool operator >= (const EnumType & b) const { return T::num >= static_cast<int32_t>(b); }
	constexpr bool operator != (const EnumType & b) const { return T::num != static_cast<int32_t>(b); }
	constexpr bool operator <  (const EnumType & b) const { return T::num <  static_cast<int32_t>(b); }
	constexpr bool operator >  (const EnumType & b) const { return T::num >  static_cast<int32_t>(b); }

	constexpr bool operator == (const IdentifierWithEnum & b) const { return T::num == b.num; }
	constexpr bool operator <= (const IdentifierWithEnum & b) const { return T::num <= b.num; }
	constexpr bool operator >= (const IdentifierWithEnum & b) const { return T::num >= b.num; }
	constexpr bool operator != (const IdentifierWithEnum & b) const { return T::num != b.num; }
	constexpr bool operator <  (const IdentifierWithEnum & b) const { return T::num <  b.num; }
	constexpr bool operator >  (const IdentifierWithEnum & b) const { return T::num >  b.num; }

	constexpr IdentifierWithEnum & operator++()
	{
		++T::num;
		return *this;
	}

	constexpr IdentifierWithEnum operator++(int)
	{
		IdentifierWithEnum ret(*this);
		++T::num;
		return ret;
	}

	constexpr operator int32_t () const
	{
		return T::num;
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

class PlayerColor : public Identifier<PlayerColor>
{
public:
	using Identifier<PlayerColor>::Identifier;

	enum EPlayerColor
	{
		PLAYER_LIMIT_I = 8,
	};

	using Mask = uint8_t;

	DLL_LINKAGE static const PlayerColor SPECTATOR; //252
	DLL_LINKAGE static const PlayerColor CANNOT_DETERMINE; //253
	DLL_LINKAGE static const PlayerColor UNFLAGGABLE; //254 - neutral objects (pandora, banks)
	DLL_LINKAGE static const PlayerColor NEUTRAL; //255
	DLL_LINKAGE static const PlayerColor PLAYER_LIMIT; //player limit per map

	DLL_LINKAGE bool isValidPlayer() const; //valid means < PLAYER_LIMIT (especially non-neutral)
	DLL_LINKAGE bool isSpectator() const;

	DLL_LINKAGE std::string getStr(bool L10n = false) const;
	DLL_LINKAGE std::string getStrCap(bool L10n = false) const;
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
		PATHFINDING = 0, ARCHERY, LOGISTICS, SCOUTING, DIPLOMACY, NAVIGATION, LEADERSHIP, WISDOM, MYSTICISM,
		LUCK, BALLISTICS, EAGLE_EYE, NECROMANCY, ESTATES, FIRE_MAGIC, AIR_MAGIC, WATER_MAGIC, EARTH_MAGIC,
		SCHOLAR, TACTICS, ARTILLERY, LEARNING, OFFENCE, ARMORER, INTELLIGENCE, SORCERY, RESISTANCE,
		FIRST_AID, SKILL_SIZE
	};
	static_assert(GameConstants::SKILL_QUANTITY == SKILL_SIZE, "Incorrect number of skills");
};

class SecondarySkill : public IdentifierWithEnum<SecondarySkillBase>
{
public:
	using IdentifierWithEnum<SecondarySkillBase>::IdentifierWithEnum;
};

class FactionIDBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NONE = -2,
		DEFAULT = -1,
		RANDOM = -1,
		ANY = -1,
		CASTLE,
		RAMPART,
		TOWER,
		INFERNO,
		NECROPOLIS,
		DUNGEON,
		STRONGHOLD,
		FORTRESS,
		CONFLUX,
		NEUTRAL
	};

	static si32 decode(const std::string& identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class FactionID : public IdentifierWithEnum<FactionIDBase>
{
public:
	using IdentifierWithEnum<FactionIDBase>::IdentifierWithEnum;
};

using ETownType = FactionID;

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

		//Special buildings for towns.
		LIGHTHOUSE  = SPECIAL_1,
		STABLES     = SPECIAL_2, //Castle
		BROTHERHOOD = SPECIAL_3,

		MYSTIC_POND         = SPECIAL_1,
		FOUNTAIN_OF_FORTUNE = SPECIAL_2, //Rampart
		TREASURY            = SPECIAL_3,

		ARTIFACT_MERCHANT = SPECIAL_1,
		LOOKOUT_TOWER     = SPECIAL_2, //Tower
		LIBRARY           = SPECIAL_3,
		WALL_OF_KNOWLEDGE = SPECIAL_4,

		STORMCLOUDS   = SPECIAL_2,
		CASTLE_GATE   = SPECIAL_3, //Inferno
		ORDER_OF_FIRE = SPECIAL_4,

		COVER_OF_DARKNESS    = SPECIAL_1,
		NECROMANCY_AMPLIFIER = SPECIAL_2, //Necropolis
		SKELETON_TRANSFORMER = SPECIAL_3,

		//ARTIFACT_MERCHANT - same ID as in tower
		MANA_VORTEX      = SPECIAL_2,
		PORTAL_OF_SUMMON = SPECIAL_3, //Dungeon
		BATTLE_ACADEMY   = SPECIAL_4,

		ESCAPE_TUNNEL     = SPECIAL_1,
		FREELANCERS_GUILD = SPECIAL_2, //Stronghold
		BALLISTA_YARD     = SPECIAL_3,
		HALL_OF_VALHALLA  = SPECIAL_4,

		CAGE_OF_WARLORDS = SPECIAL_1,
		GLYPHS_OF_FEAR   = SPECIAL_2, // Fortress
		BLOOD_OBELISK    = SPECIAL_3,

		//ARTIFACT_MERCHANT - same ID as in tower
		MAGIC_UNIVERSITY = SPECIAL_2, // Conflux
	};

	bool IsSpecialOrGrail() const
	{
		return num == SPECIAL_1 || num == SPECIAL_2 || num == SPECIAL_3 || num == SPECIAL_4 || num == GRAIL;
	}
};

class BuildingID : public IdentifierWithEnum<BuildingIDBase>
{
public:
	using IdentifierWithEnum<BuildingIDBase>::IdentifierWithEnum;
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

class Obj : public IdentifierWithEnum<ObjBase>
{
public:
	using IdentifierWithEnum<ObjBase>::IdentifierWithEnum;
};

class RoadIdBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NO_ROAD = 0,
		FIRST_REGULAR_ROAD = 1,
		DIRT_ROAD = 1,
		GRAVEL_ROAD = 2,
		COBBLESTONE_ROAD = 3,
		ORIGINAL_ROAD_COUNT //+1
	};
};

class RoadId : public IdentifierWithEnum<RoadIdBase>
{
public:
	using IdentifierWithEnum<RoadIdBase>::IdentifierWithEnum;
};

class RiverIdBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NO_RIVER = 0,
		FIRST_REGULAR_RIVER = 1,
		WATER_RIVER = 1,
		ICY_RIVER = 2,
		MUD_RIVER = 3,
		LAVA_RIVER = 4,
		ORIGINAL_RIVER_COUNT //+1
	};
};

class RiverId : public IdentifierWithEnum<RiverIdBase>
{
public:
	using IdentifierWithEnum<RiverIdBase>::IdentifierWithEnum;
};

using River = RiverId;
using Road = RoadId;

class DLL_LINKAGE EPathfindingLayerBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		LAND = 0, SAIL = 1, WATER, AIR, NUM_LAYERS, WRONG, AUTO
	};
};

class EPathfindingLayer : public IdentifierWithEnum<EPathfindingLayerBase>
{
public:
	using IdentifierWithEnum<EPathfindingLayerBase>::IdentifierWithEnum;
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

class ArtifactPosition : public IdentifierWithEnum<ArtifactPositionBase>
{
public:
	using IdentifierWithEnum<ArtifactPositionBase>::IdentifierWithEnum;
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

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class ArtifactID : public IdentifierWithEnum<ArtifactIDBase>
{
public:
	using IdentifierWithEnum<ArtifactIDBase>::IdentifierWithEnum;
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

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class CreatureID : public IdentifierWithEnum<CreatureIDBase>
{
public:
	using IdentifierWithEnum<CreatureIDBase>::IdentifierWithEnum;
};

class SpellIDBase : public IdentifierBase
{
public:
	enum Type
	{
		SPELLBOOK_PRESET = -3,
		PRESET = -2,
		NONE = -1,
		SUMMON_BOAT=0, SCUTTLE_BOAT=1, VISIONS=2, VIEW_EARTH=3, DISGUISE=4, VIEW_AIR=5,
		FLY=6, WATER_WALK=7, DIMENSION_DOOR=8, TOWN_PORTAL=9,

		QUICKSAND=10, LAND_MINE=11, FORCE_FIELD=12, FIRE_WALL=13, EARTHQUAKE=14,
		MAGIC_ARROW=15, ICE_BOLT=16, LIGHTNING_BOLT=17, IMPLOSION=18,
		CHAIN_LIGHTNING=19, FROST_RING=20, FIREBALL=21, INFERNO=22,
		METEOR_SHOWER=23, DEATH_RIPPLE=24, DESTROY_UNDEAD=25, ARMAGEDDON=26,
		SHIELD=27, AIR_SHIELD=28, FIRE_SHIELD=29, PROTECTION_FROM_AIR=30,
		PROTECTION_FROM_FIRE=31, PROTECTION_FROM_WATER=32,
		PROTECTION_FROM_EARTH=33, ANTI_MAGIC=34, DISPEL=35, MAGIC_MIRROR=36,
		CURE=37, RESURRECTION=38, ANIMATE_DEAD=39, SACRIFICE=40, BLESS=41,
		CURSE=42, BLOODLUST=43, PRECISION=44, WEAKNESS=45, STONE_SKIN=46,
		DISRUPTING_RAY=47, PRAYER=48, MIRTH=49, SORROW=50, FORTUNE=51,
		MISFORTUNE=52, HASTE=53, SLOW=54, SLAYER=55, FRENZY=56,
		TITANS_LIGHTNING_BOLT=57, COUNTERSTRIKE=58, BERSERK=59, HYPNOTIZE=60,
		FORGETFULNESS=61, BLIND=62, TELEPORT=63, REMOVE_OBSTACLE=64, CLONE=65,
		SUMMON_FIRE_ELEMENTAL=66, SUMMON_EARTH_ELEMENTAL=67, SUMMON_WATER_ELEMENTAL=68, SUMMON_AIR_ELEMENTAL=69,

		STONE_GAZE=70, POISON=71, BIND=72, DISEASE=73, PARALYZE=74, AGE=75, DEATH_CLOUD=76, THUNDERBOLT=77,
		DISPEL_HELPFUL_SPELLS=78, DEATH_STARE=79, ACID_BREATH_DEFENSE=80, ACID_BREATH_DAMAGE=81,

		FIRST_NON_SPELL = 70, AFTER_LAST = 82
	};

	DLL_LINKAGE const CSpell * toSpell() const; //deprecated
	DLL_LINKAGE const spells::Spell * toSpell(const spells::Service * service) const;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class SpellID : public IdentifierWithEnum<SpellIDBase>
{
public:
	using IdentifierWithEnum<SpellIDBase>::IdentifierWithEnum;
};

class BattleFieldInfo;
class BattleField : public Identifier<BattleField>
{
public:
	using Identifier<BattleField>::Identifier;

	DLL_LINKAGE static const BattleField NONE;
	DLL_LINKAGE const BattleFieldInfo * getInfo() const;
};

class BoatIdBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		NONE = -1,
		NECROPOLIS = 0,
		CASTLE,
		FORTRESS
	};
};

class BoatId : public IdentifierWithEnum<BoatIdBase>
{
public:
	using IdentifierWithEnum<BoatIdBase>::IdentifierWithEnum;
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

	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
	static std::string entityType();
};

class TerrainId : public IdentifierWithEnum<TerrainIdBase>
{
public:
	using IdentifierWithEnum<TerrainIdBase>::IdentifierWithEnum;
};

using ETerrainId = TerrainId;

class ObstacleInfo;
class Obstacle : public Identifier<Obstacle>
{
public:
	using Identifier<Obstacle>::Identifier;
	DLL_LINKAGE const ObstacleInfo * getInfo() const;
};

class SpellSchoolBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		ANY 	= -1,
		AIR 	= 0,
		FIRE 	= 1,
		WATER 	= 2,
		EARTH 	= 3,
	};
};

class SpellSchool : public IdentifierWithEnum<SpellSchoolBase>
{
public:
	using IdentifierWithEnum<SpellSchoolBase>::IdentifierWithEnum;
};

using ESpellSchool = SpellSchool;

class GameResIDBase : public IdentifierBase
{
public:
	enum Type : int32_t
	{
		WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL,
		COUNT,

		WOOD_AND_ORE = 127,  // special case for town bonus resource
		INVALID = -1
	};
};

class GameResID : public IdentifierWithEnum<GameResIDBase>
{
public:
	using IdentifierWithEnum<GameResIDBase>::IdentifierWithEnum;
};

using EGameResID = GameResID;

VCMI_LIB_NAMESPACE_END
