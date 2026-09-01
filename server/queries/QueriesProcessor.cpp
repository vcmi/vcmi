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

#include "CQuery.h"

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

	query->onRemoval(player);

	//Exposure on query below happens only if removal didn't trigger any new query
	if(nextQuery && nextQuery == topQuery(player))
		nextQuery->onExposure(query);

	if(queriesStackListener)
		queriesStackListener->onQueryStackChanged(player);
}

void QueriesProcessor::popQuery(const CQuery &query)
{
	LOG_TRACE_PARAMS(logGlobal, "query='%s'", query);

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
	for(auto player : query->players)
		popQuery(player, query);
}

void QueriesProcessor::addQuery(QueryPtr query)
{
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
	query->onAdded(player);

	if(queriesStackListener)
		queriesStackListener->onQueryStackChanged(player);
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

