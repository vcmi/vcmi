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

#include "../../lib/spells/BattleSpellMechanics.h"
#include <vcmi/spells/Magic.h>

class SpellTargetEvaluator
{
public:
	static std::vector<spells::Target> getViableTargets(const spells::Mechanics * spellMechanics);

private:
	enum Compare
	{
		EQUAL,
		DIFFERENT,
		BETTER,
		WORSE
	};

	static std::vector<spells::Target> defaultLocationSpellHeuristics(const spells::Mechanics * spellMechanics);
	static std::vector<spells::Target> allTargetableCreatures(const spells::Mechanics * spellMechanics);
	static std::vector<spells::Target> theBestLocationCasts(const spells::Mechanics * spellMechanics);
	static Compare compareAffectedStacks(
	const spells::Mechanics * spellMechanics, const std::set<const CStack *> & newCast, const std::set<const CStack *> & oldCast);
	static Compare compareAffectedStacksSubset(
	const spells::Mechanics * spellMechanics, const std::set<const CStack *> & newSubset, const std::set<const CStack *> & oldSubset);
	static SpellTargetEvaluator::Compare reverse(Compare compare);
	static bool isCastHarmful(const spells::Mechanics * spellMechanics, const std::set<const CStack *> & affectedStacks);
	static bool canBeCastAt(const spells::Mechanics * spellMechanics, BattleHex hex);
	static void addIfCanBeCast(const spells::Mechanics * spellMechanics, BattleHex hex, std::vector<spells::Target> & targets);
};
