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
#include "bonuses/HeroBonus.h"

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

TerrainId IFactionMember::getNativeTerrain() const
{
	constexpr auto any = TerrainId(ETerrainId::ANY_TERRAIN);
	const std::string cachingStringNoTerrainPenalty = "type_NO_TERRAIN_PENALTY_sANY";
	static const auto selectorNoTerrainPenalty = Selector::typeSubtype(Bonus::NO_TERRAIN_PENALTY, any);

	//this code is used in the CreatureTerrainLimiter::limit to setup battle bonuses
	//and in the CGHeroInstance::getNativeTerrain() to setup movement bonuses or/and penalties.
	return getBonusBearer()->hasBonus(selectorNoTerrainPenalty, cachingStringNoTerrainPenalty)
		? any : VLC->factions()->getById(getFaction())->getNativeTerrain();
}

int32_t IFactionMember::magicResistance() const
{
	si32 val = getBonusBearer()->valOfBonuses(Selector::type()(Bonus::MAGIC_RESISTANCE));
	vstd::amin (val, 100);
	return val;
}

int IFactionMember::getAttack(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_ATTACK";

	static const auto selector = Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);

	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int IFactionMember::getDefense(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_DEFENSE";

	static const auto selector = Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int IFactionMember::getMinDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_1";
	static const auto selector = Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1));
	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int IFactionMember::getMaxDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_2";
	static const auto selector = Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2));
	return getBonusBearer()->valOfBonuses(selector, cachingStr);
}

int IFactionMember::getPrimSkillLevel(PrimarySkill::PrimarySkill id) const
{
	static const CSelector selectorAllSkills = Selector::type()(Bonus::PRIMARY_SKILL);
	static const std::string keyAllSkills = "type_PRIMARY_SKILL";
	auto allSkills = getBonusBearer()->getBonuses(selectorAllSkills, keyAllSkills);
	auto ret = allSkills->valOfBonuses(Selector::subtype()(id));
	auto minSkillValue = (id == PrimarySkill::SPELL_POWER || id == PrimarySkill::KNOWLEDGE) ? 1 : 0;
	return std::max(ret, minSkillValue); //otherwise, some artifacts may cause negative skill value effect, sp=0 works in old saves
}

int IFactionMember::MoraleValAndBonusList(TConstBonusListPtr & bonusList) const
{
	static const auto unaffectedByMoraleSelector = Selector::type()(Bonus::NON_LIVING).Or(Selector::type()(Bonus::UNDEAD))
													.Or(Selector::type()(Bonus::SIEGE_WEAPON)).Or(Selector::type()(Bonus::NO_MORALE));

	static const std::string cachingStrUn = "IFactionMember::unaffectedByMoraleSelector";
	auto unaffected = getBonusBearer()->hasBonus(unaffectedByMoraleSelector, cachingStrUn);
	if(unaffected)
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return 0;
	}

	static const auto moraleSelector = Selector::type()(Bonus::MORALE);
	static const std::string cachingStrMor = "type_MORALE";
	bonusList = getBonusBearer()->getBonuses(moraleSelector, cachingStrMor);

	return std::clamp(bonusList->totalValue(), -3, +3);
}

int IFactionMember::LuckValAndBonusList(TConstBonusListPtr & bonusList) const
{
	if(getBonusBearer()->hasBonusOfType(Bonus::NO_LUCK))
	{
		if(bonusList && !bonusList->empty())
			bonusList = std::make_shared<const BonusList>();
		return 0;
	}

	static const auto luckSelector = Selector::type()(Bonus::LUCK);
	static const std::string cachingStrLuck = "type_LUCK";
	bonusList = getBonusBearer()->getBonuses(luckSelector, cachingStrLuck);

	return std::clamp(bonusList->totalValue(), -3, +3);
}

int IFactionMember::MoraleVal() const
{
	TConstBonusListPtr tmp = nullptr;
	return MoraleValAndBonusList(tmp);
}

int IFactionMember::LuckVal() const
{
	TConstBonusListPtr tmp = nullptr;
	return LuckValAndBonusList(tmp);
}

ui32 ICreature::MaxHealth() const
{
	const std::string cachingStr = "type_STACK_HEALTH";
	static const auto selector = Selector::type()(Bonus::STACK_HEALTH);
	auto value = getBonusBearer()->valOfBonuses(selector, cachingStr);
	return std::max(1, value); //never 0
}

ui32 ICreature::Speed(int turn, bool useBind) const
{
	//war machines cannot move
	if(getBonusBearer()->hasBonus(Selector::type()(Bonus::SIEGE_WEAPON).And(Selector::turns(turn))))
	{
		return 0;
	}
	//bind effect check - doesn't influence stack initiative
	if(useBind && getBonusBearer()->hasBonus(Selector::type()(Bonus::BIND_EFFECT).And(Selector::turns(turn))))
	{
		return 0;
	}

	return getBonusBearer()->valOfBonuses(Selector::type()(Bonus::STACKS_SPEED).And(Selector::turns(turn)));
}

bool ICreature::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	static const std::string cachingStr = "ICreature::isLiving";
	static const CSelector selector = Selector::type()(Bonus::UNDEAD)
		.Or(Selector::type()(Bonus::NON_LIVING))
		.Or(Selector::type()(Bonus::GARGOYLE))
		.Or(Selector::type()(Bonus::SIEGE_WEAPON));

	return !getBonusBearer()->hasBonus(selector, cachingStr);
}


VCMI_LIB_NAMESPACE_END