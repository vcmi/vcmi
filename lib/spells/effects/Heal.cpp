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

void Heal::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	apply(m->getEffectValue(), server, m, target);
}

void Heal::apply(int64_t value, ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	BattleLogMessage logMessage;
	BattleUnitsChanged pack;
	prepareHealEffect(value, pack, logMessage, *server->getRNG(), m, target);
	if(!pack.changedStacks.empty())
		server->apply(&pack);
	if(!logMessage.lines.empty())
		server->apply(&logMessage);
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

void Heal::prepareHealEffect(int64_t value, BattleUnitsChanged & pack, BattleLogMessage & logMessage, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	for(const auto & oneTarget : target)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			auto unitHPgained = m->applySpellBonus(value, unit);

			auto state = unit->acquire();
			const auto countBeforeHeal = state->getCount();
			state->heal(unitHPgained, healLevel, healPower);

			if(const auto resurrectedCount = std::max(0, state->getCount() - countBeforeHeal))
			{
				// %d %s rise from the dead!
				// in the table first comes plural string, then the singular one
				MetaString resurrectText;
				state->addText(resurrectText, MetaString::GENERAL_TXT, 116, resurrectedCount == 1);
				state->addNameReplacement(resurrectText);
				resurrectText.addReplacement(resurrectedCount);
				logMessage.lines.push_back(std::move(resurrectText));
			}

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
