/*
 * Sacrifice.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Sacrifice.h"
#include "../ISpellMechanics.h"

#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"
#include "../../networkPacks/PacksForClientBattle.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void Sacrifice::adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const
{
	if(!types.empty())
	{
		if(types[0] != AimType::CREATURE)
		{
			types.clear();
			return;
		}

		if(types.size() == 1)
		{
			types.push_back(AimType::CREATURE);
		}
		else if(types.size() > 1)
		{
			if(types[1] != AimType::CREATURE)
				types.clear();
		}
	}
}

bool Sacrifice::applicableGeneral(Problem & problem, const Mechanics * m) const
{
	auto mainFilter = [this, m](const battle::Unit * unit){ return applicableUnit(m, unit, true, false);};
	auto targets = m->battle()->battleGetUnitsIf(mainFilter);

	bool targetExists = false;
	bool targetToSacrificeExists = false;

	for(const battle::Unit * target : targets)
	{
		auto unit = target->acquire();
		if(target->alive() && unit->isLiving() && !unit->hasBonusOfType(BonusType::MECHANICAL))
			targetToSacrificeExists = true;
		else if(target->isDead())
			targetExists = true;

		if(targetExists && targetToSacrificeExists)
			break;
	}

	if(!(targetExists && targetToSacrificeExists))
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return true;
}

bool Sacrifice::applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const
{
	//TODO: support for multiple targets?

	if(target.empty())
		return false;

	Target healTarget;
	healTarget.emplace_back(target.front());

	if(!Heal::applicableTarget(problem, m, healTarget))
		return false;

	if(healTarget.front().unitValue->alive())
		return false;

	if(target.size() == 2)
	{
		const auto *victim = target.at(1).unitValue;
		if(!victim)
			return false;

		return victim->alive() && applicableUnit(m, victim, false, false);
	}

	return true;
}

void Sacrifice::apply(ServerCallback * server, const Mechanics * m, const Target & target) const
{
	if(target.size() != 2)
	{
		logGlobal->error("Sacrifice effect requires 2 targets");
		return;
	}

	const battle::Unit * victim = target.back().unitValue;

	if(!victim)
	{
		logGlobal->error("No unit to Sacrifice");
		return;
	}

	Target healTarget;
	healTarget.emplace_back(target.front());

	Heal::apply(calculateHealEffectValue(m, victim), server, m, healTarget);

	BattleUnitsChanged removeUnits;
	removeUnits.battleID = m->battle()->getBattle()->getBattleID();
	removeUnits.changedStacks.emplace_back(victim->unitId(), UnitChanges::EOperation::REMOVE);
	server->apply(removeUnits);
}

bool Sacrifice::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	return unit->isValidTarget(true);
}

Target Sacrifice::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	Target res = Heal::transformTarget(m, aimPoint, spellTarget);

	//ignore spell range for now, arbitrary range support requires redesign
	res.resize(1);

	//add victim
	if(aimPoint.size() >= 2)
	{
		const auto *victim = aimPoint.at(1).unitValue;
		if(victim && applicableUnit(m, victim, false, false))
			res.emplace_back(victim);
	}

	return res;
}

SpellEffectValue Sacrifice::getHealthChange(const Mechanics * m, const Target & spellTarget) const
{
	SpellEffectValue result{};

	if(spellTarget.empty())
		return result;

	const battle::Unit * target = spellTarget.front().unitValue;

	result.unitType = target->creatureId();

	if(!target->alive())
	{
		result.hpDelta = target->unitBaseAmount() * target->getMaxHealth();
		result.unitsDelta = target->unitBaseAmount();
		return result;
	}

	result.hpDelta = calculateHealEffectValue(m, target);
	result.unitsDelta = -target->getCount();

	return result;
}

int64_t Sacrifice::calculateHealEffectValue(const Mechanics * m, const battle::Unit * victim) 
{
	return (m->getEffectPower() + victim->getMaxHealth() + m->calculateRawEffectValue(0, 1)) * victim->getCount();
}


}
}

VCMI_LIB_NAMESPACE_END
