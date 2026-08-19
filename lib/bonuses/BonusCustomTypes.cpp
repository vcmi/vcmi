/*
 * BonusCustomTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusCustomTypes.h"
#include "CBonusTypeHandler.h"
#include "GameLibrary.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"

const BonusCustomSubtype BonusCustomSubtype::creatureDamageBoth(0);
const BonusCustomSubtype BonusCustomSubtype::creatureDamageMin(1);
const BonusCustomSubtype BonusCustomSubtype::creatureDamageMax(2);
const BonusCustomSubtype BonusCustomSubtype::damageTypeAll(-1);
const BonusCustomSubtype BonusCustomSubtype::damageTypeMelee(0);
const BonusCustomSubtype BonusCustomSubtype::damageTypeRanged(1);
const BonusCustomSubtype BonusCustomSubtype::heroMovementLand(1);
const BonusCustomSubtype BonusCustomSubtype::heroMovementSea(0);
const BonusCustomSubtype BonusCustomSubtype::rebirthRegular(0);
const BonusCustomSubtype BonusCustomSubtype::rebirthSpecial(1);
const BonusCustomSubtype BonusCustomSubtype::visionsMonsters(0);
const BonusCustomSubtype BonusCustomSubtype::visionsHeroes(1);
const BonusCustomSubtype BonusCustomSubtype::visionsTowns(2);
const BonusCustomSubtype BonusCustomSubtype::immunityBattleWide(0);
const BonusCustomSubtype BonusCustomSubtype::immunityEnemyHero(1);
const BonusCustomSubtype BonusCustomSubtype::movementFlying(-1);
const BonusCustomSubtype BonusCustomSubtype::movementTeleporting(1);
const BonusCustomSubtype BonusCustomSubtype::freeShootingNoPenalty(0);
const BonusCustomSubtype BonusCustomSubtype::freeShootingExceptAdjacent(1);

const BonusCustomSource BonusCustomSource::undeadMoraleDebuff(-2);

BonusCustomSubtype BonusCustomSubtype::spellLevel(int level)
{
	return BonusCustomSubtype(level);
}

BonusCustomSubtype BonusCustomSubtype::creatureLevel(int level)
{
	return BonusCustomSubtype(level);
}

BonusCustomSubtype BonusCustomSubtype::alignment(EAlignment alignment)
{
	return BonusCustomSubtype(static_cast<int32_t>(alignment));
}

si32 BonusCustomSubtype::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusCustomSubtype::encode(const si32 index)
{
	return std::to_string(index);
}

si32 BonusCustomSource::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusCustomSource::encode(const si32 index)
{
	return std::to_string(index);
}

std::string BonusTypeID::encode(int32_t index)
{
	if (index == static_cast<int32_t>(BonusType::NONE))
		return "";
	return LIBRARY->bth->bonusToString(static_cast<BonusType>(index));
}

si32 BonusTypeID::decode(const std::string & identifier)
{
	if (identifier.empty())
		return RiverId::NO_RIVER.getNum();

	return resolveIdentifier("bonus", identifier);
}

BonusSubtypeKind bonusSubtypeKind(BonusType type)
{
	switch(type)
	{
		case BonusType::MAGIC_SCHOOL_SKILL:
		case BonusType::SPELL_DAMAGE:
		case BonusType::SPELLS_OF_SCHOOL:
		case BonusType::SPELL_DAMAGE_REDUCTION:
		case BonusType::SPELL_SCHOOL_IMMUNITY:
		case BonusType::NEGATIVE_EFFECTS_IMMUNITY:
			return BonusSubtypeKind::SPELL_SCHOOL;

		case BonusType::HATES_TRAIT:
			return BonusSubtypeKind::BONUS_TYPE;

		case BonusType::NO_TERRAIN_PENALTY:
			return BonusSubtypeKind::TERRAIN;

		case BonusType::COMBAT_EVENT_TRIGGER:
			return BonusSubtypeKind::SCRIPT;

		case BonusType::PRIMARY_SKILL:
			return BonusSubtypeKind::PRIMARY_SKILL;

		case BonusType::IMPROVED_NECROMANCY:
		case BonusType::HERO_GRANTS_ATTACKS:
		case BonusType::BONUS_DAMAGE_CHANCE:
		case BonusType::BONUS_DAMAGE_PERCENTAGE:
		case BonusType::SPECIAL_UPGRADE:
		case BonusType::HATE:
		case BonusType::MANUAL_CONTROL:
		case BonusType::SKELETON_TRANSFORMER_TARGET:
		case BonusType::DEITYOFFIRE:
			return BonusSubtypeKind::CREATURE;

		case BonusType::SPELL_IMMUNITY:
		case BonusType::SPELL_DURATION:
		case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
		case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
		case BonusType::SPECIAL_PECULIAR_ENCHANT:
		case BonusType::SPECIAL_SPELL_LEV:
		case BonusType::SPECIAL_SPELL_SCALING:
		case BonusType::SPECIFIC_SPELL_DAMAGE:
		case BonusType::SPECIFIC_SPELL_RANGE:
		case BonusType::SPELL:
		case BonusType::OPENING_BATTLE_SPELL:
		case BonusType::SPELL_LIKE_ATTACK:
		case BonusType::CATAPULT:
		case BonusType::CATAPULT_EXTRA_SHOTS:
		case BonusType::HEALER:
		case BonusType::SPELLCASTER:
		case BonusType::ENCHANTER:
		case BonusType::SPELL_AFTER_ATTACK:
		case BonusType::SPELL_BEFORE_ATTACK:
		case BonusType::SPECIFIC_SPELL_POWER:
		case BonusType::MORE_DAMAGE_FROM_SPELL:
		case BonusType::ADJACENT_SPELLCASTER:
		case BonusType::NOT_ACTIVE:
			return BonusSubtypeKind::SPELL;

		case BonusType::GENERATE_RESOURCE:
		case BonusType::RESOURCES_CONSTANT_BOOST:
		case BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST:
			return BonusSubtypeKind::RESOURCE;

		case BonusType::MOVEMENT:
		case BonusType::WATER_WALKING:
		case BonusType::FLYING_MOVEMENT:
		case BonusType::NEGATE_ALL_NATURAL_IMMUNITIES:
		case BonusType::CREATURE_DAMAGE:
		case BonusType::FLYING:
		case BonusType::FIRST_STRIKE:
		case BonusType::GENERAL_DAMAGE_REDUCTION:
		case BonusType::PERCENTAGE_DAMAGE_BOOST:
		case BonusType::REBIRTH:
		case BonusType::VISIONS:
		case BonusType::SPELLS_OF_LEVEL:
		case BonusType::CREATURE_GROWTH:
		case BonusType::ON_COMBAT_EVENT:
			return BonusSubtypeKind::CUSTOM;
		default:
			return BonusSubtypeKind::NONE;
	}
}

std::string bonusSubtypeEntity(BonusSubtypeKind kind)
{
	switch(kind)
	{
		case BonusSubtypeKind::CUSTOM:        return "bonusSubtype";
		case BonusSubtypeKind::SPELL:         return "spell";
		case BonusSubtypeKind::CREATURE:      return "creature";
		case BonusSubtypeKind::PRIMARY_SKILL: return "primarySkill";
		case BonusSubtypeKind::TERRAIN:       return "terrain";
		case BonusSubtypeKind::RESOURCE:      return "resource";
		case BonusSubtypeKind::SPELL_SCHOOL:  return "spellSchool";
		case BonusSubtypeKind::BONUS_TYPE:    return "bonus";
		case BonusSubtypeKind::SCRIPT:        return "script";
		default:                              return {};
	}
}

BonusSubtypeID bonusSubtypeOf(BonusSubtypeKind kind, int32_t index)
{
	switch(kind)
	{
		case BonusSubtypeKind::CUSTOM:        return BonusCustomSubtype(index);
		case BonusSubtypeKind::SPELL:         return SpellID(index);
		case BonusSubtypeKind::CREATURE:      return CreatureID(index);
		case BonusSubtypeKind::PRIMARY_SKILL: return PrimarySkill(index);
		case BonusSubtypeKind::TERRAIN:       return TerrainId(index);
		case BonusSubtypeKind::RESOURCE:      return GameResID(index);
		case BonusSubtypeKind::SPELL_SCHOOL:  return SpellSchool(index);
		case BonusSubtypeKind::BONUS_TYPE:    return BonusTypeID(index);
		case BonusSubtypeKind::SCRIPT:        return ScriptID(index);
		default:                              return BonusSubtypeID();
	}
}

BonusSubtypeID decodeBonusSubtype(BonusType type, const std::string & identifier)
{
	const auto kind = bonusSubtypeKind(type);

	if(kind == BonusSubtypeKind::NONE)
		throw std::runtime_error("Bonus type " + LIBRARY->bth->bonusToString(type) + " has no subtypes!");

	auto resolved = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), bonusSubtypeEntity(kind), identifier);

	if(!resolved)
		throw std::runtime_error("Identifier '" + identifier + "' names no subtype of bonus " + LIBRARY->bth->bonusToString(type));

	return bonusSubtypeOf(kind, *resolved);
}
