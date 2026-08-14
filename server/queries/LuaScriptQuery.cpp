/*
 * LuaScriptQuery.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaScriptQuery.h"

#include "QueriesProcessor.h"
#include "../CGameHandler.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/gameState/CGameState.h"

#include <vcmi/scripting/MapEventDispatcher.h>

LuaScriptQuery::LuaScriptQuery(CGameHandler * owner, PlayerColor player):
	CQuery(owner, TYPE)
{
	addPlayer(player);
}

void LuaScriptQuery::setCoroutine(int handle)
{
	coroutineHandle = handle;
}

void LuaScriptQuery::setPendingAnswer(std::optional<int32_t> answer)
{
	pendingAnswer = answer;
}

void LuaScriptQuery::setVisitingHero(ObjectInstanceID hero)
{
	visitingHero = hero;
}

void LuaScriptQuery::onExposure(QueryPtr topQuery)
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

	// Resuming may spawn a new child query (another blocking action); in that case the coroutine is
	// not finished and this query stays on the stack under the freshly-added child.
	bool finished = dispatcher->resumeCoroutine(*gh, coroutineHandle, pendingAnswer);
	pendingAnswer.reset();

	if(finished)
		owner->popIfTop(*this);
}
