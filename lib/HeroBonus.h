#pragma once
#include "../global.h"
#include <string>

struct DLL_EXPORT HeroBonus
{
	enum BonusType
	{
		//handled
		NONE, 
		MOVEMENT, //both water/land
		LAND_MOVEMENT, 
		SEA_MOVEMENT, 
		MORALE, 
		LUCK, 
		MORALE_AND_LUCK, 
		PRIMARY_SKILL, //uses subtype to pick skill
		SIGHT_RADIOUS, 
		MANA_REGENERATION, //points per turn apart from normal (1 + mysticism)
		//not handled yet:
		MAGIC_RESISTANCE, // %
		SECONDARY_SKILL_PREMY, //%
		SURRENDER_DISCOUNT, //%
		STACKS_SPEED,
		FLYING_MOVEMENT, SPELL_DURATION, AIR_SPELL_DMG_PREMY, EARTH_SPELL_DMG_PREMY, FIRE_SPELL_DMG_PREMY, 
		WATER_SPELL_DMG_PREMY, BLOCK_SPELLS_ABOVE_LEVEL, WATER_WALKING, NO_SHOTING_PENALTY, DISPEL_IMMUNITY, 
		NEGATE_ALL_NATURAL_IMMUNITIES, STACK_HEALTH, SPELL_IMMUNITY, BLOCK_MORALE, BLOCK_LUCK, FIRE_SPELLS,
		AIR_SPELLS, WATER_SPELLS, EARTH_SPELLS, 
		GENERATE_RESOURCE, //daily value, uses subtype (resource type)
		CREATURE_GROWTH, //for legion artifacts: value - week growth bonus, subtype - monster level
		WHIRLPOOL_PROTECTION, //hero won't lose army when teleporting through whirlpool
		SPELL, //hero knows spell, val - skill level (0 - 3), subtype - spell id
		SPELLS_OF_LEVEL, //hero knows all spells of given level, val - skill level; subtype - level
		ENEMY_CANT_ESCAPE //for shackles of war
	};
	enum BonusDuration{PERMANENT, ONE_BATTLE, ONE_DAY, ONE_WEEK};
	enum BonusSource{ARTIFACT, OBJECT};

	ui8 duration; //uses BonusDuration values
	ui8 type; //uses BonusType values - says to what is this bonus
	si32 subtype; //-1 if not applicable
	ui8 source;//uses BonusSource values - what gave that bonus
	si32 val;//for morale/luck [-3,+3], others any
	ui32 id; //id of object/artifact
	std::string description; 

	HeroBonus(ui8 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1)
		:duration(Dur), type(Type), source(Src), val(Val), id(ID), description(Desc), subtype(Subtype)
	{}
	HeroBonus(ui8 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, si32 Subtype=-1)
		:duration(Dur), type(Type), source(Src), val(Val), id(ID), subtype(Subtype)
	{}
	HeroBonus()
	{
		subtype = -1;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & duration & type & source & val & id & description;
	}

	static bool OneDay(const HeroBonus &hb)
	{
		return hb.duration==HeroBonus::ONE_DAY;
	}
	static bool OneWeek(const HeroBonus &hb)
	{
		return hb.duration==HeroBonus::ONE_WEEK;
	}
	static bool OneBattle(const HeroBonus &hb)
	{
		return hb.duration==HeroBonus::ONE_BATTLE;
	}
	static bool IsFrom(const HeroBonus &hb, ui8 source, ui32 id) //if id==0xffffff then id doesn't matter
	{
		return hb.source==source && (id==0xffffff  ||  hb.id==id);
	}
};
