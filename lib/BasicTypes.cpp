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
#include "HeroBonus.h"

#include <vcmi/Entity.h>
#include <vcmi/Faction.h>
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
VCMI_LIB_NAMESPACE_END