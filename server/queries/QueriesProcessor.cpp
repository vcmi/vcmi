/*
 * CQuery.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "QueriesProcessor.h"

#include "../CGameHandler.h"
#include "CQuery.h"

QueriesProcessor::QueriesProcessor(CGameHandler & gameHandler)
	: gameHandler(gameHandler)
{
}

void QueriesProcessor::popQuery(PlayerColor player, QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%s', query='%s'", player % query);
	if(topQuery(player) != query)
	{
		logGlobal->trace("Cannot remove, not a top!");
		return;
	}

	const auto idx = static_cast<size_t>(player.getNum());
	assert(query);

	auto & stack = queries.at(idx);
	stack.pop_back();
	auto nextQuery = topQuery(player);

	rememberCompleted(player, query->queryID);
	rememberCompleted(player, query->getActiveQuestionID());
	markStackChanged(player);

	query->onRemoval(player);

	//Exposure on query below happens only if removal didn't trigger any new query
	if(nextQuery && nextQuery == topQuery(player))
	{
		// A routine is told which child finished and is then stepped by settle();
		// it must not also receive the generic exposure hook.
		if(auto * routine = nextQuery->asRoutine())
			routine->onChildCompleted(query);
		else
			nextQuery->onExposure(query);
	}

	// Resolving answered queries, notifying the listener and checking victory
	// conditions all happen in settle(), once the stacks have stopped moving.
}

void QueriesProcessor::popQuery(const CQuery &query)
{
	LOG_TRACE_PARAMS(logGlobal, "query='%s'", query);
	MutationScope mutation(*this);

	assert(query.players.size());
	for(auto player : query.players)
	{
		auto top = topQuery(player);
		if(top.get() == &query)
			popQuery(top);
		else
		{
			if(logGlobal->isTraceEnabled())
			{
				const auto idx = static_cast<size_t>(player.getNum());

				logGlobal->trace("Cannot remove query %s", query.toString());
				logGlobal->trace("Queries found:");
				for(const auto & q : queries.at(idx))
				{
					logGlobal->trace(q->toString());
				}
			}
		}
	}
}

void QueriesProcessor::popQuery(QueryPtr query)
{
	MutationScope mutation(*this);

	for(auto player : query->players)
		popQuery(player, query);
}

void QueriesProcessor::addQuery(QueryPtr query)
{
	MutationScope mutation(*this);

	for(auto player : query->players)
		addQuery(player, query);
}

void QueriesProcessor::addQueryWhenIdle(QueryPtr query)
{
	MutationScope mutation(*this);

	assert(query);
	for(auto player : query->players)
	{
		if(player.isValidPlayer())
			waiting.at(player.getNum()).push_back(query);
	}
}

void QueriesProcessor::addQuery(PlayerColor player, QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%d', query='%s'", player.getNum() % query);

	const auto idx = static_cast<size_t>(player.getNum());
	assert(query);
	auto & stack = queries.at(idx);
	// Prevent adding the same query twice in a row
	if(!stack.empty() && stack.back() == query)
		return;
	query->onAdding(player);
	queries.at(idx).push_back(query);
	markStackChanged(player);
	query->onAdded(player);
}

QueryPtr QueriesProcessor::topQuery(PlayerColor player)
{
	assert(player.isValidPlayer());
	if(!player.isValidPlayer())
		return nullptr;

	return vstd::backOrNull(queries[player]);
}

void QueriesProcessor::popIfTop(QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "query='%d'", query);
	if(!query)
	{
		logGlobal->error("The query is nullptr! Ignoring.");
		return;
	}

	popIfTop(*query);
}

void QueriesProcessor::popIfTop(const CQuery & query)
{
	MutationScope mutation(*this);

	for(PlayerColor color : query.players)
		if(topQuery(color).get() == &query)
			popQuery(color, topQuery(color));
}

QueriesProcessor::AllQueriesViewConst QueriesProcessor::allQueries() const
{
	return AllQueriesViewConst(queries);
}

QueriesProcessor::AllQueriesView QueriesProcessor::allQueries()
{
	return AllQueriesView(queries);
}

QueryPtr QueriesProcessor::getQuery(QueryID queryID)
{
	for(auto & playerQueries : queries)
		for(auto & query : playerQueries)
			if(query->queryID == queryID)
				return query;
	return nullptr;
}

int QueriesProcessor::countQuery(const QueryPtr & query) const
{
	if(!query)
		return 0;

	int result = 0;
	for(const auto & currentQuery : allQueries())
	{
		if(currentQuery == query)
			++result;
	}
	return result;
}

QueryPtr QueriesProcessor::getQuery(QueryID queryID, PlayerColor player)
{
	if(!player.isValidPlayer())
		return nullptr;

	for(const auto & query : queries.at(player.getNum()))
		if(query->queryID == queryID || query->getActiveQuestionID() == queryID)
			return query;

	return nullptr;
}

void QueriesProcessor::rememberCompleted(PlayerColor player, QueryID queryID)
{
	if(!player.isValidPlayer() || !queryID.hasValue())
		return;

	auto & completed = recentlyCompleted.at(player.getNum());
	completed.push_back(queryID);

	while(completed.size() > RECENTLY_COMPLETED_LIMIT)
		completed.pop_front();
}

bool QueriesProcessor::wasRecentlyCompleted(PlayerColor player, QueryID queryID) const
{
	if(!player.isValidPlayer())
		return false;

	return vstd::contains(recentlyCompleted.at(player.getNum()), queryID);
}

void QueriesProcessor::markStackChanged(PlayerColor player)
{
	if(player.isValidPlayer())
		stackChanged.at(player.getNum()) = true;
}

QueriesProcessor::MutationScope::MutationScope(QueriesProcessor & owner)
	: owner(owner)
{
	owner.mutationDepth++;
}

QueriesProcessor::MutationScope::~MutationScope()
{
	owner.mutationDepth--;

	if(owner.mutationDepth == 0)
		owner.settle();
}

bool QueriesProcessor::advanceRoutines()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		for(int step = 0; step < MAX_ROUTINE_STEPS; ++step)
		{
			auto top = topQuery(player);
			if(!top)
				break;

			auto * routine = top->asRoutine();
			if(!routine)
				break;

			const size_t depthBefore = queries.at(idx).size();
			const StepResult result = routine->advance();
			changedAnything = true;

			// The step pushed a child query: the routine is suspended until it is done.
			if(queries.at(idx).size() != depthBefore || topQuery(player) != top)
				break;

			if(result == StepResult::Done)
			{
				popQuery(player, top);
				break;
			}

			if(step + 1 == MAX_ROUTINE_STEPS)
				logGlobal->error("Routine did not finish after %d steps: %s", MAX_ROUTINE_STEPS, top->toString());
		}
	}

	return changedAnything;
}

bool QueriesProcessor::advanceInteractions()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		auto top = topQuery(player);
		if(!top)
			continue;

		auto * interaction = top->asInteraction();
		if(!interaction || top->isAnswered())
			continue;

		// The player has been asked and has not answered yet.
		if(top->hasOutstandingQuestion())
			continue;

		switch(interaction->askNextQuestion())
		{
			case PromptResult::Asked:
				changedAnything = true;
				break;

			case PromptResult::Finished:
				popQuery(player, top);
				changedAnything = true;
				break;

			case PromptResult::NotReady:
				break;
		}
	}

	return changedAnything;
}

bool QueriesProcessor::resolveAnsweredQueries()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		while(auto top = topQuery(player))
		{
			if(!top->isAnswered())
				break;

			if(!top->endsByPlayerAnswer())
				break; // should not happen - submitReply refuses to answer such queries

			popQuery(player, top);
			changedAnything = true;
		}
	}

	return changedAnything;
}

bool QueriesProcessor::promoteWaitingQueries()
{
	bool startedAnything = false;

	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		if(!queries.at(idx).empty())
			continue; // fast path; everyPlayerIsIdle below is the actual condition

		auto & queue = waiting.at(idx);
		if(queue.empty())
			continue;

		auto query = queue.front();
		queue.pop_front();

		// A query shared by several players is queued for each of them; start it
		// only once, when the last of them is ready for it.
		const bool everyPlayerIsIdle = std::ranges::all_of(query->players, [this](PlayerColor player)
		{
			return player.isValidPlayer() && queries.at(player.getNum()).empty();
		});

		if(!everyPlayerIsIdle)
		{
			queue.push_front(query);
			continue;
		}

		for(auto player : query->players)
			vstd::erase_if_present(waiting.at(player.getNum()), query);

		addQuery(query);
		startedAnything = true;
	}

	return startedAnything;
}

bool QueriesProcessor::runVictoryChecks()
{
	// Cleared first so that the flags report only what the checks themselves change.
	stackChanged = {};

	// checkVictoryLossConditionsForPlayer() ignores players that still have queries,
	// so it is enough to offer it every player that just went idle.
	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		if(!queries.at(idx).empty())
			continue;

		gameHandler.checkVictoryLossConditionsForPlayer(player);
	}

	return std::ranges::any_of(stackChanged, [](bool value){ return value; });
}

void QueriesProcessor::settle()
{
	if(settling)
		return; // the running settle() loop will observe whatever changed

	settling = true;

	// Must be cleared even if a query hook throws, otherwise no deferred work
	// would ever run again.
	struct SettlingReset
	{
		bool & flag;
		~SettlingReset() { flag = false; }
	} settlingReset{settling};

	for(int round = 0; round < MAX_SETTLE_ROUNDS; ++round)
	{
		// A reply may have arrived for a query that was buried at the time. Now that
		// the stacks have stopped moving, any answered query on top must be removed.
		if(resolveAnsweredQueries())
			continue;

		// A routine exposed by that removal has more work to do before the player
		// can be considered idle.
		if(advanceRoutines())
			continue;

		// An interaction may still have questions to put to the player.
		if(advanceInteractions())
			continue;

		// Only now, with nothing left of whatever the player was doing, may work
		// that was queued up behind it begin.
		if(promoteWaitingQueries())
			continue;

		if(runVictoryChecks())
			continue;

		return;
	}

	logGlobal->error("Query stacks did not settle after %d rounds! Queries:\n%s", MAX_SETTLE_ROUNDS, describeStacks());
}

ReplyOutcome QueriesProcessor::submitReply(QueryID queryID, PlayerColor player, std::optional<int32_t> reply)
{
	MutationScope mutation(*this);

	auto query = getQuery(queryID, player);

	if(!query)
	{
		// The query may have been removed while the reply was travelling to us.
		// That is a normal race and must not be reported as a problem.
		if(wasRecentlyCompleted(player, queryID))
			return ReplyOutcome::IgnoredAlreadyCompleted;

		// It may also belong to a different player - report that specifically,
		// rather than claiming the query does not exist at all.
		if(getQuery(queryID))
			return ReplyOutcome::RejectedWrongPlayer;

		return ReplyOutcome::RejectedUnknownQuery;
	}

	if(!vstd::contains(query->players, player))
		return ReplyOutcome::RejectedWrongPlayer;

	if(!query->endsByPlayerAnswer())
		return ReplyOutcome::RejectedNotAnswerable;

	if(query->isAnswered())
		return ReplyOutcome::IgnoredAlreadyAnswered;

	if(auto * interaction = query->asInteraction())
	{
		// An interaction asks more than once, so an answer has to name the question
		// it belongs to. Naming the interaction itself is not good enough: that would
		// let an answer to a question already superseded be taken for the current one.
		if(queryID != query->getActiveQuestionID())
			return ReplyOutcome::IgnoredAlreadyCompleted;

		// An interaction is not finished by an answer - it may have more to ask.
		// Remember the question so that a repeated answer to it is recognised as a
		// stale one rather than mistaken for an answer to whatever is asked next.
		rememberCompleted(player, query->getActiveQuestionID());
		query->activeQuestionID = QueryID::NONE;
		interaction->applyAnswer(reply);
		return ReplyOutcome::Accepted;
	}

	query->setReply(reply);
	query->answeredBy = player;

	// The query is resolved for every player it affects, not just the one who
	// replied - but only once it is at the top of each of their stacks, which
	// settle() takes care of when this scope closes.
	return ReplyOutcome::Accepted;
}

void QueriesProcessor::retryDeferredWork(PlayerColor player)
{
	MutationScope mutation(*this);
	markStackChanged(player);
}

std::string QueriesProcessor::describeStacks() const
{
	std::string result;

	for(size_t idx = 0; idx < queries.size(); ++idx)
	{
		const auto & stack = queries.at(idx);
		if(stack.empty())
			continue;

		result += boost::str(boost::format("  player %d, %d quer%s (top last):\n")
			% idx
			% stack.size()
			% (stack.size() == 1 ? "y" : "ies"));

		for(const auto & query : stack)
			result += "    " + query->toString() + "\n";
	}

	if(result.empty())
		return "  (no player has any queries)\n";

	return result;
}
