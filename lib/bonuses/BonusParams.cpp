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

#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

BonusParams::BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr, int deprecatedSubtype):
	isConverted(true)
{
	if(deprecatedTypeStr == "SECONDARY_SKILL_PREMY" || deprecatedTypeStr == "SPECIAL_SECONDARY_SKILL")
	{
		if(deprecatedSubtype == SecondarySkill::PATHFINDING || deprecatedSubtypeStr == "skill.pathfinding")
			type = Bonus::ROUGH_TERRAIN_DISCOUNT;
		else if(deprecatedSubtype == SecondarySkill::DIPLOMACY || deprecatedSubtypeStr == "skill.diplomacy")
			type = Bonus::WANDERING_CREATURES_JOIN_BONUS;
		else if(deprecatedSubtype == SecondarySkill::WISDOM || deprecatedSubtypeStr == "skill.wisdom")
			type = Bonus::MAX_LEARNABLE_SPELL_LEVEL;
		else if(deprecatedSubtype == SecondarySkill::MYSTICISM || deprecatedSubtypeStr == "skill.mysticism")
			type = Bonus::MANA_REGENERATION;
		else if(deprecatedSubtype == SecondarySkill::NECROMANCY || deprecatedSubtypeStr == "skill.necromancy")
			type = Bonus::UNDEAD_RAISE_PERCENTAGE;
		else if(deprecatedSubtype == SecondarySkill::LEARNING || deprecatedSubtypeStr == "skill.learning")
			type = Bonus::HERO_EXPERIENCE_GAIN_PERCENT;
		else if(deprecatedSubtype == SecondarySkill::RESISTANCE || deprecatedSubtypeStr == "skill.resistance")
			type = Bonus::MAGIC_RESISTANCE;
		else if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = Bonus::LEARN_BATTLE_SPELL_CHANCE;
		else if(deprecatedSubtype == SecondarySkill::SCOUTING || deprecatedSubtypeStr == "skill.scouting")
			type = Bonus::SIGHT_RADIUS;
		else if(deprecatedSubtype == SecondarySkill::INTELLIGENCE || deprecatedSubtypeStr == "skill.intelligence")
		{
			type = Bonus::MANA_PER_KNOWLEDGE;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
		}
		else if(deprecatedSubtype == SecondarySkill::SORCERY || deprecatedSubtypeStr == "skill.sorcery")
			type = Bonus::SPELL_DAMAGE;
		else if(deprecatedSubtype == SecondarySkill::SCHOLAR || deprecatedSubtypeStr == "skill.scholar")
			type = Bonus::LEARN_MEETING_SPELL_LIMIT;
		else if(deprecatedSubtype == SecondarySkill::ARCHERY|| deprecatedSubtypeStr == "skill.archery")
		{
			subtype = 1;
			subtypeRelevant = true;
			type = Bonus::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::OFFENCE || deprecatedSubtypeStr == "skill.offence")
		{
			subtype = 0;
			subtypeRelevant = true;
			type = Bonus::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::ARMORER || deprecatedSubtypeStr == "skill.armorer")
		{
			subtype = -1;
			subtypeRelevant = true;
			type = Bonus::GENERAL_DAMAGE_REDUCTION;
		}
		else if(deprecatedSubtype == SecondarySkill::NAVIGATION || deprecatedSubtypeStr == "skill.navigation")
		{
			subtype = 0;
			subtypeRelevant = true;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
			type = Bonus::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::LOGISTICS || deprecatedSubtypeStr == "skill.logistics")
		{
			subtype = 1;
			subtypeRelevant = true;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
			type = Bonus::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::ESTATES || deprecatedSubtypeStr == "skill.estates")
		{
			type = Bonus::GENERATE_RESOURCE;
			subtype = GameResID(EGameResID::GOLD);
			subtypeRelevant = true;
		}
		else if(deprecatedSubtype == SecondarySkill::AIR_MAGIC || deprecatedSubtypeStr == "skill.airMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 4;
		}
		else if(deprecatedSubtype == SecondarySkill::WATER_MAGIC || deprecatedSubtypeStr == "skill.waterMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 1;
		}
		else if(deprecatedSubtype == SecondarySkill::FIRE_MAGIC || deprecatedSubtypeStr == "skill.fireMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 2;
		}
		else if(deprecatedSubtype == SecondarySkill::EARTH_MAGIC || deprecatedSubtypeStr == "skill.earthMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 8;
		}
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = Bonus::BONUS_DAMAGE_CHANCE;
			subtypeRelevant = true;
			subtypeStr = "core:creature.ballista";
		}
		else if (deprecatedSubtype == SecondarySkill::FIRST_AID || deprecatedSubtypeStr == "skill.firstAid")
		{
			type = Bonus::SPECIFIC_SPELL_POWER;
			subtypeRelevant = true;
			subtypeStr = "core:spell.firstAid";
		}
		else if (deprecatedSubtype == SecondarySkill::BALLISTICS || deprecatedSubtypeStr == "skill.ballistics")
		{
			type = Bonus::CATAPULT_EXTRA_SHOTS;
			subtypeRelevant = true;
			subtypeStr = "core:spell.catapultShot";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SECONDARY_SKILL_VAL2")
	{
		if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = Bonus::LEARN_BATTLE_SPELL_LEVEL_LIMIT;
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = Bonus::HERO_GRANTS_ATTACKS;
			subtypeRelevant = true;
			subtypeStr = "core:creature.ballista";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SEA_MOVEMENT")
	{
		subtype = 0;
		subtypeRelevant = true;
		valueType = Bonus::ADDITIVE_VALUE;
		valueTypeRelevant = true;
		type = Bonus::MOVEMENT;
	}
	else if (deprecatedTypeStr == "LAND_MOVEMENT")
	{
		subtype = 1;
		subtypeRelevant = true;
		valueType = Bonus::ADDITIVE_VALUE;
		valueTypeRelevant = true;
		type = Bonus::MOVEMENT;
	}
	else if (deprecatedTypeStr == "MAXED_SPELL")
	{
		type = Bonus::SPELL;
		subtypeStr = deprecatedSubtypeStr;
		subtypeRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
		val = 3;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "FULL_HP_REGENERATION")
	{
		type = Bonus::HP_REGENERATION;
		val = 100000; //very high value to always chose stack health
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING1")
	{
		type = Bonus::KING;
		val = 0;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING2")
	{
		type = Bonus::KING;
		val = 2;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING3")
	{
		type = Bonus::KING;
		val = 3;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "SIGHT_RADIOUS")
		type = Bonus::SIGHT_RADIUS;
	else if (deprecatedTypeStr == "SELF_MORALE")
	{
		type = Bonus::MORALE;
		val = 1;
		valRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
	}
	else if (deprecatedTypeStr == "SELF_LUCK")
	{
		type = Bonus::LUCK;
		val = 1;
		valRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
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
		if(subtypeRelevant && !subtypeStr.empty())
			ret["subtype"].String() = subtypeStr;
		else if(subtypeRelevant)
			ret["subtype"].Integer() = subtype;
		if(valueTypeRelevant)
			ret["valueType"].String() = vstd::findKey(bonusValueMap, valueType);
		if(valRelevant)
			ret["val"].Float() = val;
		if(targetTypeRelevant)
			ret["targetSourceType"].String() = vstd::findKey(bonusSourceMap, targetType);
		jsonCreated = true;
	}
	return ret;
};

CSelector BonusParams::toSelector()
{
	assert(isConverted);
	if(subtypeRelevant && !subtypeStr.empty())
		JsonUtils::resolveIdentifier(subtype, toJson(), "subtype");

	auto ret = Selector::type()(type);
	if(subtypeRelevant)
		ret = ret.And(Selector::subtype()(subtype));
	if(valueTypeRelevant)
		ret = ret.And(Selector::valueType(valueType));
	if(targetTypeRelevant)
		ret = ret.And(Selector::targetSourceType()(targetType));
	return ret;
}

VCMI_LIB_NAMESPACE_END