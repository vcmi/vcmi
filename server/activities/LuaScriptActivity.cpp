/*
 * LuaScriptActivity.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaScriptActivity.h"

#include "ActivityProcessor.h"
#include "../CGameHandler.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/gameState/CGameState.h"

#include <vcmi/scripting/MapEventDispatcher.h>

LuaScriptActivity::LuaScriptActivity(CGameHandler * owner, PlayerColor player):
	Activity(owner, TYPE)
{
	addPlayer(player);
}

void LuaScriptActivity::setCoroutine(int handle)
{
	coroutineHandle = handle;
}

void LuaScriptActivity::setPendingAnswer(std::optional<int32_t> answer)
{
	pendingAnswer = answer;
}

void LuaScriptActivity::setVisitingHero(ObjectInstanceID hero)
{
	visitingHero = hero;
}

void LuaScriptActivity::onExposure(ActivityPtr topActivity)
{
	auto * dispatcher = gh->gameState().getMapEventDispatcher();

	// If the hero lost a scripted combat it no longer exists; abandon the coroutine rather than resume
	// a handler whose captured hero is gone.
	bool heroGone = visitingHero.hasValue() && gh->gameInfo().getHero(visitingHero) == nullptr;

	if(!dispatcher || heroGone)
	{
		owner->popIfTop(*this);
		return;
	}

	// Resuming may spawn a new child activity (another blocking action); in that case the coroutine is
	// not finished and this activity stays on the stack under the freshly-added child.
	bool finished = dispatcher->resumeCoroutine(*gh, coroutineHandle, pendingAnswer);
	pendingAnswer.reset();

	if(finished)
		owner->popIfTop(*this);
}
