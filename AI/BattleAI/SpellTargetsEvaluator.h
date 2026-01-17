/*
 * SpellTargetsEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/spells/Magic.h>
#include "../../lib/spells/BattleSpellMechanics.h"

class SpellTargetEvaluator
{
public:
        static std::vector<spells::Target> getViableTargets(spells::Mechanics * spellMechanics);
private:

        enum Compare {
          EQUAL,
          DIFFERENT,
          BETTER,
          WORSE
        };

        static std::vector<spells::Target> defaultHeuristics(spells::Mechanics * spellMechanics);
        static std::vector<spells::Target> allSusceptibleCreatures(spells::Mechanics * spellMechanics);
        static std::vector<spells::Target> theBestLocationCasts(spells::Mechanics * spellMechanics);
        static Compare compareAffectedStacks(spells::Mechanics * spellMechanics, std::set<const CStack *> newCast, std::set<const CStack *> oldCast);
        static Compare compareAffectedStacksSubset(spells::Mechanics * spellMechanics, std::set<const CStack *> newSubset, std::set<const CStack *> oldSubset);
        static SpellTargetEvaluator::Compare reverse(Compare compare);
        static bool isCastHarmful(spells::Mechanics * spellMechanics, std::set<const CStack *> affectedStacks);
        static bool canBeCastAt(spells::Mechanics * spellMechanics, BattleHex hex);
        static void addIfCanBeCast(spells::Mechanics * spellMechanics, BattleHex hex, std::vector<spells::Target> & targets);
};
