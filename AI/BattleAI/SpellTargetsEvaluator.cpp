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
#include "SpellTargetsEvaluator.h"
#include <vcmi/spells/Spell.h>
#include "../../lib/spells/Problem.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/CStack.h"

using namespace spells;

std::vector<Target> SpellTargetEvaluator::getViableTargets(Mechanics * spellMechanics)
{
    std::vector<Target> result;
    if (!spellMechanics)
    {
        logGlobal->error("SpellTargetEvaluator received empty spellMechanics. Should not occur!");
        return result;
    }
    std::vector<AimType> targetTypes = spellMechanics->getTargetTypes();
    if (targetTypes.size() != 1)	//multi-destination spells like teleport are not implemented - they would be too resource-intensive
        return result;

    auto targetType = targetTypes.front();

    switch(targetType)
    {
    case AimType::CREATURE:
        return allSusceptibleCreatures(spellMechanics);
    case AimType::LOCATION:
    {
        if (spellMechanics->isNeutralSpell())
            return defaultHeuristics(spellMechanics);   // theoretically anything can be a useful destination, so we balance performance and validity
        else
            return theBestLocationCasts(spellMechanics);
    }
    case AimType::NO_TARGET:
        return std::vector<Target>(1); //default-constructed target means cast without destination
    default:
        return result;
    }
}

std::vector<Target> SpellTargetEvaluator::defaultHeuristics(spells::Mechanics * spellMechanics)
{
        std::vector<Target> result = allSusceptibleCreatures(spellMechanics);
        auto stacks = spellMechanics->battle()->battleGetAllStacks(true);
        for(const auto * stack : stacks) //insert a random surrounding hex
        {
            auto surroundingHexes = stack->getSurroundingHexes();
            auto randomSurroundingHex = surroundingHexes.at(rand() % surroundingHexes.size()); // don't think this method bias matter with such small numbers
            addIfCanBeCast(spellMechanics, randomSurroundingHex, result);
        }
        return result;
}

std::vector<Target> SpellTargetEvaluator::allSusceptibleCreatures(spells::Mechanics * spellMechanics) {
        std::vector<Target> result;
        auto stacks = spellMechanics->battle()->battleGetAllStacks(true);
        for(const auto * stack : stacks)
            addIfCanBeCast(spellMechanics, stack->getPosition(), result);
        return result;
}

std::vector<Target> SpellTargetEvaluator::theBestLocationCasts(spells::Mechanics * spellMechanics)
{
    std::vector<Target> result;
    std::map<BattleHex,std::set<const CStack *>> allCasts;
    std::map<BattleHex,std::set<const CStack *>> bestCasts;
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

    for (auto const& cast : allCasts)
    {
        std::set<BattleHex> worseCasts;
        if(isCastHarmful(spellMechanics, cast.second))
            continue;

        bool isBestCast = true;
        for(auto const& bestCast : bestCasts)
        {
            Compare compare = compareAffectedStacks(spellMechanics, cast.second, bestCast.second);

            if (compare == Compare::WORSE || compare == Compare::EQUAL)
            {
                isBestCast = false;
                break;
            }

            if (compare == Compare::BETTER)
            {
                worseCasts.insert(bestCast.first);
            }
        }

        if (isBestCast)
        {
            bestCasts.insert(cast);
            for (BattleHex worseCast : worseCasts)
                bestCasts.erase(worseCast);
        }
    }

    for (auto const& cast : bestCasts)
    {
        Destination des(cast.first);
        result.push_back({des});
    }
    return result;
}

bool SpellTargetEvaluator::isCastHarmful(spells::Mechanics * spellMechanics, std::set<const CStack *> affectedStacks)
{

    bool isAffectedAlly = false;
    bool isAffectedEnemy = false;

    for (const  CStack * affectedUnit : affectedStacks)
    {
            if(affectedUnit->unitSide() == spellMechanics->casterSide)
                    isAffectedAlly = true;
            else
                    isAffectedEnemy = true;
    }

    return (spellMechanics->isPositiveSpell() && !isAffectedAlly) || (spellMechanics->isNegativeSpell() && !isAffectedEnemy);
}

SpellTargetEvaluator::Compare SpellTargetEvaluator::compareAffectedStacks(spells::Mechanics * spellMechanics, std::set<const CStack *> newCast, std::set<const CStack *> oldCast)
{
        if (newCast.size() == oldCast.size())
                return newCast == oldCast ? Compare::EQUAL : Compare::DIFFERENT;

        auto getAlliedUnits = [&spellMechanics] (std::set<const CStack *> allUnits) -> std::set<const CStack *>  {
                std::set<const CStack *> alliedUnits;
                for (auto stack : allUnits) {
                        if (stack->unitSide() == spellMechanics->casterSide)
                                alliedUnits.insert(stack);
                }
                return alliedUnits;
        };

        auto getEnemyUnits = [&spellMechanics] (std::set<const CStack *> allUnits) -> std::set<const CStack *>  {
                std::set<const CStack *> enemyUnits;
                for (auto stack : allUnits) {
                        if (stack->unitSide() != spellMechanics->casterSide)
                                enemyUnits.insert(stack);
                }
                return enemyUnits;
        };


        Compare alliedSubsetComparison = compareAffectedStacksSubset(spellMechanics, getAlliedUnits(newCast), getAlliedUnits(oldCast));
        Compare enemySubsetComparison = compareAffectedStacksSubset(spellMechanics, getEnemyUnits(newCast), getEnemyUnits(oldCast));

        if (spellMechanics->isPositiveSpell())
                enemySubsetComparison = reverse(enemySubsetComparison);
        else if (spellMechanics->isNegativeSpell())
                alliedSubsetComparison = reverse(alliedSubsetComparison);

        std::set<Compare>comparisonResults = {alliedSubsetComparison, enemySubsetComparison};
        std::set<std::set<Compare>>possibleBetterResults = {{Compare::BETTER, Compare::BETTER},{Compare::BETTER, Compare::EQUAL}};
        std::set<std::set<Compare>>possibleWorstResults = {{Compare::WORSE, Compare::WORSE},{Compare::WORSE, Compare::EQUAL}};

        if (possibleBetterResults.find(comparisonResults) != possibleBetterResults.end())
                return Compare::BETTER;
        if (possibleWorstResults.find(comparisonResults) != possibleWorstResults.end())
                return Compare::WORSE;

        return Compare::DIFFERENT;
}

SpellTargetEvaluator::Compare SpellTargetEvaluator::compareAffectedStacksSubset(spells::Mechanics * spellMechanics, std::set<const CStack *> newSubset, std::set<const CStack *> oldSubset)
{
        if (newSubset.size() == oldSubset.size())
                return newSubset == oldSubset ? Compare::EQUAL : Compare::DIFFERENT;

        if (oldSubset.size() > newSubset.size())
                return reverse(compareAffectedStacksSubset(spellMechanics, oldSubset, newSubset));

        const std::set<const CStack *> & biggerSet = newSubset;
        const std::set<const CStack *> & smallerSet = oldSubset;

        if (std::includes(biggerSet.begin(), biggerSet.end(),
                          smallerSet.begin(), smallerSet.end()))
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


bool SpellTargetEvaluator::canBeCastAt(spells::Mechanics * spellMechanics, BattleHex hex)
{
    detail::ProblemImpl ignored;
    Destination des(hex);
    return spellMechanics->canBeCastAt({des}, ignored);
}

void SpellTargetEvaluator::addIfCanBeCast(spells::Mechanics * spellMechanics, BattleHex hex, std::vector<Target> & targets)
{
    detail::ProblemImpl ignored;
    Destination des(hex);
    if(spellMechanics->canBeCastAt({des}, ignored))
        targets.push_back({des});
}


