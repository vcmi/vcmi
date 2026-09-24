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

	// A hero that lost a scripted combat no longer exists, so the coroutine is abandoned
	// instead of resumed with a hero that is gone.
	bool heroGone = visitingHero.hasValue() && gh->gameInfo().getHero(visitingHero) == nullptr;

	if(!dispatcher || heroGone)
	{
		owner->popIfTop(*this);
		return;
	}

	// Resuming may add a new child activity for another blocking action. The coroutine is
	// then not finished and this activity stays on the stack below that child.
	bool finished = dispatcher->resumeCoroutine(*gh, coroutineHandle, pendingAnswer);
	pendingAnswer.reset();

	if(finished)
		owner->popIfTop(*this);
}
