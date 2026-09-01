/*
 * TurnStartVisitScheduler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "queries/IQueryStackListener.h"

class CGameHandler;
class QueriesProcessor;

struct PendingTurnStartVisit
{
	PlayerColor player;
	ObjectInstanceID objectId;
	ObjectInstanceID heroId;
};

// Schedules deferred turn-start object visits and runs them one at a time
// per player, resuming only after the previous visit's query chain finishes.
class TurnStartVisitScheduler final : public IQueryStackListener
{
public:
	TurnStartVisitScheduler(CGameHandler & gameHandler, QueriesProcessor & queries);

	void enqueue(PlayerColor player, std::deque<PendingTurnStartVisit> visits);
	void processNext(PlayerColor player);
	void onQueryStackChanged(PlayerColor player) override;
	void clear(PlayerColor player);

private:
	CGameHandler & gameHandler;
	QueriesProcessor & queries;
	std::array<std::deque<PendingTurnStartVisit>, PlayerColor::PLAYER_LIMIT_I> sessions;
};

