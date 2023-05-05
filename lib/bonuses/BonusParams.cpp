/*
 * BonusParams.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BonusParams.h"
#include "BonusSelector.h"

#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::set<std::string> deprecatedBonusSet = {
	"SECONDARY_SKILL_PREMY",
	"SECONDARY_SKILL_VAL2",
	"MAXED_SPELL",
	"LAND_MOVEMENT",
	"SEA_MOVEMENT",
	"SIGHT_RADIOUS",
	"NO_TYPE",
	"SPECIAL_SECONDARY_SKILL",
	"FULL_HP_REGENERATION",
	"KING1",
	"KING2",
	"KING3",
	"BLOCK_MORALE",
	"BLOCK_LUCK",
	"SELF_MORALE",
	"SELF_LUCK",
	"DIRECT_DAMAGE_IMMUNITY",
	"AIR_SPELL_DMG_PREMY",
	"EARTH_SPELL_DMG_PREMY"
	"FIRE_SPELL_DMG_PREMY"
	"WATER_SPELL_DMG_PREMY",
	"FIRE_SPELLS",
	"AIR_SPELLS",
	"WATER_SPELLS",
	"EARTH_SPELLS"
};

BonusParams::BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr, int deprecatedSubtype):
	isConverted(true)
{
	if(deprecatedTypeStr == "SECONDARY_SKILL_PREMY" || deprecatedTypeStr == "SPECIAL_SECONDARY_SKILL")
	{
		if(deprecatedSubtype == SecondarySkill::PATHFINDING || deprecatedSubtypeStr == "skill.pathfinding")
			type = BonusType::ROUGH_TERRAIN_DISCOUNT;
		else if(deprecatedSubtype == SecondarySkill::DIPLOMACY || deprecatedSubtypeStr == "skill.diplomacy")
			type = BonusType::WANDERING_CREATURES_JOIN_BONUS;
		else if(deprecatedSubtype == SecondarySkill::WISDOM || deprecatedSubtypeStr == "skill.wisdom")
			type = BonusType::MAX_LEARNABLE_SPELL_LEVEL;
		else if(deprecatedSubtype == SecondarySkill::MYSTICISM || deprecatedSubtypeStr == "skill.mysticism")
			type = BonusType::MANA_REGENERATION;
		else if(deprecatedSubtype == SecondarySkill::NECROMANCY || deprecatedSubtypeStr == "skill.necromancy")
			type = BonusType::UNDEAD_RAISE_PERCENTAGE;
		else if(deprecatedSubtype == SecondarySkill::LEARNING || deprecatedSubtypeStr == "skill.learning")
			type = BonusType::HERO_EXPERIENCE_GAIN_PERCENT;
		else if(deprecatedSubtype == SecondarySkill::RESISTANCE || deprecatedSubtypeStr == "skill.resistance")
			type = BonusType::MAGIC_RESISTANCE;
		else if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = BonusType::LEARN_BATTLE_SPELL_CHANCE;
		else if(deprecatedSubtype == SecondarySkill::SCOUTING || deprecatedSubtypeStr == "skill.scouting")
			type = BonusType::SIGHT_RADIUS;
		else if(deprecatedSubtype == SecondarySkill::INTELLIGENCE || deprecatedSubtypeStr == "skill.intelligence")
		{
			type = BonusType::MANA_PER_KNOWLEDGE;
			valueType = BonusValueType::PERCENT_TO_BASE;
		}
		else if(deprecatedSubtype == SecondarySkill::SORCERY || deprecatedSubtypeStr == "skill.sorcery")
			type = BonusType::SPELL_DAMAGE;
		else if(deprecatedSubtype == SecondarySkill::SCHOLAR || deprecatedSubtypeStr == "skill.scholar")
			type = BonusType::LEARN_MEETING_SPELL_LIMIT;
		else if(deprecatedSubtype == SecondarySkill::ARCHERY|| deprecatedSubtypeStr == "skill.archery")
		{
			subtype = 1;
			type = BonusType::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::OFFENCE || deprecatedSubtypeStr == "skill.offence")
		{
			subtype = 0;
			type = BonusType::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::ARMORER || deprecatedSubtypeStr == "skill.armorer")
		{
			subtype = -1;
			type = BonusType::GENERAL_DAMAGE_REDUCTION;
		}
		else if(deprecatedSubtype == SecondarySkill::NAVIGATION || deprecatedSubtypeStr == "skill.navigation")
		{
			subtype = 0;
			valueType = BonusValueType::PERCENT_TO_BASE;
			type = BonusType::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::LOGISTICS || deprecatedSubtypeStr == "skill.logistics")
		{
			subtype = 1;
			valueType = BonusValueType::PERCENT_TO_BASE;
			type = BonusType::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::ESTATES || deprecatedSubtypeStr == "skill.estates")
		{
			type = BonusType::GENERATE_RESOURCE;
			subtype = GameResID(EGameResID::GOLD);
		}
		else if(deprecatedSubtype == SecondarySkill::AIR_MAGIC || deprecatedSubtypeStr == "skill.airMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = 4;
		}
		else if(deprecatedSubtype == SecondarySkill::WATER_MAGIC || deprecatedSubtypeStr == "skill.waterMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = 1;
		}
		else if(deprecatedSubtype == SecondarySkill::FIRE_MAGIC || deprecatedSubtypeStr == "skill.fireMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = 2;
		}
		else if(deprecatedSubtype == SecondarySkill::EARTH_MAGIC || deprecatedSubtypeStr == "skill.earthMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = 8;
		}
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = BonusType::BONUS_DAMAGE_CHANCE;
			subtypeStr = "core:creature.ballista";
		}
		else if (deprecatedSubtype == SecondarySkill::FIRST_AID || deprecatedSubtypeStr == "skill.firstAid")
		{
			type = BonusType::SPECIFIC_SPELL_POWER;
			subtypeStr = "core:spell.firstAid";
		}
		else if (deprecatedSubtype == SecondarySkill::BALLISTICS || deprecatedSubtypeStr == "skill.ballistics")
		{
			type = BonusType::CATAPULT_EXTRA_SHOTS;
			subtypeStr = "core:spell.catapultShot";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SECONDARY_SKILL_VAL2")
	{
		if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT;
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = BonusType::HERO_GRANTS_ATTACKS;
			subtypeStr = "core:creature.ballista";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SEA_MOVEMENT")
	{
		subtype = 0;
		valueType = BonusValueType::ADDITIVE_VALUE;
		type = BonusType::MOVEMENT;
	}
	else if (deprecatedTypeStr == "LAND_MOVEMENT")
	{
		subtype = 1;
		valueType = BonusValueType::ADDITIVE_VALUE;
		type = BonusType::MOVEMENT;
	}
	else if (deprecatedTypeStr == "MAXED_SPELL")
	{
		type = BonusType::SPELL;
		subtypeStr = deprecatedSubtypeStr;
		valueType = BonusValueType::INDEPENDENT_MAX;
		val = 3;
	}
	else if (deprecatedTypeStr == "FULL_HP_REGENERATION")
	{
		type = BonusType::HP_REGENERATION;
		val = 100000; //very high value to always chose stack health
	}
	else if (deprecatedTypeStr == "KING1")
	{
		type = BonusType::KING;
		val = 0;
	}
	else if (deprecatedTypeStr == "KING2")
	{
		type = BonusType::KING;
		val = 2;
	}
	else if (deprecatedTypeStr == "KING3")
	{
		type = BonusType::KING;
		val = 3;
	}
	else if (deprecatedTypeStr == "SIGHT_RADIOUS")
		type = BonusType::SIGHT_RADIUS;
	else if (deprecatedTypeStr == "SELF_MORALE")
	{
		type = BonusType::MORALE;
		val = 1;
		valueType = BonusValueType::INDEPENDENT_MAX;
	}
	else if (deprecatedTypeStr == "SELF_LUCK")
	{
		type = BonusType::LUCK;
		val = 1;
		valueType = BonusValueType::INDEPENDENT_MAX;
	}
	else if (deprecatedTypeStr == "DIRECT_DAMAGE_IMMUNITY")
	{
		type = BonusType::SPELL_DAMAGE_REDUCTION;
		val = 100;
	}
	else if (deprecatedTypeStr == "AIR_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = 0;
	}
	else if (deprecatedTypeStr == "FIRE_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = 1;
	}
	else if (deprecatedTypeStr == "WATER_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = 2;
	}
	else if (deprecatedTypeStr == "EARTH_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = 3;
	}
	else if (deprecatedTypeStr == "AIR_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = 0;
	}
	else if (deprecatedTypeStr == "FIRE_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = 1;
	}
	else if (deprecatedTypeStr == "WATER_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = 2;
	}
	else if (deprecatedTypeStr == "EARTH_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = 3;
	}
	else
		isConverted = false;
}

const JsonNode & BonusParams::toJson()
{
	assert(isConverted);
	if(ret.isNull())
	{
		ret["type"].String() = vstd::findKey(bonusNameMap, type);
		if(subtypeStr)
			ret["subtype"].String() = *subtypeStr;
		else if(subtype)
			ret["subtype"].Integer() = *subtype;
		if(valueType)
			ret["valueType"].String() = vstd::findKey(bonusValueMap, *valueType);
		if(val)
			ret["val"].Float() = *val;
		if(targetType)
			ret["targetSourceType"].String() = vstd::findKey(bonusSourceMap, *targetType);
		jsonCreated = true;
	}
	return ret;
};

CSelector BonusParams::toSelector()
{
	assert(isConverted);
	if(subtypeStr)
	{
		subtype = -1;
		JsonUtils::resolveIdentifier(*subtype, toJson(), "subtype");
	}

	auto ret = Selector::type()(type);
	if(subtype)
		ret = ret.And(Selector::subtype()(*subtype));
	if(valueType)
		ret = ret.And(Selector::valueType(*valueType));
	if(targetType)
		ret = ret.And(Selector::targetSourceType()(*targetType));
	return ret;
}

VCMI_LIB_NAMESPACE_END