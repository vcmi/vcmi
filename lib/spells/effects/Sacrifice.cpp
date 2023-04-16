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
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void Sacrifice::adjustTargetTypes(std::vector<TargetType> & types) const
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

bool Sacrifice::applicable(Problem & problem, const Mechanics * m) const
{
	auto mainFilter = std::bind(&UnitEffect::getStackFilter, this, m, true, _1);
	auto predicate = std::bind(&UnitEffect::eraseByImmunityFilter, this, m, _1);

	auto targets = m->battle()->battleGetUnitsIf(mainFilter);
	vstd::erase_if(targets, predicate);

	bool targetExists = false;
	bool targetToSacrificeExists = false;

	for(auto & target : targets)
	{
		if(target->alive())
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

bool Sacrifice::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	//TODO: support for multiple targets?

	if(target.empty())
		return false;

	EffectTarget healTarget;
	healTarget.emplace_back(target.front());

	if(!Heal::applicable(problem, m, healTarget))
		return false;

	if(target.size() == 2)
	{
		const auto *victim = target.at(1).unitValue;
		if(!victim)
			return false;
		
		return victim->alive() && getStackFilter(m, false, victim) && isReceptive(m, victim);
	}

	return true;
}

void Sacrifice::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
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

	EffectTarget healTarget;
	healTarget.emplace_back(target.front());

	Heal::apply(calculateHealEffectValue(m, victim), server, m, healTarget);

	BattleUnitsChanged removeUnits;
	removeUnits.changedStacks.emplace_back(victim->unitId(), UnitChanges::EOperation::REMOVE);
	server->apply(&removeUnits);
}

bool Sacrifice::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	return unit->isValidTarget(true);
}

EffectTarget Sacrifice::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	EffectTarget res = Heal::transformTarget(m, aimPoint, spellTarget);

	//ignore spell range for now, arbitrary range support requires redesign
	res.resize(1);

	//add victim
	if(aimPoint.size() >= 2)
	{
		const auto *victim = aimPoint.at(1).unitValue;
		if(victim && getStackFilter(m, false, victim) && isReceptive(m, victim))
			res.emplace_back(victim);
	}

	return res;
}

int64_t Sacrifice::calculateHealEffectValue(const Mechanics * m, const battle::Unit * victim) 
{
	return (m->getEffectPower() + victim->MaxHealth() + m->calculateRawEffectValue(0, 1)) * victim->getCount();
}


}
}

VCMI_LIB_NAMESPACE_END
