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

#include "../../NetPacksBase.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void UnitEffect::adjustTargetTypes(std::vector<TargetType> & types) const
{

}

void UnitEffect::adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const
{
	for(const auto & destnation : spellTarget)
		hexes.insert(destnation.hexValue);
}

bool UnitEffect::applicable(Problem & problem, const Mechanics * m) const
{
	//stack effect is applicable in general if there is at least one smart target

	auto mainFilter = std::bind(&UnitEffect::getStackFilter, this, m, false, _1);
	auto predicate = std::bind(&UnitEffect::eraseByImmunityFilter, this, m, _1);

	auto targets = m->battle()->battleGetUnitsIf(mainFilter);
	vstd::erase_if(targets, predicate);
	if(targets.empty())
	{
		MetaString text;
		text.addTxt(MetaString::GENERAL_TXT, 185);
		problem.add(std::move(text), Problem::NORMAL);
		return false;
	}


	return true;
}

bool UnitEffect::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	//stack effect is applicable if it affects at least one target (smartness should not be checked)
	//assume target correctly transformed, just reapply filter

	for(const auto & item : target)
		if(item.unitValue)
			if(getStackFilter(m, false, item.unitValue))
				return true;

	return false;
}

bool UnitEffect::getStackFilter(const Mechanics * m, bool alwaysSmart, const battle::Unit * s) const
{
	return isValidTarget(m, s) && isSmartTarget(m, s, alwaysSmart);
}

bool UnitEffect::eraseByImmunityFilter(const Mechanics * m, const battle::Unit * s) const
{
	return !isReceptive(m, s);
}

EffectTarget UnitEffect::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	EffectTarget res;
	vstd::copy_if(target, std::back_inserter(res), [this, m](const Destination & d)
	{
		return d.unitValue && isValidTarget(m, d.unitValue) && isReceptive(m, d.unitValue);
	});
	return res;
}

EffectTarget UnitEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	if(chainLength > 1)
		return transformTargetByChain(m, aimPoint, spellTarget);
	else
		return transformTargetByRange(m, aimPoint, spellTarget);
}

EffectTarget UnitEffect::transformTargetByRange(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	auto mainFilter = std::bind(&UnitEffect::getStackFilter, this, m, false, _1);

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

	auto predicate = std::bind(&UnitEffect::eraseByImmunityFilter, this, m, _1);

	vstd::erase_if(targets, predicate);

	if(m->alwaysHitFirstTarget())
	{
		if(!aimPoint.empty() && aimPoint.front().unitValue)
			targets.insert(aimPoint.front().unitValue);
		else
			logGlobal->error("Spell-like attack with no primary target.");
	}

	EffectTarget effectTarget;

	for(const auto *s : targets)
		effectTarget.push_back(Destination(s));

	return effectTarget;
}

EffectTarget UnitEffect::transformTargetByChain(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	EffectTarget byRange = transformTargetByRange(m, aimPoint, spellTarget);

	if(byRange.empty())
	{
		return EffectTarget();
	}

	const Destination & mainDestination = byRange.front();

	if(!mainDestination.hexValue.isValid())
	{
		return EffectTarget();
	}

	std::set<BattleHex> possibleHexes;

	auto possibleTargets = m->battle()->battleGetUnitsIf([&](const battle::Unit * unit) -> bool
	{
		return isValidTarget(m, unit);
	});

	for(const auto *unit : possibleTargets)
	{
		for(auto hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
			possibleHexes.insert(hex);
	}

	BattleHex destHex = mainDestination.hexValue;
	EffectTarget effectTarget;

	for(int32_t targetIndex = 0; targetIndex < chainLength; ++targetIndex)
	{
		const auto *unit = m->battle()->battleGetUnitByPos(destHex, true);

		if(!unit)
			break;
		if(m->alwaysHitFirstTarget() && targetIndex == 0)
			effectTarget.emplace_back(unit);
		else if(isReceptive(m, unit) && isValidTarget(m, unit))
			effectTarget.emplace_back(unit);
		else
			effectTarget.emplace_back();

		for(auto hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
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
	if(ignoreImmunity)
	{
		//ignore all immunities, except specific absolute immunity(VCMI addition)

		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << m->getSpellIndex() << "addInfo_1";
		return !unit->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, m->getSpellIndex(), 1), cachingStr.str());
	}
	else
	{
		return m->isReceptive(unit);
	}
}

bool UnitEffect::isSmartTarget(const Mechanics * m, const battle::Unit * unit, bool alwaysSmart) const
{
	const bool smart = m->isSmart() || alwaysSmart;
	const bool ignoreOwner = !smart;
	return ignoreOwner || m->ownerMatches(unit);
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
