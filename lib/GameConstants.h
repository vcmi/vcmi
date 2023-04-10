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

#include "ConstTransitivePtr.h"

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

struct IdTag
{};

namespace GameConstants
{
	DLL_LINKAGE extern const std::string VCMI_VERSION;

	constexpr int PUZZLE_MAP_PIECES = 48;

	constexpr int MAX_HEROES_PER_PLAYER = 8;
	constexpr int AVAILABLE_HEROES_PER_PLAYER = 2;

	constexpr int ALL_PLAYERS = 255; //bitfield

	constexpr int CREATURES_PER_TOWN = 7; //without upgrades
	constexpr int SPELL_LEVELS = 5;
	constexpr int SPELL_SCHOOL_LEVELS = 4;
	constexpr int CRE_LEVELS = 10; // number of creature experience levels

	constexpr int HERO_GOLD_COST = 2500;
	constexpr int SPELLBOOK_GOLD_COST = 500;
	constexpr int SKILL_GOLD_COST = 2000;
	constexpr int BATTLE_PENALTY_DISTANCE = 10; //if the distance is > than this, then shooting stack has distance penalty
	constexpr int ARMY_SIZE = 7;
	constexpr int SKILL_PER_HERO = 8;
	constexpr ui32 HERO_HIGH_LEVEL = 10; // affects primary skill upgrade order

	constexpr int SKILL_QUANTITY=28;
	constexpr int PRIMARY_SKILLS=4;
	constexpr int RESOURCE_QUANTITY=8;
	constexpr int HEROES_PER_TYPE=8; //amount of heroes of each type

	// amounts of OH3 objects. Can be changed by mods, should be used only during H3 loading phase
	constexpr int F_NUMBER = 9;
	constexpr int ARTIFACTS_QUANTITY=171;
	constexpr int HEROES_QUANTITY=156;
	constexpr int SPELLS_QUANTITY=70;
	constexpr int CREATURES_COUNT = 197;

	constexpr ui32 BASE_MOVEMENT_COST = 100; //default cost for non-diagonal movement

	constexpr int HERO_PORTRAIT_SHIFT = 30;// 2 special frames + some extra portraits

	constexpr std::array<int, 11> POSSIBLE_TURNTIME = {1, 2, 4, 6, 8, 10, 15, 20, 25, 30, 0};
}

#define ID_LIKE_CLASS_COMMON(CLASS_NAME, ENUM_NAME)	\
constexpr CLASS_NAME(const CLASS_NAME & other) = default;	\
constexpr CLASS_NAME & operator=(const CLASS_NAME & other) = default;	\
explicit constexpr CLASS_NAME(si32 id)				\
	: num(static_cast<ENUM_NAME>(id))				\
{}													\
constexpr operator ENUM_NAME() const				\
{													\
	return num;										\
}													\
constexpr si32 getNum() const						\
{													\
	return static_cast<si32>(num);					\
}													\
constexpr ENUM_NAME toEnum() const					\
{													\
	return num;										\
}													\
template <typename Handler> void serialize(Handler &h, const int version)	\
{													\
	h & num;										\
}													\
constexpr CLASS_NAME & advance(int i)				\
{													\
	num = static_cast<ENUM_NAME>(static_cast<int>(num) + i);		\
	return *this;									\
}


// Operators are performance-critical and to be inlined they must be in header
#define ID_LIKE_OPERATORS_INTERNAL(A, B, AN, BN)	\
STRONG_INLINE constexpr bool operator==(const A & a, const B & b)	\
{													\
	return AN == BN ;								\
}													\
STRONG_INLINE constexpr bool operator!=(const A & a, const B & b)	\
{													\
	return AN != BN ;								\
}													\
STRONG_INLINE constexpr bool operator<(const A & a, const B & b)	\
{													\
	return AN < BN ;								\
}													\
STRONG_INLINE constexpr bool operator<=(const A & a, const B & b)	\
{													\
	return AN <= BN ;								\
}													\
STRONG_INLINE constexpr bool operator>(const A & a, const B & b)	\
{													\
	return AN > BN ;								\
}													\
STRONG_INLINE constexpr bool operator>=(const A & a, const B & b)	\
{													\
	return AN >= BN ;								\
}

#define ID_LIKE_OPERATORS(CLASS_NAME, ENUM_NAME)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, CLASS_NAME, a.num, b.num)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, ENUM_NAME, a.num, b)	\
	ID_LIKE_OPERATORS_INTERNAL(ENUM_NAME, CLASS_NAME, a, b.num)


#define INSTID_LIKE_CLASS_COMMON(CLASS_NAME, NUMERIC_NAME)	\
public:														\
constexpr CLASS_NAME(const CLASS_NAME & other):						\
	BaseForID<CLASS_NAME, NUMERIC_NAME>(other)				\
{															\
}															\
constexpr CLASS_NAME & operator=(const CLASS_NAME & other) = default;	\
constexpr CLASS_NAME & operator=(NUMERIC_NAME other) { num = other; return *this; };	\
explicit constexpr CLASS_NAME(si32 id = -1)								\
	: BaseForID<CLASS_NAME, NUMERIC_NAME>(id)				\
{}

template < typename Derived, typename NumericType>
class BaseForID : public IdTag
{
protected:
	NumericType num;

public:
	constexpr NumericType getNum() const
	{
		return num;
	}

	//to make it more similar to IDLIKE
	constexpr NumericType toEnum() const
	{
		return num;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & num;
	}

	constexpr explicit BaseForID(NumericType _num = -1) :
		num(_num)
	{
	}

	constexpr void advance(int change)
	{
		num += change;
	}

	constexpr bool operator == (const BaseForID & b) const { return num == b.num; }
	constexpr bool operator <= (const BaseForID & b) const { return num <= b.num; }
	constexpr bool operator >= (const BaseForID & b) const { return num >= b.num; }
	constexpr bool operator != (const BaseForID & b) const { return num != b.num; }
	constexpr bool operator <  (const BaseForID & b) const { return num <  b.num; }
	constexpr bool operator >  (const BaseForID & b) const { return num >  b.num; }

	constexpr BaseForID & operator++() { ++num; return *this; }

	constexpr operator NumericType() const
	{
		return num;
	}
};

template < typename T>
class Identifier : public IdTag
{
public:
	using EnumType    = T;
	using NumericType = typename std::underlying_type<EnumType>::type;

private:
	NumericType num;

public:
	constexpr NumericType getNum() const
	{
		return num;
	}

	constexpr EnumType toEnum() const
	{
		return static_cast<EnumType>(num);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & num;
	}

	constexpr explicit Identifier(NumericType _num = -1)
	{
		num = _num;
	}

	/* implicit */constexpr Identifier(EnumType _num):
		num(static_cast<NumericType>(_num))
	{
	}

	constexpr void advance(int change)
	{
		num += change;
	}

	constexpr bool operator == (const Identifier & b) const { return num == b.num; }
	constexpr bool operator <= (const Identifier & b) const { return num <= b.num; }
	constexpr bool operator >= (const Identifier & b) const { return num >= b.num; }
	constexpr bool operator != (const Identifier & b) const { return num != b.num; }
	constexpr bool operator <  (const Identifier & b) const { return num <  b.num; }
	constexpr bool operator >  (const Identifier & b) const { return num > b.num; }

	constexpr Identifier & operator++()
	{
		++num;
		return *this;
	}

	constexpr Identifier operator++(int)
	{
		Identifier ret(*this);
		++num;
		return ret;
	}

	constexpr operator NumericType() const
	{
		return num;
	}
};


template<typename Der, typename Num>
std::ostream & operator << (std::ostream & os, BaseForID<Der, Num> id);

template<typename Der, typename Num>
std::ostream & operator << (std::ostream & os, BaseForID<Der, Num> id)
{
	//We use common type with short to force char and unsigned char to be promoted and formatted as numbers.
	typedef typename std::common_type<short, Num>::type Number;
	return os << static_cast<Number>(id.getNum());
}

template<typename EnumType>
std::ostream & operator << (std::ostream & os, Identifier<EnumType> id)
{
	//We use common type with short to force char and unsigned char to be promoted and formatted as numbers.
	typedef typename std::common_type<short, typename Identifier<EnumType>::NumericType>::type Number;
	return os << static_cast<Number>(id.getNum());
}

class ArtifactInstanceID : public BaseForID<ArtifactInstanceID, si32>
{
	INSTID_LIKE_CLASS_COMMON(ArtifactInstanceID, si32)

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;
};

class QueryID : public BaseForID<QueryID, si32>
{
	INSTID_LIKE_CLASS_COMMON(QueryID, si32)

	QueryID & operator++()
	{
		++num;
		return *this;
	}
};

class ObjectInstanceID : public BaseForID<ObjectInstanceID, si32>
{
	INSTID_LIKE_CLASS_COMMON(ObjectInstanceID, si32)

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;
};

class HeroClassID : public BaseForID<HeroClassID, si32>
{
	INSTID_LIKE_CLASS_COMMON(HeroClassID, si32)
};

class HeroTypeID : public BaseForID<HeroTypeID, si32>
{
	INSTID_LIKE_CLASS_COMMON(HeroTypeID, si32)

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

class SlotID : public BaseForID<SlotID, si32>
{
	INSTID_LIKE_CLASS_COMMON(SlotID, si32)

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;

	DLL_LINKAGE static const SlotID COMMANDER_SLOT_PLACEHOLDER;
	DLL_LINKAGE static const SlotID SUMMONED_SLOT_PLACEHOLDER; ///<for all summoned creatures, only during battle
	DLL_LINKAGE static const SlotID WAR_MACHINES_SLOT; ///<for all war machines during battle
	DLL_LINKAGE static const SlotID ARROW_TOWERS_SLOT; ///<for all arrow towers during battle

	bool validSlot() const
	{
		return getNum() >= 0  &&  getNum() < GameConstants::ARMY_SIZE;
	}
};

class PlayerColor : public BaseForID<PlayerColor, ui8>
{
	INSTID_LIKE_CLASS_COMMON(PlayerColor, ui8)

	enum EPlayerColor
	{
		PLAYER_LIMIT_I = 8
	};

	DLL_LINKAGE static const PlayerColor SPECTATOR; //252
	DLL_LINKAGE static const PlayerColor CANNOT_DETERMINE; //253
	DLL_LINKAGE static const PlayerColor UNFLAGGABLE; //254 - neutral objects (pandora, banks)
	DLL_LINKAGE static const PlayerColor NEUTRAL; //255
	DLL_LINKAGE static const PlayerColor PLAYER_LIMIT; //player limit per map

	DLL_LINKAGE bool isValidPlayer() const; //valid means < PLAYER_LIMIT (especially non-neutral)
	DLL_LINKAGE bool isSpectator() const;

	DLL_LINKAGE std::string getStr(bool L10n = false) const;
	DLL_LINKAGE std::string getStrCap(bool L10n = false) const;

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;
};

class TeamID : public BaseForID<TeamID, ui8>
{
	INSTID_LIKE_CLASS_COMMON(TeamID, ui8)

	DLL_LINKAGE static const TeamID NO_TEAM;

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;
};

class TeleportChannelID : public BaseForID<TeleportChannelID, si32>
{
	INSTID_LIKE_CLASS_COMMON(TeleportChannelID, si32)

	friend class CGameInfoCallback;
	friend class CNonConstInfoCallback;
};

// #ifndef INSTANTIATE_BASE_FOR_ID_HERE
// extern template std::ostream & operator << <ArtifactInstanceID>(std::ostream & os, BaseForID<ArtifactInstanceID> id);
// extern template std::ostream & operator << <ObjectInstanceID>(std::ostream & os, BaseForID<ObjectInstanceID> id);
// #endif

// Enum declarations
namespace PrimarySkill
{
	enum PrimarySkill { NONE = -1, ATTACK, DEFENSE, SPELL_POWER, KNOWLEDGE,
				EXPERIENCE = 4}; //for some reason changePrimSkill uses it
}

class SecondarySkill
{
public:
	enum ESecondarySkill
	{
		WRONG = -2,
		DEFAULT = -1,
		PATHFINDING = 0, ARCHERY, LOGISTICS, SCOUTING, DIPLOMACY, NAVIGATION, LEADERSHIP, WISDOM, MYSTICISM,
		LUCK, BALLISTICS, EAGLE_EYE, NECROMANCY, ESTATES, FIRE_MAGIC, AIR_MAGIC, WATER_MAGIC, EARTH_MAGIC,
		SCHOLAR, TACTICS, ARTILLERY, LEARNING, OFFENCE, ARMORER, INTELLIGENCE, SORCERY, RESISTANCE,
		FIRST_AID, SKILL_SIZE
	};

	static_assert(GameConstants::SKILL_QUANTITY == SKILL_SIZE, "Incorrect number of skills");

	SecondarySkill(ESecondarySkill _num = WRONG) : num(_num)
	{}

	ID_LIKE_CLASS_COMMON(SecondarySkill, ESecondarySkill)

	ESecondarySkill num;
};

ID_LIKE_OPERATORS(SecondarySkill, SecondarySkill::ESecondarySkill)

enum class EAlignment : uint8_t { GOOD, EVIL, NEUTRAL };

namespace ETownType//deprecated
{
	enum ETownType
	{
		ANY = -1,
		CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX, NEUTRAL
	};
}

class FactionID : public BaseForID<FactionID, int32_t>
{
	INSTID_LIKE_CLASS_COMMON(FactionID, si32)

	DLL_LINKAGE static const FactionID ANY;
	DLL_LINKAGE static const FactionID CASTLE;
	DLL_LINKAGE static const FactionID RAMPART;
	DLL_LINKAGE static const FactionID TOWER;
	DLL_LINKAGE static const FactionID INFERNO;
	DLL_LINKAGE static const FactionID NECROPOLIS;
	DLL_LINKAGE static const FactionID DUNGEON;
	DLL_LINKAGE static const FactionID STRONGHOLD;
	DLL_LINKAGE static const FactionID FORTRESS;
	DLL_LINKAGE static const FactionID CONFLUX;
	DLL_LINKAGE static const FactionID NEUTRAL;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};


class BuildingID
{
public:
	//Quite useful as long as most of building mechanics hardcoded
	// NOTE: all building with completely configurable mechanics will be removed from list
	enum EBuildingID
	{
		DEFAULT = -50,
		HORDE_PLACEHOLDER7 = -36,
		HORDE_PLACEHOLDER6 = -35,
		HORDE_PLACEHOLDER5 = -34,
		HORDE_PLACEHOLDER4 = -33,
		HORDE_PLACEHOLDER3 = -32,
		HORDE_PLACEHOLDER2 = -31,
		HORDE_PLACEHOLDER1 = -30,
		HORDE_BUILDING_CONVERTER = -29, //-1 => -30
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

	BuildingID(EBuildingID _num = NONE) : num(_num)
	{}

	STRONG_INLINE
	bool IsSpecialOrGrail() const
	{
		return num == SPECIAL_1 || num == SPECIAL_2 || num == SPECIAL_3 || num == SPECIAL_4 || num == GRAIL;
	}

	ID_LIKE_CLASS_COMMON(BuildingID, EBuildingID)

	EBuildingID num;
};

ID_LIKE_OPERATORS(BuildingID, BuildingID::EBuildingID)

namespace BuildingSubID
{
	enum EBuildingSubID
	{
		DEFAULT = -50,
		NONE = -1,
		STABLES,
		BROTHERHOOD_OF_SWORD,
		CASTLE_GATE,
		CREATURE_TRANSFORMER,
		MYSTIC_POND,
		FOUNTAIN_OF_FORTUNE,
		ARTIFACT_MERCHANT,
		LOOKOUT_TOWER,
		LIBRARY,
		MANA_VORTEX,
		PORTAL_OF_SUMMONING,
		ESCAPE_TUNNEL,
		FREELANCERS_GUILD,
		BALLISTA_YARD,
		ATTACK_VISITING_BONUS,
		MAGIC_UNIVERSITY,
		SPELL_POWER_GARRISON_BONUS,
		ATTACK_GARRISON_BONUS,
		DEFENSE_GARRISON_BONUS,
		DEFENSE_VISITING_BONUS,
		SPELL_POWER_VISITING_BONUS,
		KNOWLEDGE_VISITING_BONUS,
		EXPERIENCE_VISITING_BONUS,
		LIGHTHOUSE,
		TREASURY,
		CUSTOM_VISITING_BONUS
	};
}

namespace MappedKeys
{

	static const std::map<std::string, BuildingID> BUILDING_NAMES_TO_TYPES =
	{
		{ "special1", BuildingID::SPECIAL_1 },
		{ "special2", BuildingID::SPECIAL_2 },
		{ "special3", BuildingID::SPECIAL_3 },
		{ "special4", BuildingID::SPECIAL_4 },
		{ "grail", BuildingID::GRAIL }
	};

	static const std::map<BuildingID, std::string> BUILDING_TYPES_TO_NAMES =
	{
		{ BuildingID::SPECIAL_1, "special1", },
		{ BuildingID::SPECIAL_2, "special2" },
		{ BuildingID::SPECIAL_3, "special3" },
		{ BuildingID::SPECIAL_4, "special4" },
		{ BuildingID::GRAIL, "grail"}
	};

	static const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS =
	{
		{ "mysticPond", BuildingSubID::MYSTIC_POND },
		{ "artifactMerchant", BuildingSubID::ARTIFACT_MERCHANT },
		{ "freelancersGuild", BuildingSubID::FREELANCERS_GUILD },
		{ "magicUniversity", BuildingSubID::MAGIC_UNIVERSITY },
		{ "castleGate", BuildingSubID::CASTLE_GATE },
		{ "creatureTransformer", BuildingSubID::CREATURE_TRANSFORMER },//only skeleton transformer yet
		{ "portalOfSummoning", BuildingSubID::PORTAL_OF_SUMMONING },
		{ "ballistaYard", BuildingSubID::BALLISTA_YARD },
		{ "stables", BuildingSubID::STABLES },
		{ "manaVortex", BuildingSubID::MANA_VORTEX },
		{ "lookoutTower", BuildingSubID::LOOKOUT_TOWER },
		{ "library", BuildingSubID::LIBRARY },
		{ "brotherhoodOfSword", BuildingSubID::BROTHERHOOD_OF_SWORD },//morale garrison bonus
		{ "fountainOfFortune", BuildingSubID::FOUNTAIN_OF_FORTUNE },//luck garrison bonus
		{ "spellPowerGarrisonBonus", BuildingSubID::SPELL_POWER_GARRISON_BONUS },//such as 'stormclouds', but this name is not ok for good towns
		{ "attackGarrisonBonus", BuildingSubID::ATTACK_GARRISON_BONUS },
		{ "defenseGarrisonBonus", BuildingSubID::DEFENSE_GARRISON_BONUS },
		{ "escapeTunnel", BuildingSubID::ESCAPE_TUNNEL },
		{ "attackVisitingBonus", BuildingSubID::ATTACK_VISITING_BONUS },
		{ "defenceVisitingBonus", BuildingSubID::DEFENSE_VISITING_BONUS },
		{ "spellPowerVisitingBonus", BuildingSubID::SPELL_POWER_VISITING_BONUS },
		{ "knowledgeVisitingBonus", BuildingSubID::KNOWLEDGE_VISITING_BONUS },
		{ "experienceVisitingBonus", BuildingSubID::EXPERIENCE_VISITING_BONUS },
		{ "lighthouse", BuildingSubID::LIGHTHOUSE },
		{ "treasury", BuildingSubID::TREASURY }
	};
}

namespace EAiTactic
{
enum EAiTactic
{
	NONE = -1,
	RANDOM,
	WARRIOR,
	BUILDER,
	EXPLORER
};
}

namespace EBuildingState
{
	enum EBuildingState
	{
		HAVE_CAPITAL, NO_WATER, FORBIDDEN, ADD_MAGES_GUILD, ALREADY_PRESENT, CANT_BUILD_TODAY,
		NO_RESOURCES, ALLOWED, PREREQUIRES, MISSING_BASE, BUILDING_ERROR, TOWN_NOT_OWNED
	};
}

namespace ESpellCastProblem
{
	enum ESpellCastProblem
	{
		OK, NO_HERO_TO_CAST_SPELL, CASTS_PER_TURN_LIMIT, NO_SPELLBOOK,
		HERO_DOESNT_KNOW_SPELL, NOT_ENOUGH_MANA, ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL,
		SPELL_LEVEL_LIMIT_EXCEEDED, NO_SPELLS_TO_DISPEL,
		NO_APPROPRIATE_TARGET, STACK_IMMUNE_TO_SPELL, WRONG_SPELL_TARGET, ONGOING_TACTIC_PHASE,
		MAGIC_IS_BLOCKED, //For Orb of Inhibition and similar - no casting at all
		INVALID
	};
}

namespace EMarketMode
{
	enum EMarketMode
	{
		RESOURCE_RESOURCE, RESOURCE_PLAYER, CREATURE_RESOURCE, RESOURCE_ARTIFACT,
		ARTIFACT_RESOURCE, ARTIFACT_EXP, CREATURE_EXP, CREATURE_UNDEAD, RESOURCE_SKILL,
		MARTKET_AFTER_LAST_PLACEHOLDER
	};
}

namespace ECommander
{
	enum SecondarySkills {ATTACK, DEFENSE, HEALTH, DAMAGE, SPEED, SPELL_POWER, CASTS, RESISTANCE};
	const int MAX_SKILL_LEVEL = 5;
}

enum class EWallPart : int8_t
{
	INDESTRUCTIBLE_PART_OF_GATE = -3, INDESTRUCTIBLE_PART = -2, INVALID = -1,
	KEEP = 0, BOTTOM_TOWER, BOTTOM_WALL, BELOW_GATE, OVER_GATE, UPPER_WALL, UPPER_TOWER, GATE,
	PARTS_COUNT /* This constant SHOULD always stay as the last item in the enum. */
};

enum class EWallState : int8_t
{
	NONE = -1, //no wall
	DESTROYED,
	DAMAGED,
	INTACT,
	REINFORCED, // walls in towns with castle
};

enum class EGateState : uint8_t
{
	NONE,
	CLOSED,
	BLOCKED, // gate is blocked in closed state, e.g. by creature
	OPENED,
	DESTROYED
};

namespace ESiegeHex
{
	enum ESiegeHex : si16
	{
		DESTRUCTIBLE_WALL_1 = 29,
		DESTRUCTIBLE_WALL_2 = 78,
		DESTRUCTIBLE_WALL_3 = 130,
		DESTRUCTIBLE_WALL_4 = 182,
		GATE_BRIDGE = 94,
		GATE_OUTER = 95,
		GATE_INNER = 96
	};
}

namespace ETileType
{
	enum ETileType
	{
		FREE,
		POSSIBLE,
		BLOCKED,
		USED
	};
}

enum class ETeleportChannelType
{
	IMPASSABLE,
	BIDIRECTIONAL,
	UNIDIRECTIONAL,
	MIXED
};

class Obj
{
public:
	enum EObj
	{
		NO_OBJ = -1,
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
		HOLE = 124,
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
	Obj(EObj _num = NO_OBJ) : num(_num)
	{}

	ID_LIKE_CLASS_COMMON(Obj, EObj)

	EObj num;
};

ID_LIKE_OPERATORS(Obj, Obj::EObj)

enum class Road : int8_t
{
	NO_ROAD = 0,
	FIRST_REGULAR_ROAD = 1,
	DIRT_ROAD = 1,
	GRAVEL_ROAD = 2,
	COBBLESTONE_ROAD = 3,
	ORIGINAL_ROAD_COUNT //+1
};

enum class River : int8_t
{
	NO_RIVER = 0,
	FIRST_REGULAR_RIVER = 1,
	WATER_RIVER = 1,
	ICY_RIVER = 2,
	MUD_RIVER = 3,
	LAVA_RIVER = 4,
	ORIGINAL_RIVER_COUNT //+1
};

namespace SecSkillLevel
{
	enum SecSkillLevel
	{
		NONE,
		BASIC,
		ADVANCED,
		EXPERT,
		LEVELS_SIZE
	};
}

namespace Date
{
	enum EDateType
	{
		DAY = 0,
		DAY_OF_WEEK = 1,
		WEEK = 2,
		MONTH = 3,
		DAY_OF_MONTH
	};
}

enum class EActionType : int32_t
{
	CANCEL = -3,
	END_TACTIC_PHASE = -2,
	INVALID = -1,
	NO_ACTION = 0,
	HERO_SPELL,
	WALK,
	DEFEND,
	RETREAT,
	SURRENDER,
	WALK_AND_ATTACK,
	SHOOT,
	WAIT,
	CATAPULT,
	MONSTER_SPELL,
	BAD_MORALE,
	STACK_HEAL,
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EActionType actionType);

class DLL_LINKAGE EDiggingStatus
{
public:
	enum EEDiggingStatus
	{
		UNKNOWN = -1,
		CAN_DIG = 0,
		LACK_OF_MOVEMENT,
		WRONG_TERRAIN,
		TILE_OCCUPIED,
		BACKPACK_IS_FULL
	};

	EDiggingStatus(EEDiggingStatus _num = UNKNOWN) : num(_num)
	{}

	ID_LIKE_CLASS_COMMON(EDiggingStatus, EEDiggingStatus)

	EEDiggingStatus num;
};

ID_LIKE_OPERATORS(EDiggingStatus, EDiggingStatus::EEDiggingStatus)

class DLL_LINKAGE EPathfindingLayer
{
public:
	enum EEPathfindingLayer : ui8
	{
		LAND = 0, SAIL = 1, WATER, AIR, NUM_LAYERS, WRONG, AUTO
	};

	EPathfindingLayer(EEPathfindingLayer _num = WRONG) : num(_num)
	{}

	ID_LIKE_CLASS_COMMON(EPathfindingLayer, EEPathfindingLayer)

	EEPathfindingLayer num;
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EPathfindingLayer & pathfindingLayer);

ID_LIKE_OPERATORS(EPathfindingLayer, EPathfindingLayer::EEPathfindingLayer)

namespace EPlayerStatus
{
	enum EStatus {WRONG = -1, INGAME, LOSER, WINNER};
}

namespace PlayerRelations
{
	enum PlayerRelations {ENEMIES, ALLIES, SAME_PLAYER};
}

class ArtifactPosition
{
public:
	enum EArtifactPosition
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
		COMMANDER1 = 0, COMMANDER2, COMMANDER3, COMMANDER4, COMMANDER5, COMMANDER6, COMMANDER_AFTER_LAST
	};

	static_assert (AFTER_LAST == 19, "incorrect number of artifact slots");

	ArtifactPosition(EArtifactPosition _num = PRE_FIRST) : num(_num)
	{}

	ArtifactPosition(std::string slotName);

	ID_LIKE_CLASS_COMMON(ArtifactPosition, EArtifactPosition)

	EArtifactPosition num;

        STRONG_INLINE EArtifactPosition operator+(const int arg)
	{
		return EArtifactPosition(static_cast<int>(num) + arg);
	}
	STRONG_INLINE EArtifactPosition operator+(const EArtifactPosition & arg)
	{
		return EArtifactPosition(static_cast<int>(num) + static_cast<int>(arg));
	}
};

ID_LIKE_OPERATORS(ArtifactPosition, ArtifactPosition::EArtifactPosition)

namespace GameConstants
{
	const auto BACKPACK_START = ArtifactPosition::AFTER_LAST;
}

class ArtifactID
{
public:
	enum EArtifactID
	{
		NONE = -1,
		SPELLBOOK = 0,
		SPELL_SCROLL = 1,
		GRAIL = 2,
		CATAPULT = 3,
		BALLISTA = 4,
		AMMO_CART = 5,
		FIRST_AID_TENT = 6,
		//CENTAUR_AXE = 7,
		//BLACKSHARD_OF_THE_DEAD_KNIGHT = 8,
		ARMAGEDDONS_BLADE = 128,
		TITANS_THUNDER = 135,
		//CORNUCOPIA = 140,
		//FIXME: the following is only true if WoG is enabled. Otherwise other mod artifacts will take these slots.
		ART_SELECTION = 144,
		ART_LOCK = 145, // FIXME: We must get rid of this one since it's conflict with artifact from mods. See issue 2455
		AXE_OF_SMASHING = 146,
		MITHRIL_MAIL = 147,
		SWORD_OF_SHARPNESS = 148,
		HELM_OF_IMMORTALITY = 149,
		PENDANT_OF_SORCERY = 150,
		BOOTS_OF_HASTE = 151,
		BOW_OF_SEEKING = 152,
		DRAGON_EYE_RING = 153
		//HARDENED_SHIELD = 154,
		//SLAVAS_RING_OF_POWER = 155
	};

	ArtifactID(EArtifactID _num = NONE) : num(_num)
	{}

	DLL_LINKAGE const CArtifact * toArtifact() const;
	DLL_LINKAGE const Artifact * toArtifact(const ArtifactService * service) const;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);

	ID_LIKE_CLASS_COMMON(ArtifactID, EArtifactID)

	EArtifactID num;

	struct hash
	{
		size_t operator()(const ArtifactID & aid) const
		{
			return std::hash<int>()(aid.num);
		}
	};
};

ID_LIKE_OPERATORS(ArtifactID, ArtifactID::EArtifactID)

class CreatureID
{
public:
	enum ECreatureID
	{
		NONE = -1,
		ARCHER = 2,
		CAVALIER = 10,
		CHAMPION = 11,
		STONE_GOLEM = 32,
		IRON_GOLEM = 33,
		IMP = 42,
		SKELETON = 56,
		WALKING_DEAD = 58,
		WIGHTS = 60,
		LICHES = 64,
		BONE_DRAGON = 68,
		TROGLODYTES = 70,
		MEDUSA = 76,
		HYDRA = 110,
		CHAOS_HYDRA = 111,
		AIR_ELEMENTAL = 112,
		EARTH_ELEMENTAL = 113,
		FIRE_ELEMENTAL = 114,
		WATER_ELEMENTAL = 115,
		GOLD_GOLEM = 116,
		DIAMOND_GOLEM = 117,
		PSYCHIC_ELEMENTAL = 120,
		MAGIC_ELEMENTAL = 121,
		CATAPULT = 145,
		BALLISTA = 146,
		FIRST_AID_TENT = 147,
		AMMO_CART = 148,
		ARROW_TOWERS = 149
	};

	CreatureID(ECreatureID _num = NONE) : num(_num)
	{}

	DLL_LINKAGE const CCreature * toCreature() const;
	DLL_LINKAGE const Creature * toCreature(const CreatureService * creatures) const;

	ID_LIKE_CLASS_COMMON(CreatureID, ECreatureID)

	ECreatureID num;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

ID_LIKE_OPERATORS(CreatureID, CreatureID::ECreatureID)

class SpellID
{
public:
	enum ESpellID
	{
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

	SpellID(ESpellID _num = NONE) : num(_num)
	{}

	DLL_LINKAGE const CSpell * toSpell() const; //deprecated
	DLL_LINKAGE const spells::Spell * toSpell(const spells::Service * service) const;

	ID_LIKE_CLASS_COMMON(SpellID, ESpellID)

	ESpellID num;

	///json serialization helpers
	static si32 decode(const std::string & identifier);
	static std::string encode(const si32 index);
};

ID_LIKE_OPERATORS(SpellID, SpellID::ESpellID)

class BattleFieldInfo;
class BattleField : public BaseForID<BattleField, si32>
{
	INSTID_LIKE_CLASS_COMMON(BattleField, si32)

	DLL_LINKAGE static const BattleField NONE;

	DLL_LINKAGE friend bool operator==(const BattleField & l, const BattleField & r);
	DLL_LINKAGE friend bool operator!=(const BattleField & l, const BattleField & r);
	DLL_LINKAGE friend bool operator<(const BattleField & l, const BattleField & r);

	DLL_LINKAGE operator std::string() const;
	DLL_LINKAGE const BattleFieldInfo * getInfo() const;

	DLL_LINKAGE static BattleField fromString(const std::string & identifier);
};

enum class ETerrainId {
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

using TerrainId = Identifier<ETerrainId>;
using RoadId = Identifier<Road>;
using RiverId = Identifier<River>;

class ObstacleInfo;
class Obstacle : public BaseForID<Obstacle, si32>
{
	INSTID_LIKE_CLASS_COMMON(Obstacle, si32)

	DLL_LINKAGE const ObstacleInfo * getInfo() const;
	DLL_LINKAGE operator std::string() const;
	DLL_LINKAGE static Obstacle fromString(const std::string & identifier);
};

enum class ESpellSchool: ui8
{
	AIR 	= 0,
	FIRE 	= 1,
	WATER 	= 2,
	EARTH 	= 3
};

enum class EMetaclass: ui8
{
	INVALID = 0,
	ARTIFACT,
	CREATURE,
	FACTION,
	EXPERIENCE,
	HERO,
	HEROCLASS,
	LUCK,
	MANA,
	MORALE,
	MOVEMENT,
	OBJECT,
	PRIMARY_SKILL,
	SECONDARY_SKILL,
	SPELL,
	RESOURCE
};

enum class EHealLevel: ui8
{
	HEAL,
	RESURRECT,
	OVERHEAL
};

enum class EHealPower : ui8
{
	ONE_BATTLE,
	PERMANENT
};

// Typedef declarations
typedef ui8 TFaction;
typedef si64 TExpType;
typedef si32 TBonusSubtype;
typedef si32 TQuantity;

typedef int TRmgTemplateZoneId;

#undef ID_LIKE_CLASS_COMMON
#undef ID_LIKE_OPERATORS
#undef ID_LIKE_OPERATORS_INTERNAL
#undef INSTID_LIKE_CLASS_COMMON

VCMI_LIB_NAMESPACE_END
