/*
 * Enums.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Enums.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

Enums::EnumMap<EHealLevel> Enums::exportHealLevel() const
{
	return {
		{ "heal", EHealLevel::HEAL },
		{ "resurrect", EHealLevel::RESURRECT },
		{ "overheal", EHealLevel::OVERHEAL },
	};
}

Enums::EnumMap<EHealPower> Enums::exportHealPower() const
{
	return {
		{ "oneBattle", EHealPower::ONE_BATTLE },
		{ "permanent", EHealPower::PERMANENT },
	};
}

Enums::EnumMap<ESpellCastProblem> Enums::exportSpellCastProblem() const
{
	return {
		{ "noHeroToCastSpell", ESpellCastProblem::NO_HERO_TO_CAST_SPELL},
		{ "castsPerTurnLimit", ESpellCastProblem::CASTS_PER_TURN_LIMIT},
		{ "noSpellbook", ESpellCastProblem::NO_SPELLBOOK},
		{ "heroDoesntKnowSpell", ESpellCastProblem::HERO_DOESNT_KNOW_SPELL},
		{ "notEnoughMana", ESpellCastProblem::NOT_ENOUGH_MANA},
		{ "advmapSpellInsteadOfBattleSpell", ESpellCastProblem::ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL},
		{ "spellLevelLimitExceeded", ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED},
		{ "noSpellsToDispel", ESpellCastProblem::NO_SPELLS_TO_DISPEL},
		{ "noAppropriateTarget", ESpellCastProblem::NO_APPROPRIATE_TARGET},
		{ "stackImmuneToSpell", ESpellCastProblem::STACK_IMMUNE_TO_SPELL},
		{ "wrongSpellTarget", ESpellCastProblem::WRONG_SPELL_TARGET},
		{ "ongoinTacticPhase", ESpellCastProblem::ONGOING_TACTIC_PHASE},
		{ "magicIsBlocked", ESpellCastProblem::MAGIC_IS_BLOCKED}
	};
}

}

VCMI_LIB_NAMESPACE_END
