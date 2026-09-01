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

EntityTypeEnum bonusSubtypeEntityType(BonusType type)
{
	switch(type)
	{
		case BonusType::MAGIC_SCHOOL_SKILL:
		case BonusType::SPELL_DAMAGE:
		case BonusType::SPELLS_OF_SCHOOL:
		case BonusType::SPELL_DAMAGE_REDUCTION:
		case BonusType::SPELL_SCHOOL_IMMUNITY:
		case BonusType::NEGATIVE_EFFECTS_IMMUNITY:
			return EntityTypeEnum::SPELL_SCHOOL;

		case BonusType::HATES_TRAIT:
			return EntityTypeEnum::BONUS_TYPE;

		case BonusType::NO_TERRAIN_PENALTY:
			return EntityTypeEnum::TERRAIN;

		case BonusType::COMBAT_EVENT_TRIGGER:
			return EntityTypeEnum::SCRIPT;

		case BonusType::PRIMARY_SKILL:
			return EntityTypeEnum::PRIMARY_SKILL;

		case BonusType::IMPROVED_NECROMANCY:
		case BonusType::HERO_GRANTS_ATTACKS:
		case BonusType::BONUS_DAMAGE_CHANCE:
		case BonusType::BONUS_DAMAGE_PERCENTAGE:
		case BonusType::SPECIAL_UPGRADE:
		case BonusType::HATE:
		case BonusType::MANUAL_CONTROL:
		case BonusType::SKELETON_TRANSFORMER_TARGET:
		case BonusType::DEITYOFFIRE:
			return EntityTypeEnum::CREATURE;

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
			return EntityTypeEnum::SPELL;

		case BonusType::GENERATE_RESOURCE:
		case BonusType::RESOURCES_CONSTANT_BOOST:
		case BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST:
			return EntityTypeEnum::RESOURCE;

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
        case BonusType::FREE_SHOOTING:
        case BonusType::ALIGNMENT_MIX: // alignment
			return EntityTypeEnum::CUSTOM;
		default:
			return EntityTypeEnum::NONE;
	}
}

std::string entityTypeName(EntityTypeEnum entityType)
{
	switch(entityType)
	{
		case EntityTypeEnum::CUSTOM:        return "bonusSubtype";
		case EntityTypeEnum::SPELL:         return "spell";
		case EntityTypeEnum::CREATURE:      return "creature";
		case EntityTypeEnum::PRIMARY_SKILL: return "primarySkill";
		case EntityTypeEnum::TERRAIN:       return "terrain";
		case EntityTypeEnum::RESOURCE:      return "resource";
		case EntityTypeEnum::SPELL_SCHOOL:  return "spellSchool";
		case EntityTypeEnum::BONUS_TYPE:    return "bonus";
		case EntityTypeEnum::SCRIPT:        return "script";
		default:                           return {};
	}
}

BonusSubtypeID bonusSubtypeOf(EntityTypeEnum entityType, int32_t index)
{
	switch(entityType)
	{
		case EntityTypeEnum::CUSTOM:        return BonusCustomSubtype(index);
		case EntityTypeEnum::SPELL:         return SpellID(index);
		case EntityTypeEnum::CREATURE:      return CreatureID(index);
		case EntityTypeEnum::PRIMARY_SKILL: return PrimarySkill(index);
		case EntityTypeEnum::TERRAIN:       return TerrainId(index);
		case EntityTypeEnum::RESOURCE:      return GameResID(index);
		case EntityTypeEnum::SPELL_SCHOOL:  return SpellSchool(index);
		case EntityTypeEnum::BONUS_TYPE:    return BonusTypeID(index);
		case EntityTypeEnum::SCRIPT:        return ScriptID(index);
		default:                           return BonusSubtypeID();
	}
}

BonusSubtypeID decodeBonusSubtype(BonusType type, const std::string & identifier)
{
	const auto entityType = bonusSubtypeEntityType(type);

	if(entityType == EntityTypeEnum::NONE)
		throw std::runtime_error("Bonus type " + LIBRARY->bth->bonusToString(type) + " has no subtypes!");

	auto resolved = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), entityTypeName(entityType), identifier);

	if(!resolved)
		throw std::runtime_error("Identifier '" + identifier + "' names no subtype of bonus " + LIBRARY->bth->bonusToString(type));

	return bonusSubtypeOf(entityType, *resolved);
}
