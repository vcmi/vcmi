/*
 * UnitEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "UnitEffect.h"

#include "../ISpellMechanics.h"

#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"
#include <vcmi/spells/Spell.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void UnitEffect::adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const
{
	for(const auto & destnation : spellTarget)
		hexes.insert(destnation.hexValue);
}

bool UnitEffect::applicableGeneral(Problem & problem, const Mechanics * m) const
{
	auto mainFilter = [this, m](const battle::Unit * unit){ return applicableUnit(m, unit, false, false);};

	//stack effect is applicable in general if there is at least one smart target
	auto targets = m->battle()->battleGetUnitsIf(mainFilter);

	if(targets.empty())
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return true;
}

bool UnitEffect::applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const
{
	//stack effect is applicable if it affects at least one target (smartness should not be checked)
	//assume target correctly transformed, just reapply filter

	for(const auto & item : target)
		if(item.unitValue)
			if(applicableUnit(m, item.unitValue, false, true))
				return true;

	return false;
}

bool UnitEffect::applicableUnit(const Mechanics * m, const battle::Unit * unit, bool alwaysSmart, bool alwaysReceptive) const
{
	if (!isValidTarget(m, unit))
		return false;

	if (!alwaysReceptive && !isReceptive(m, unit))
		return false;

	const bool smart = m->isSmart() || alwaysSmart;
	const bool ignoreOwner = !smart;
	return ignoreOwner || m->ownerMatches(unit);
}

Target UnitEffect::filterTarget(const Mechanics * m, const Target & target) const
{
	Target res;
	vstd::copy_if(target, std::back_inserter(res), [this, m](const Destination & d)
	{
		return d.unitValue && isValidTarget(m, d.unitValue) && isReceptive(m, d.unitValue);
	});
	return res;
}

Target UnitEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	if(chainLength > 1)
		return transformTargetByChain(m, aimPoint, spellTarget);
	else
		return transformTargetByRange(m, aimPoint, spellTarget);
}

Target UnitEffect::transformTargetByRange(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	auto mainFilter = [this, m](const battle::Unit * unit){ return applicableUnit(m, unit, false, false);};

	Target spellTargetCopy(spellTarget);

	// make sure that we have valid target with valid aim, even if spell have invalid range configured
	// TODO: check than spell range is actually not valid
	// also hackfix for banned creature massive spells
	// FIXME: potentially breaking change: aimPoint may NOT be in Target - example: frost ring
	if(!aimPoint.empty() && spellTarget.empty())
		spellTargetCopy.insert(spellTargetCopy.begin(), Destination(aimPoint.front()));

	std::set<const battle::Unit *> targets;

	if(m->isMassive())
	{
		//ignore spellTarget and add all stacks
		auto units = m->battle()->battleGetUnitsIf(mainFilter);
		for(const auto *unit : units)
			targets.insert(unit);
	}
	else
	{
		//process each tile
		for(const Destination & dest : spellTargetCopy)
		{
			if(dest.unitValue)
			{
				if(mainFilter(dest.unitValue))
					targets.insert(dest.unitValue);
			}
			else if(dest.hexValue.isValid())
			{
				//select one unit on tile, prefer alive one
				const battle::Unit * targetOnTile = nullptr;

				auto predicate = [&](const battle::Unit * unit)
				{
					return unit->coversPos(dest.hexValue) && mainFilter(unit);
				};

				auto units = m->battle()->battleGetUnitsIf(predicate);

				for(const auto *unit : units)
				{
					if(unit->alive())
					{
						targetOnTile = unit;
						break;
					}
				}

				if(targetOnTile == nullptr && !units.empty())
					targetOnTile = units.front();

				if(targetOnTile)
					targets.insert(targetOnTile);
			}
			else
			{
				logGlobal->debug("Invalid destination in spell Target");
			}
		}
	}

	if(m->alwaysHitFirstTarget())
	{
		//TODO: examine if adjustments needed related to INVINCIBLE bonus
		if(!aimPoint.empty() && aimPoint.front().unitValue)
			targets.insert(aimPoint.front().unitValue);
	}

	Target effectTarget;

	for(const auto *s : targets)
		effectTarget.push_back(Destination(s));

	return effectTarget;
}

Target UnitEffect::transformTargetByChain(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	Target byRange = transformTargetByRange(m, aimPoint, spellTarget);

	if(byRange.empty())
	{
		return Target();
	}

	const Destination & mainDestination = byRange.front();

	if(!mainDestination.hexValue.isValid())
	{
		return Target();
	}

	BattleHexArray possibleHexes;

	auto possibleTargets = m->battle()->battleGetUnitsIf([&](const battle::Unit * unit) -> bool
	{
		return isReceptive(m, unit) && isValidTarget(m, unit);
	});

	for(const auto *unit : possibleTargets)
	{
		for(const auto & hex : unit->getHexes())
			possibleHexes.insert(hex);
	}

	BattleHex destHex = mainDestination.hexValue;
	Target effectTarget;

	for(int32_t targetIndex = 0; targetIndex < chainLength; ++targetIndex)
	{
		const auto *unit = m->battle()->battleGetUnitByPos(destHex, true);

		if(!unit)
			break;

		bool wouldResist = m->wouldResist(unit);
		if(m->alwaysHitFirstTarget() && targetIndex == 0)
			effectTarget.emplace_back(unit);
		if(wouldResist && targetIndex == 0)
		{
			// if first target resists, chain ends here, resistance animation played
			effectTarget.emplace_back(unit);
			break;
		}
		else if(isReceptive(m, unit) && isValidTarget(m, unit) && !wouldResist)
			effectTarget.emplace_back(unit);
		else if(isReceptive(m, unit) && isValidTarget(m, unit) && wouldResist)
		{
			// target is skipped, no magic resistance animation (Heroes 3 logic)
			targetIndex--;
		}
		else
			effectTarget.emplace_back();

		for(const auto & hex : unit->getHexes())
			if (possibleHexes.contains(hex))
				possibleHexes.erase(hex);

		if(possibleHexes.empty())
			break;

		destHex = BattleHex::getClosestTile(unit->unitSide(), destHex, possibleHexes);
	}

	return effectTarget;
}

bool UnitEffect::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	// TODO: override in rising effect
	// TODO: check absolute immunity here

	return unit->isValidTarget(false);
}

bool UnitEffect::isReceptive(const Mechanics * m, const battle::Unit * unit) const
{
	if (unit->isInvincible() && m->isNegativeSpell())
		return false;
	if(ignoreImmunity)
	{
		//ignore all immunities, except specific absolute immunity(VCMI addition)
		return !unit->hasAbsoluteImmunity(m->getSpellId());
	}
	else
	{
		return m->isReceptive(unit);
	}
}

void UnitEffect::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("ignoreImmunity", ignoreImmunity);
	handler.serializeInt("chainLength", chainLength, 0);
	handler.serializeFloat("chainFactor", chainFactor, 0);
	serializeJsonUnitEffect(handler);
}


}
}

VCMI_LIB_NAMESPACE_END
