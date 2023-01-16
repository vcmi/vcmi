/*
 * Heal.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Heal.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN


static const std::string EFFECT_NAME = "core:heal";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Heal, EFFECT_NAME);

Heal::Heal()
	: UnitEffect(),
	healLevel(EHealLevel::HEAL),
	healPower(EHealPower::PERMANENT),
	minFullUnits(0)
{

}

Heal::~Heal() = default;

void Heal::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	apply(m->getEffectValue(), server, m, target);
}

void Heal::apply(int64_t value, ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	BattleUnitsChanged pack;
	prepareHealEffect(value, pack, *server->getRNG(), m, target);
	if(!pack.changedStacks.empty())
		server->apply(&pack);
}

bool Heal::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	const bool onlyAlive = healLevel == EHealLevel::HEAL;
	const bool validInGenaral = unit->isValidTarget(!onlyAlive);

	if(!validInGenaral)
		return false;

	auto injuries = unit->getTotalHealth() - unit->getAvailableHealth();

	if(injuries == 0)
		return false;

	if(minFullUnits > 0)
	{
		auto hpGained = std::min(m->getEffectValue(), injuries);
		if(hpGained < minFullUnits * unit->MaxHealth())
			return false;
	}

	if(unit->isDead())
	{
		//check if alive unit blocks resurrection
		for(const BattleHex & hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
		{
			auto blocking = m->battle()->battleGetUnitsIf([hex, unit](const battle::Unit * other)
			{
				return other->isValidTarget(false) && other->coversPos(hex) && other != unit;
			});

			if(!blocking.empty())
				return false;
		}
	}
	return true;
}

void Heal::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	static const std::vector<std::string> HEAL_LEVEL_MAP =
	{
		"heal",
		"resurrect",
		"overHeal"
	};

	static const std::vector<std::string> HEAL_POWER_MAP =
	{
		"oneBattle",
		"permanent"
	};

	handler.serializeEnum("healLevel", healLevel, EHealLevel::HEAL, HEAL_LEVEL_MAP);
	handler.serializeEnum("healPower", healPower, EHealPower::PERMANENT, HEAL_POWER_MAP);
	handler.serializeInt("minFullUnits", minFullUnits);
}

void Heal::prepareHealEffect(int64_t value, BattleUnitsChanged & pack, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	for(auto & oneTarget : target)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			auto unitHPgained = m->applySpellBonus(value, unit);

			auto state = unit->acquire();
			state->heal(unitHPgained, healLevel, healPower);

			if(unitHPgained > 0)
			{
				UnitChanges info(state->unitId(), UnitChanges::EOperation::RESET_STATE);
				info.healthDelta = unitHPgained;
				state->save(info.data);
				pack.changedStacks.push_back(info);
			}
		}
	}
}


}
}

VCMI_LIB_NAMESPACE_END
