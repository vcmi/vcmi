/*
 * SpellActivities.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SpellActivities.h"

#include "ActivityProcessor.h"
#include "LuaScriptActivity.h"

#include "../CGameHandler.h"
#include "../ServerSpellCastEnvironment.h"

#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/spells/ISpellMechanics.h"

TownSelectionActivity::TownSelectionActivity(CGameHandler * owner, PlayerColor player, SpellID spell, ObjectInstanceID caster, std::vector<ObjectInstanceID> offeredTowns)
	: DialogActivity(owner, TYPE)
	, spell(spell)
	, caster(caster)
	, offeredTowns(std::move(offeredTowns))
{
	addPlayer(player);
}

bool TownSelectionActivity::acceptsAnswerWithoutValue() const
{
	// The player may close the window instead of picking a town, which cancels the cast
	return true;
}

void TownSelectionActivity::onRemoval(PlayerColor color)
{
	if(!answer)
		return; // window was closed without choosing a town

	const ObjectInstanceID chosen(*answer);

	if(!vstd::contains(offeredTowns, chosen))
	{
		gh->complain("Invalid town selected in dialog");
		return;
	}

	// Both may have been removed while the dialog was open
	const auto * town = gh->gameInfo().getTown(chosen);
	const auto * hero = gh->gameInfo().getHero(caster);

	if(!town || !hero)
		return;

	AdventureSpellCastParameters parameters;
	parameters.caster = hero;
	parameters.pos = town->visitablePos();

	// performCast instead of a new cast: castability was already checked before asking the
	// player, and a new cast would charge the spell cost twice.
	spell.toSpell()->getAdventureMechanics().performCast(gh->spellEnv.get(), parameters);
}

ScriptDialogActivity::ScriptDialogActivity(CGameHandler * owner, PlayerColor player)
	: DialogActivity(owner, TYPE)
{
	addPlayer(player);
}

bool ScriptDialogActivity::acceptsAnswerWithoutValue() const
{
	return true;
}

void ScriptDialogActivity::onRemoval(PlayerColor color)
{
	// The script below this activity is suspended until it receives this answer
	if(auto * script = owner->findSoleActivity<LuaScriptActivity>(color))
		script->setPendingAnswer(answer);
}
