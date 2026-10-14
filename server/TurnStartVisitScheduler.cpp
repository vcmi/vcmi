/*
 * TurnStartVisitScheduler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TurnStartVisitScheduler.h"

#include "CGameHandler.h"
#include "lib/gameState/CGameState.h"
#include "queries/QueriesProcessor.h"

TurnStartVisitScheduler::TurnStartVisitScheduler(CGameHandler & gameHandler, QueriesProcessor & queries)
	: gameHandler(gameHandler)
	, queries(queries)
{
}

void TurnStartVisitScheduler::enqueue(PlayerColor player, std::deque<PendingTurnStartVisit> visits)
{
	sessions[player] = std::move(visits);
}

void TurnStartVisitScheduler::processNext(PlayerColor player)
{
	auto & session = sessions[player];

	if(session.empty())
		return;

	if(queries.topQuery(player))
		return;

	if(!session.empty())
	{
		const auto next = session.front();
		session.pop_front();

		const auto * object = gameHandler.gameState().getObjInstance(next.objectId);
		const auto * hero = gameHandler.gameState().getHero(next.heroId);

		gameHandler.objectVisited(object, hero);
	}
}

void TurnStartVisitScheduler::onQueryStackChanged(PlayerColor player)
{
	processNext(player);
}

void TurnStartVisitScheduler::clear(PlayerColor player)
{
	sessions[player].clear();
}

