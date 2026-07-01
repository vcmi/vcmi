/*
 * SpellTargetsEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/CRandomGenerator.h"
#include "SpellTargetsEvaluator.h"
#include <vcmi/spells/Spell.h>

using namespace spells;

std::vector<Target> SpellTargetEvaluator::getViableTargets(const Mechanics * spellMechanics)
{
	std::vector<Target> result;
	std::vector<AimType> targetTypes = spellMechanics->getTargetTypes();
	if(targetTypes.size() != 1) //TODO: support for multi-destination spells
		return result;

	auto targetType = targetTypes.front();

	switch(targetType)
	{
		case AimType::CREATURE://TODO: support for multi-destination spells
			return allTargetableCreatures(spellMechanics);
		case AimType::LOCATION:
		{
			if(spellMechanics->isNeutralSpell())
				return defaultLocationSpellHeuristics(
					spellMechanics
				); // theoretically anything can be a useful destination, so we balance performance and validity
			else
				return theBestLocationCasts(spellMechanics);
		}
		case AimType::NOTHING:
			return std::vector<Target>(1); //default-constructed target means cast without destination
		default:
			return result;
	}
}

std::vector<Target> SpellTargetEvaluator::defaultLocationSpellHeuristics(const spells::Mechanics * spellMechanics)
{
	std::vector<Target> result = allTargetableCreatures(spellMechanics);
	auto units = spellMechanics->battle()->battleGetAllUnits(false);
	for(const auto * unit : units) //insert a random surrounding hex
	{
		auto surroundingHexes = unit->getSurroundingHexes();
		if(!surroundingHexes.empty())
		{
			auto randomSurroundingHex = *RandomGeneratorUtil::nextItem(surroundingHexes, CRandomGenerator::getDefault()); // don't think this method bias matter with such small numbers
			addIfCanBeCast(spellMechanics, randomSurroundingHex, result);
		}
	}
	return result;
}

std::vector<Target> SpellTargetEvaluator::allTargetableCreatures(const spells::Mechanics * spellMechanics)
{
	std::vector<Target> result;
	auto units = spellMechanics->battle()->battleGetAllUnits(false);
	for(const auto * unit : units)
		addIfCanBeCast(spellMechanics, unit->getPosition(), result);
	return result;
}

std::vector<Target> SpellTargetEvaluator::theBestLocationCasts(const spells::Mechanics * spellMechanics)
{
	std::vector<Target> result;
	std::map<BattleHex, std::set<const CStack *>> allCasts;
	std::map<BattleHex, std::set<const CStack *>> bestCasts;
	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		BattleHex dest(i);
		if(canBeCastAt(spellMechanics, dest))
		{
			Target target;
			target.emplace_back(dest);
			auto temp = spellMechanics->getAffectedStacks(target);
			std::set<const CStack *> affectedStacks(temp.begin(), temp.end());
			allCasts[dest] = affectedStacks;
		}
	}

	for(const auto & cast : allCasts)
	{
		std::set<BattleHex> worseCasts;
		if(isCastHarmful(spellMechanics, cast.second))
			continue;

		bool isBestCast = true;
		for(const auto & bestCast : bestCasts)
		{
			Compare compare = compareAffectedStacks(spellMechanics, cast.second, bestCast.second);

			if(compare == Compare::WORSE || compare == Compare::EQUAL)
			{
				isBestCast = false;
				break;
			}

			if(compare == Compare::BETTER)
			{
				worseCasts.insert(bestCast.first);
			}
		}

		if(isBestCast)
		{
			bestCasts.insert(cast);
			for(BattleHex worseCast : worseCasts)
				bestCasts.erase(worseCast);
		}
	}

	for(const auto & cast : bestCasts)
	{
		Destination des(cast.first);
		result.push_back({des});
	}
	return result;
}

bool SpellTargetEvaluator::isCastHarmful(const spells::Mechanics * spellMechanics, const std::set<const CStack *> & affectedStacks)
{

	bool isAffectedAlly = false;
	bool isAffectedEnemy = false;

	for(const CStack * affectedUnit : affectedStacks)
	{
		if(affectedUnit->unitSide() == spellMechanics->casterSide)
			isAffectedAlly = true;
		else
			isAffectedEnemy = true;
	}

	return (spellMechanics->isPositiveSpell() && !isAffectedAlly) || (spellMechanics->isNegativeSpell() && !isAffectedEnemy);
}

SpellTargetEvaluator::Compare SpellTargetEvaluator::compareAffectedStacks(
	const spells::Mechanics * spellMechanics, const std::set<const CStack *> & newCast, const std::set<const CStack *> & oldCast)
{
	if(newCast.size() == oldCast.size())
		return newCast == oldCast ? Compare::EQUAL : Compare::DIFFERENT;

	auto getAlliedUnits = [&spellMechanics](const std::set<const CStack *> & allUnits) -> std::set<const CStack *>
	{
		std::set<const CStack *> alliedUnits;
		for(auto stack : allUnits)
		{
			if(stack->unitSide() == spellMechanics->casterSide)
				alliedUnits.insert(stack);
		}
		return alliedUnits;
	};

	auto getEnemyUnits = [&spellMechanics](const std::set<const CStack *> & allUnits) -> std::set<const CStack *>
	{
		std::set<const CStack *> enemyUnits;
		for(auto stack : allUnits)
		{
			if(stack->unitSide() != spellMechanics->casterSide)
				enemyUnits.insert(stack);
		}
		return enemyUnits;
	};

	Compare alliedSubsetComparison = compareAffectedStacksSubset(spellMechanics, getAlliedUnits(newCast), getAlliedUnits(oldCast));
	Compare enemySubsetComparison = compareAffectedStacksSubset(spellMechanics, getEnemyUnits(newCast), getEnemyUnits(oldCast));

	if(spellMechanics->isPositiveSpell())
		enemySubsetComparison = reverse(enemySubsetComparison);
	else if(spellMechanics->isNegativeSpell())
		alliedSubsetComparison = reverse(alliedSubsetComparison);

	std::set<Compare> comparisonResults = {alliedSubsetComparison, enemySubsetComparison};
	std::set<std::set<Compare>> possibleBetterResults = {
		{Compare::BETTER, Compare::BETTER},
        {Compare::BETTER, Compare::EQUAL }
	};
	std::set<std::set<Compare>> possibleWorstResults = {
		{Compare::WORSE, Compare::WORSE},
        {Compare::WORSE, Compare::EQUAL}
	};

	if(possibleBetterResults.find(comparisonResults) != possibleBetterResults.end())
		return Compare::BETTER;
	if(possibleWorstResults.find(comparisonResults) != possibleWorstResults.end())
		return Compare::WORSE;

	return Compare::DIFFERENT;
}

SpellTargetEvaluator::Compare SpellTargetEvaluator::compareAffectedStacksSubset(
    const spells::Mechanics * spellMechanics, const std::set<const CStack *> & newSubset, const std::set<const CStack *> & oldSubset)
{
	if(newSubset.size() == oldSubset.size())
		return newSubset == oldSubset ? Compare::EQUAL : Compare::DIFFERENT;

	if(oldSubset.size() > newSubset.size())
		return reverse(compareAffectedStacksSubset(spellMechanics, oldSubset, newSubset));

	const std::set<const CStack *> & biggerSet = newSubset;
	const std::set<const CStack *> & smallerSet = oldSubset;

	if(std::includes(biggerSet.begin(), biggerSet.end(), smallerSet.begin(), smallerSet.end()))
		return Compare::BETTER;
	else
		return Compare::DIFFERENT;
}

SpellTargetEvaluator::Compare SpellTargetEvaluator::reverse(SpellTargetEvaluator::Compare compare)
{
	switch(compare)
	{
		case Compare::BETTER:
			return Compare::WORSE;
		case Compare::WORSE:
			return Compare::BETTER;
		default:
			return compare;
	}
}

bool SpellTargetEvaluator::canBeCastAt(const spells::Mechanics * spellMechanics, BattleHex hex)
{
	detail::ProblemImpl ignored;
	Destination des(hex);
	return spellMechanics->canBeCastAt({des}, ignored);
}

void SpellTargetEvaluator::addIfCanBeCast(const spells::Mechanics * spellMechanics, BattleHex hex, std::vector<Target> & targets)
{
	detail::ProblemImpl ignored;
	Destination des(hex);
	if(spellMechanics->canBeCastAt({des}, ignored))
		targets.push_back({des});
}
