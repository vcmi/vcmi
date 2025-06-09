/*
 * BasicTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "GameLibrary.h"
#include "GameConstants.h"
#include "IGameSettings.h"
#include "bonuses/BonusList.h"
#include "bonuses/Bonus.h"
#include "bonuses/IBonusBearer.h"

#include <vcmi/Creature.h>
#include <vcmi/Faction.h>
#include <vcmi/FactionMember.h>
#include <vcmi/FactionService.h>

VCMI_LIB_NAMESPACE_BEGIN

bool INativeTerrainProvider::isNativeTerrain(TerrainId terrain) const
{
	auto native = getNativeTerrain();
	return native == terrain || native == ETerrainId::ANY_TERRAIN;
}

TerrainId AFactionMember::getNativeTerrain() const
{
	//this code is used in the CreatureTerrainLimiter::limit to setup battle bonuses
	//and in the CGHeroInstance::getNativeTerrain() to setup movement bonuses or/and penalties.
	return getBonusBearer()->hasBonusOfType(BonusType::TERRAIN_NATIVE)
			 ? TerrainId::ANY_TERRAIN : getFactionID().toEntity(LIBRARY)->getNativeTerrain();
}

int32_t AFactionMember::magicResistance() const
{
	si32 val = getBonusBearer()->valOfBonuses(BonusType::MAGIC_RESISTANCE);
	vstd::amin (val, 100);
	return val;
}

int AFactionMember::getAttack(bool ranged) const
{
	return getBonusBearer()->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK));
}

int AFactionMember::getDefense(bool ranged) const
{
	return getBonusBearer()->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE));
}

int AFactionMember::getMinDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_1";
	static const auto selector = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageBoth).Or(Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMin));
	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int AFactionMember::getMaxDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_2";
	static const auto selector = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageBoth).Or(Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMax));
	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

bool AFactionMember::unaffectedByMorale() const
{
	static const auto unaffectedByMoraleSelector = Selector::type()(BonusType::NON_LIVING).Or(Selector::type()(BonusType::MECHANICAL)).Or(Selector::type()(BonusType::UNDEAD))
													   .Or(Selector::type()(BonusType::SIEGE_WEAPON)).Or(Selector::type()(BonusType::NO_MORALE));

	static const std::string cachingStrUn = "AFactionMember::unaffectedByMoraleSelector";
	return getBonusBearer()->hasBonus(unaffectedByMoraleSelector, cachingStrUn);
}

int AFactionMember::moraleValAndBonusList(TConstBonusListPtr & bonusList) const
{
	int32_t maxGoodMorale = LIBRARY->engineSettings()->getVector(EGameSettings::COMBAT_GOOD_MORALE_CHANCE).size();
	int32_t maxBadMorale = - (int32_t) LIBRARY->engineSettings()->getVector(EGameSettings::COMBAT_BAD_MORALE_CHANCE).size();

	if(getBonusBearer()->hasBonusOfType(BonusType::MAX_MORALE))
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return maxGoodMorale;
	}

	if(unaffectedByMorale())
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return 0;
	}

	bonusList = getBonusBearer()->getBonusesOfType(BonusType::MORALE);

	return std::clamp(bonusList->totalValue(), maxBadMorale, maxGoodMorale);
}

int AFactionMember::luckValAndBonusList(TConstBonusListPtr & bonusList) const
{
	int32_t maxGoodLuck = LIBRARY->engineSettings()->getVector(EGameSettings::COMBAT_GOOD_LUCK_CHANCE).size();
	int32_t maxBadLuck = - (int32_t) LIBRARY->engineSettings()->getVector(EGameSettings::COMBAT_BAD_LUCK_CHANCE).size();

	if(getBonusBearer()->hasBonusOfType(BonusType::MAX_LUCK))
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return maxGoodLuck;
	}

	if(getBonusBearer()->hasBonusOfType(BonusType::NO_LUCK))
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return 0;
	}

	bonusList = getBonusBearer()->getBonusesOfType(BonusType::LUCK);

	return std::clamp(bonusList->totalValue(), maxBadLuck, maxGoodLuck);
}

int AFactionMember::moraleVal() const
{
	TConstBonusListPtr tmp = nullptr;
	return moraleValAndBonusList(tmp);
}

int AFactionMember::luckVal() const
{
	TConstBonusListPtr tmp = nullptr;
	return luckValAndBonusList(tmp);
}

ui32 ACreature::getMaxHealth() const
{
	auto value = getBonusBearer()->valOfBonuses(BonusType::STACK_HEALTH);
	return std::max(1, value); //never 0
}

ui32 ACreature::getMovementRange() const
{
	//war machines cannot move
	if (getBonusBearer()->hasBonusOfType(BonusType::SIEGE_WEAPON))
		return 0;

	if (getBonusBearer()->hasBonusOfType(BonusType::BIND_EFFECT))
		return 0;

	return getBonusBearer()->valOfBonuses(BonusType::STACKS_SPEED);
}

int32_t ACreature::getInitiative(int turn) const
{
	if (turn == 0)
	{
		return getBonusBearer()->valOfBonuses(BonusType::STACKS_SPEED);
	}
	else
	{
		const std::string cachingStrSS = "type_STACKS_SPEED_turns_" + std::to_string(turn);
		return getBonusBearer()->valOfBonuses(Selector::type()(BonusType::STACKS_SPEED).And(Selector::turns(turn)), cachingStrSS);
	}
}

ui32 ACreature::getMovementRange(int turn) const
{
	if (turn == 0)
		return getMovementRange();

	const std::string cachingStrSW = "type_SIEGE_WEAPON_turns_" + std::to_string(turn);
	const std::string cachingStrBE = "type_BIND_EFFECT_turns_" + std::to_string(turn);
	const std::string cachingStrSS = "type_STACKS_SPEED_turns_" + std::to_string(turn);

	//war machines cannot move
	if(getBonusBearer()->hasBonus(Selector::type()(BonusType::SIEGE_WEAPON).And(Selector::turns(turn)), cachingStrSW))
		return 0;

	if(getBonusBearer()->hasBonus(Selector::type()(BonusType::BIND_EFFECT).And(Selector::turns(turn)), cachingStrBE))
		return 0;

	return getBonusBearer()->valOfBonuses(Selector::type()(BonusType::STACKS_SPEED).And(Selector::turns(turn)), cachingStrSS);
}

bool ACreature::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	return getBonusBearer()->hasBonusOfType(BonusType::LIVING);
}


VCMI_LIB_NAMESPACE_END
