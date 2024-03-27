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

#include "BonusEnum.h"
#include "BonusParams.h"
#include "BonusSelector.h"

#include "../ResourceSet.h"
#include "../VCMI_Lib.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"

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
	"EARTH_SPELLS",
	"FIRE_IMMUNITY",
	"AIR_IMMUNITY",
	"WATER_IMMUNITY",
	"EARTH_IMMUNITY"
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
			type = BonusType::MANA_PER_KNOWLEDGE_PERCENTAGE;
			valueType = BonusValueType::PERCENT_TO_BASE;
		}
		else if(deprecatedSubtype == SecondarySkill::SORCERY || deprecatedSubtypeStr == "skill.sorcery")
		{
			type = BonusType::SPELL_DAMAGE;
			subtype = BonusSubtypeID(SpellSchool::ANY);
		}
		else if(deprecatedSubtype == SecondarySkill::SCHOLAR || deprecatedSubtypeStr == "skill.scholar")
			type = BonusType::LEARN_MEETING_SPELL_LIMIT;
		else if(deprecatedSubtype == SecondarySkill::ARCHERY|| deprecatedSubtypeStr == "skill.archery")
		{
			subtype = BonusCustomSubtype::damageTypeRanged;
			type = BonusType::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::OFFENCE || deprecatedSubtypeStr == "skill.offence")
		{
			subtype = BonusCustomSubtype::damageTypeMelee;
			type = BonusType::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::ARMORER || deprecatedSubtypeStr == "skill.armorer")
		{
			subtype = BonusCustomSubtype::damageTypeAll;
			type = BonusType::GENERAL_DAMAGE_REDUCTION;
		}
		else if(deprecatedSubtype == SecondarySkill::NAVIGATION || deprecatedSubtypeStr == "skill.navigation")
		{
			subtype = BonusCustomSubtype::heroMovementSea;
			valueType = BonusValueType::PERCENT_TO_BASE;
			type = BonusType::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::LOGISTICS || deprecatedSubtypeStr == "skill.logistics")
		{
			subtype = BonusCustomSubtype::heroMovementLand;
			valueType = BonusValueType::PERCENT_TO_BASE;
			type = BonusType::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::ESTATES || deprecatedSubtypeStr == "skill.estates")
		{
			type = BonusType::GENERATE_RESOURCE;
			subtype = BonusSubtypeID(GameResID(EGameResID::GOLD));
		}
		else if(deprecatedSubtype == SecondarySkill::AIR_MAGIC || deprecatedSubtypeStr == "skill.airMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = BonusSubtypeID(SpellSchool::AIR);
		}
		else if(deprecatedSubtype == SecondarySkill::WATER_MAGIC || deprecatedSubtypeStr == "skill.waterMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = BonusSubtypeID(SpellSchool::WATER);
		}
		else if(deprecatedSubtype == SecondarySkill::FIRE_MAGIC || deprecatedSubtypeStr == "skill.fireMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = BonusSubtypeID(SpellSchool::FIRE);
		}
		else if(deprecatedSubtype == SecondarySkill::EARTH_MAGIC || deprecatedSubtypeStr == "skill.earthMagic")
		{
			type = BonusType::MAGIC_SCHOOL_SKILL;
			subtype = BonusSubtypeID(SpellSchool::EARTH);
		}
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = BonusType::BONUS_DAMAGE_CHANCE;
			subtype = BonusSubtypeID(CreatureID(CreatureID::BALLISTA));
		}
		else if (deprecatedSubtype == SecondarySkill::FIRST_AID || deprecatedSubtypeStr == "skill.firstAid")
		{
			type = BonusType::SPECIFIC_SPELL_POWER;
			subtype = SpellID(*VLC->identifiers()->getIdentifier( ModScope::scopeGame(), "spell", "firstAid"));
		}
		else if (deprecatedSubtype == SecondarySkill::BALLISTICS || deprecatedSubtypeStr == "skill.ballistics")
		{
			type = BonusType::CATAPULT_EXTRA_SHOTS;
			subtype = SpellID(*VLC->identifiers()->getIdentifier( ModScope::scopeGame(), "spell", "catapultShot"));
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
			subtype = BonusSubtypeID(CreatureID(CreatureID::BALLISTA));
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SEA_MOVEMENT")
	{
		subtype = BonusCustomSubtype::heroMovementSea;
		valueType = BonusValueType::ADDITIVE_VALUE;
		type = BonusType::MOVEMENT;
	}
	else if (deprecatedTypeStr == "LAND_MOVEMENT")
	{
		subtype = BonusCustomSubtype::heroMovementLand;
		valueType = BonusValueType::ADDITIVE_VALUE;
		type = BonusType::MOVEMENT;
	}
	else if (deprecatedTypeStr == "MAXED_SPELL")
	{
		type = BonusType::SPELL;
		subtype = SpellID(*VLC->identifiers()->getIdentifier( ModScope::scopeGame(), "spell", deprecatedSubtypeStr));
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
		subtype = BonusSubtypeID(SpellSchool::ANY);
		val = 100;
	}
	else if (deprecatedTypeStr == "AIR_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = BonusSubtypeID(SpellSchool::AIR);
	}
	else if (deprecatedTypeStr == "FIRE_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = BonusSubtypeID(SpellSchool::FIRE);
	}
	else if (deprecatedTypeStr == "WATER_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = BonusSubtypeID(SpellSchool::WATER);
	}
	else if (deprecatedTypeStr == "EARTH_SPELL_DMG_PREMY")
	{
		type = BonusType::SPELL_DAMAGE;
		subtype = BonusSubtypeID(SpellSchool::EARTH);
	}
	else if (deprecatedTypeStr == "AIR_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = BonusSubtypeID(SpellSchool::AIR);
	}
	else if (deprecatedTypeStr == "FIRE_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = BonusSubtypeID(SpellSchool::FIRE);
	}
	else if (deprecatedTypeStr == "WATER_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = BonusSubtypeID(SpellSchool::WATER);
	}
	else if (deprecatedTypeStr == "EARTH_SPELLS")
	{
		type = BonusType::SPELLS_OF_SCHOOL;
		subtype = BonusSubtypeID(SpellSchool::EARTH);
	}
	else if (deprecatedTypeStr == "AIR_IMMUNITY")
	{
		subtype = BonusSubtypeID(SpellSchool::AIR);
		switch(deprecatedSubtype)
		{
			case 0:
				type = BonusType::SPELL_SCHOOL_IMMUNITY;
				break;
			case 1:
				type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				break;
			default:
				type = BonusType::SPELL_DAMAGE_REDUCTION;
				val = 100;
		}
	}
	else if (deprecatedTypeStr == "FIRE_IMMUNITY")
	{
		subtype = BonusSubtypeID(SpellSchool::FIRE);
		switch(deprecatedSubtype)
		{
			case 0:
				type = BonusType::SPELL_SCHOOL_IMMUNITY;
				break;
			case 1:
				type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				break;
			default:
				type = BonusType::SPELL_DAMAGE_REDUCTION;
				val = 100;
		}
	}
	else if (deprecatedTypeStr == "WATER_IMMUNITY")
	{
		subtype = BonusSubtypeID(SpellSchool::WATER);
		switch(deprecatedSubtype)
		{
			case 0:
				type = BonusType::SPELL_SCHOOL_IMMUNITY;
				break;
			case 1:
				type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				break;
			default:
				type = BonusType::SPELL_DAMAGE_REDUCTION;
				val = 100;
		}
	}
	else if (deprecatedTypeStr == "EARTH_IMMUNITY")
	{
		subtype = BonusSubtypeID(SpellSchool::EARTH);
		switch(deprecatedSubtype)
		{
			case 0:
				type = BonusType::SPELL_SCHOOL_IMMUNITY;
				break;
			case 1:
				type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				break;
			default:
				type = BonusType::SPELL_DAMAGE_REDUCTION;
				val = 100;
		}
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
		if(subtype)
			ret["subtype"].String() = subtype->toString();
		if(valueType)
			ret["valueType"].String() = vstd::findKey(bonusValueMap, *valueType);
		if(val)
			ret["val"].Float() = *val;
		if(targetType)
			ret["targetSourceType"].String() = vstd::findKey(bonusSourceMap, *targetType);
		jsonCreated = true;
	}
	ret.setModScope(ModScope::scopeGame());
	return ret;
};

CSelector BonusParams::toSelector()
{
	assert(isConverted);

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
