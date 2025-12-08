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

#include "../../battle/IBattleState.h"
#include "../../battle/CUnitState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../networkPacks/PacksForClientBattle.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void Heal::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	apply(m->getEffectValue(), server, m, target);
}

SpellEffectValue Heal::getHealthChange(const Mechanics * m, const EffectTarget & spellTarget) const
{
	SpellEffectValue result;

	for(const auto & oneTarget : spellTarget)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			result += getHealEffectValue(m->getEffectValue(), m, unit);
			result.unitType = unit->creatureId();
		}
	}
	return result;
}

void Heal::apply(int64_t value, ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	BattleLogMessage logMessage;
	logMessage.battleID = m->battle()->getBattle()->getBattleID();

	BattleUnitsChanged pack;
	pack.battleID = m->battle()->getBattle()->getBattleID();

	prepareHealEffect(value, pack, logMessage, *server->getRNG(), m, target);
	if(!pack.changedStacks.empty())
		server->apply(pack);
	if(!logMessage.lines.empty())
		server->apply(logMessage);
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
		if(hpGained < minFullUnits * unit->getMaxHealth())
			return false;
	}

	if(unit->isDead())
	{
		//check if alive unit blocks resurrection
		for(const BattleHex & hex : unit->getHexes())
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

SpellEffectValue Heal::getHealEffectValue(int64_t value, const Mechanics * m, const battle::Unit * unit, std::shared_ptr<battle::Unit> newState) const
{
	SpellEffectValue result {};

	result.hpDelta = value;

	if(!newState)
		newState = unit->acquire();
	const auto countBeforeHeal = newState->getCount();
	newState->heal(result.hpDelta, healLevel, healPower);

	result.unitsDelta = std::max(0, newState->getCount() - countBeforeHeal);

	return result;
}

void Heal::prepareHealEffect(int64_t value, BattleUnitsChanged & pack, BattleLogMessage & logMessage, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	for(const auto & oneTarget : target)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			auto state = unit->acquire();
			SpellEffectValue healValue = getHealEffectValue(value, m, unit, state);

			if(healValue.hpDelta > 0)
			{
				if(healValue.unitsDelta > 0)
				{
					// %d %s rise from the dead!
					// in the table first comes plural string, then the singular one
					MetaString resurrectText;
					state->addText(resurrectText, EMetaText::GENERAL_TXT, 116, healValue.unitsDelta == 1);
					state->addNameReplacement(resurrectText);
					resurrectText.replaceNumber(healValue.unitsDelta);
					logMessage.lines.push_back(std::move(resurrectText));
				}
				else if (healValue.hpDelta > 0 && m->caster->getHeroCaster() == nullptr) //Show text about healed HP if healed by unit
				{
					MetaString healText;
					auto casterUnitID = m->caster->getCasterUnitId();
					auto casterUnit = m->battle()->battleGetUnitByID(casterUnitID);
					healText.appendLocalString(EMetaText::GENERAL_TXT, 414);
					casterUnit->addNameReplacement(healText, false);
					state->addNameReplacement(healText, false);
					healText.replaceNumber((int)healValue.hpDelta);
					logMessage.lines.push_back(std::move(healText));
				}

				if(healValue.hpDelta > 0)
				{
					UnitChanges info(state->unitId(), UnitChanges::EOperation::RESET_STATE);
					info.healthDelta = healValue.hpDelta;
					state->save(info.data);
					pack.changedStacks.push_back(info);
				}
			}
		}
	}
}


}
}

VCMI_LIB_NAMESPACE_END
