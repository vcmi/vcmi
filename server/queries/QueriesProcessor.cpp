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

	rememberCompleted(player, query);
	markStackChanged(player);

	query->onRemoval(player);

	//Exposure on query below happens only if removal didn't trigger any new query
	if(nextQuery && nextQuery == topQuery(player))
		nextQuery->onExposure(query);

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

void QueriesProcessor::setListener(IQueryStackListener * listener)
{
	queriesStackListener = listener;
}

QueryPtr QueriesProcessor::getQuery(QueryID queryID, PlayerColor player)
{
	if(!player.isValidPlayer())
		return nullptr;

	for(const auto & query : queries.at(player.getNum()))
		if(query->queryID == queryID)
			return query;

	return nullptr;
}

void QueriesProcessor::rememberCompleted(PlayerColor player, const QueryPtr & query)
{
	if(!player.isValidPlayer() || !query)
		return;

	auto & completed = recentlyCompleted.at(player.getNum());
	completed.push_back(query->queryID);

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

bool QueriesProcessor::reportStackChanges()
{
	auto changed = stackChanged;
	stackChanged = {};

	if(!queriesStackListener)
		return false;

	for(size_t idx = 0; idx < changed.size(); ++idx)
	{
		if(changed.at(idx))
			queriesStackListener->onQueryStackChanged(PlayerColor(static_cast<int32_t>(idx)));
	}

	return std::ranges::any_of(stackChanged, [](bool value){ return value; });
}

bool QueriesProcessor::runVictoryChecks()
{
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

		if(reportStackChanges())
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

	query->setReply(reply);
	query->answeredBy = player;

	// The query is resolved for every player it affects, not just the one who
	// replied - but only once it is at the top of each of their stacks, which
	// settle() takes care of when this scope closes.
	return ReplyOutcome::Accepted;
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
