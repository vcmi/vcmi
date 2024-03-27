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

#include "VCMI_Lib.h"
#include "GameConstants.h"
#include "GameSettings.h"
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
	const std::string cachingStringNoTerrainPenalty = "type_TERRAIN_NATIVE_NONE";
	static const auto selectorNoTerrainPenalty = Selector::typeSubtype(BonusType::TERRAIN_NATIVE, BonusSubtypeID());

	//this code is used in the CreatureTerrainLimiter::limit to setup battle bonuses
	//and in the CGHeroInstance::getNativeTerrain() to setup movement bonuses or/and penalties.
	return getBonusBearer()->hasBonus(selectorNoTerrainPenalty, cachingStringNoTerrainPenalty)
		? TerrainId::ANY_TERRAIN : VLC->factions()->getById(getFaction())->getNativeTerrain();
}

int32_t AFactionMember::magicResistance() const
{
	si32 val = getBonusBearer()->valOfBonuses(Selector::type()(BonusType::MAGIC_RESISTANCE));
	vstd::amin (val, 100);
	return val;
}

int AFactionMember::getAttack(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_ATTACK";

	static const auto selector = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK));

	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int AFactionMember::getDefense(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_DEFENSE";

	static const auto selector = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE));

	return getBonusBearer()->valOfBonuses(selector, cachingStr);
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

int AFactionMember::getPrimSkillLevel(PrimarySkill id) const
{
	static const CSelector selectorAllSkills = Selector::type()(BonusType::PRIMARY_SKILL);
	static const std::string keyAllSkills = "type_PRIMARY_SKILL";
	auto allSkills = getBonusBearer()->getBonuses(selectorAllSkills, keyAllSkills);
	auto ret = allSkills->valOfBonuses(Selector::subtype()(BonusSubtypeID(id)));
	auto minSkillValue = (id == PrimarySkill::SPELL_POWER || id == PrimarySkill::KNOWLEDGE) ? 1 : 0;
	return std::max(ret, minSkillValue); //otherwise, some artifacts may cause negative skill value effect, sp=0 works in old saves
}

int AFactionMember::moraleValAndBonusList(TConstBonusListPtr & bonusList) const
{
	int32_t maxGoodMorale = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_MORALE_DICE).size();
	int32_t maxBadMorale = - (int32_t) VLC->settings()->getVector(EGameSettings::COMBAT_BAD_MORALE_DICE).size();

	if(getBonusBearer()->hasBonusOfType(BonusType::MAX_MORALE))
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return maxGoodMorale;
	}

	static const auto unaffectedByMoraleSelector = Selector::type()(BonusType::NON_LIVING).Or(Selector::type()(BonusType::UNDEAD))
													.Or(Selector::type()(BonusType::SIEGE_WEAPON)).Or(Selector::type()(BonusType::NO_MORALE));

	static const std::string cachingStrUn = "AFactionMember::unaffectedByMoraleSelector";
	auto unaffected = getBonusBearer()->hasBonus(unaffectedByMoraleSelector, cachingStrUn);
	if(unaffected)
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return 0;
	}

	static const auto moraleSelector = Selector::type()(BonusType::MORALE);
	static const std::string cachingStrMor = "type_MORALE";
	bonusList = getBonusBearer()->getBonuses(moraleSelector, cachingStrMor);

	return std::clamp(bonusList->totalValue(), maxBadMorale, maxGoodMorale);
}

int AFactionMember::luckValAndBonusList(TConstBonusListPtr & bonusList) const
{
	int32_t maxGoodLuck = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_LUCK_DICE).size();
	int32_t maxBadLuck = - (int32_t) VLC->settings()->getVector(EGameSettings::COMBAT_BAD_LUCK_DICE).size();

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

	static const auto luckSelector = Selector::type()(BonusType::LUCK);
	static const std::string cachingStrLuck = "type_LUCK";
	bonusList = getBonusBearer()->getBonuses(luckSelector, cachingStrLuck);

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
	const std::string cachingStr = "type_STACK_HEALTH";
	static const auto selector = Selector::type()(BonusType::STACK_HEALTH);
	auto value = getBonusBearer()->valOfBonuses(selector, cachingStr);
	return std::max(1, value); //never 0
}

ui32 ACreature::getMovementRange(int turn) const
{
	//war machines cannot move
	if(getBonusBearer()->hasBonus(Selector::type()(BonusType::SIEGE_WEAPON).And(Selector::turns(turn))))
	{
		return 0;
	}
	if(getBonusBearer()->hasBonus(Selector::type()(BonusType::BIND_EFFECT).And(Selector::turns(turn))))
	{
		return 0;
	}

	return getBonusBearer()->valOfBonuses(Selector::type()(BonusType::STACKS_SPEED).And(Selector::turns(turn)));
}

bool ACreature::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	static const std::string cachingStr = "ACreature::isLiving";
	static const CSelector selector = Selector::type()(BonusType::UNDEAD)
		.Or(Selector::type()(BonusType::NON_LIVING))
		.Or(Selector::type()(BonusType::GARGOYLE))
		.Or(Selector::type()(BonusType::SIEGE_WEAPON));

	return !getBonusBearer()->hasBonus(selector, cachingStr);
}


VCMI_LIB_NAMESPACE_END
