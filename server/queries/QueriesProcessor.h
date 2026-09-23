/*
 * QueriesProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "IQueryStackListener.h"
#include "constants/EntityIdentifiers.h"
#include "queries/CQuery.h"

class CGameHandler;
class CQuery;
using QueryPtr = std::shared_ptr<CQuery>;

/// Result of offering a player's reply to the query system.
/// Only Rejected* outcomes indicate a problem; the Ignored* ones are benign races
/// that are expected in normal play and must not be reported to the player.
enum class ReplyOutcome : uint8_t
{
	/// Reply was recorded. The query may or may not have been resolved yet:
	/// it is resolved once it reaches the top of every affected player's stack.
	Accepted,

	/// The query completed before this reply arrived - typically because it was
	/// removed by some other event while the reply was in flight. Benign.
	IgnoredAlreadyCompleted,

	/// This query has already been answered (duplicate or retried reply). Benign.
	IgnoredAlreadyAnswered,

	/// No such query, and none recently completed - the client is out of sync
	/// or the reply is malformed.
	RejectedUnknownQuery,

	/// The query exists but this player is not one of the players it affects.
	RejectedWrongPlayer,

	/// The query exists but is not the kind that a player reply can end.
	RejectedNotAnswerable,
};

class QueriesProcessor
{
public:
	explicit QueriesProcessor(CGameHandler & gameHandler);

	using QueriesStack = std::vector<QueryPtr>;
	using QueriesPerPlayer = std::array<QueriesStack, PlayerColor::PLAYER_LIMIT_I>;

	// Sets an optional listener notified when a player's query stack changes.
	void setListener(IQueryStackListener * listener);

private:
	void addQuery(PlayerColor player, QueryPtr query);
	void popQuery(PlayerColor player, QueryPtr query);

	QueriesPerPlayer queries;
	CGameHandler & gameHandler;
	IQueryStackListener * queriesStackListener = nullptr;

	/// IDs of queries that recently left a player's stack. Lets submitReply tell a
	/// harmless "your reply lost the race" apart from a genuinely bogus query ID.
	std::array<std::deque<QueryID>, PlayerColor::PLAYER_LIMIT_I> recentlyCompleted;
	static constexpr size_t RECENTLY_COMPLETED_LIMIT = 64;

	/// Guards resolveAnsweredQueries against re-entry: removing a query runs hooks
	/// that may themselves add or remove queries, which would otherwise recurse.
	std::array<bool, PlayerColor::PLAYER_LIMIT_I> resolvingAnswered = {};

	void rememberCompleted(PlayerColor player, const QueryPtr & query);
	bool wasRecentlyCompleted(PlayerColor player, QueryID queryID) const;

	/// Pops every query at the top of this player's stack that has already been
	/// answered. Loops, because removing one answered query can expose another.
	void resolveAnsweredQueries(PlayerColor player);

	template<typename StorageT>
	class AllQueriesViewT
	{
	public:
		explicit AllQueriesViewT(StorageT & s) : storage(&s) {}

		struct iterator
		{
			StorageT * storage = nullptr;
			size_t outer = 0;
			size_t inner = 0;

			void advance()
			{
				while(outer < storage->size())
				{
					auto & v = (*storage)[outer];
					if(inner < v.size())
						return;

					outer++;
					inner = 0;
				}
			}

			decltype(auto) operator*() const
			{
				return (*storage)[outer][inner]; // QueryPtr& or const QueryPtr&
			}

			iterator & operator++()
			{
				inner++;
				advance();
				return *this;
			}

			bool operator==(const iterator & other) const
			{
				return storage == other.storage && outer == other.outer && inner == other.inner;
			}

			bool operator!=(const iterator & other) const
			{
				return !(*this == other);
			}
		};

		iterator begin() const
		{
			iterator it{ storage, 0, 0 };
			it.advance();
			return it;
		}

		iterator end() const
		{
			return iterator{ storage, storage->size(), 0 };
		}

	private:
		StorageT * storage;
	};

public:
	using AllQueriesView = AllQueriesViewT<QueriesPerPlayer>;
	using AllQueriesViewConst = AllQueriesViewT<const QueriesPerPlayer>;

	void addQuery(QueryPtr query);
	void popQuery(const CQuery &query);
	void popQuery(QueryPtr query);
	void popIfTop(const CQuery &query); //removes this query if it is at the top (otherwise, do nothing)
	void popIfTop(QueryPtr query); //removes this query if it is at the top (otherwise, do nothing)

	QueryPtr topQuery(PlayerColor player);
	QueryPtr getQuery(QueryID queryID);

	/// Looks the query up on this player's stack only. Prefer this over getQuery()
	/// when handling player input: query IDs are not guaranteed to be unique across
	/// players (QueryID::CLIENT is shared by every pause query), so a global lookup
	/// can return another player's query.
	QueryPtr getQuery(QueryID queryID, PlayerColor player);

	/// Records a player's reply to a query. The query does not have to be at the top
	/// of the stack - the server may well have pushed something else between sending
	/// the prompt and receiving the answer, and rejecting the reply for that reason
	/// would leave both sides waiting for each other forever.
	ReplyOutcome submitReply(QueryID queryID, PlayerColor player, std::optional<int32_t> reply);

	/// Multi-line dump of every player's stack, for diagnosing a stuck player.
	std::string describeStacks() const;

	AllQueriesView allQueries();
	AllQueriesViewConst allQueries() const;
	int countQuery(const QueryPtr & query) const;

	template<typename T, typename QueryPtrT>
	using QueryAsResult = std::conditional_t<
		std::is_const_v<std::remove_pointer_t<decltype(std::declval<const QueryPtrT &>().get())>>,
		const T *,
		T *>;

	template<typename T, typename QueryPtrT>
	QueryAsResult<T, QueryPtrT> queryAs(const QueryPtrT & query)
	{
		using ResultT = std::remove_pointer_t<QueryAsResult<T, QueryPtrT>>;

		if(!query)
			return nullptr;

		if(query->getType() != T::TYPE)
			return nullptr;

		assert(dynamic_cast<ResultT*>(query.get()) != nullptr);
		return static_cast<ResultT*>(query.get());
	}

	template<typename T, typename Predicate>
	T * findQuery(Predicate predicate) const
	{
		for(const auto & playerQueries : queries)
		{
			for(auto it = playerQueries.rbegin(); it != playerQueries.rend(); ++it)
			{
				auto * query = dynamic_cast<T *>(it->get());
				if(query && predicate(*query))
					return query;
			}
		}

		return nullptr;
	}

};
