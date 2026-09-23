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

private:
	void addQuery(PlayerColor player, QueryPtr query);
	void popQuery(PlayerColor player, QueryPtr query);

	QueriesPerPlayer queries;
	CGameHandler & gameHandler;

	/// Queries that are waiting for their player to become idle. Work that is not
	/// part of whatever the player is currently doing must not push itself on top of
	/// it - the player would be answering it instead of what they were asked first.
	std::array<std::deque<QueryPtr>, PlayerColor::PLAYER_LIMIT_I> waiting;

	/// IDs of queries that recently left a player's stack. Lets submitReply tell a
	/// harmless "your reply lost the race" apart from a genuinely bogus query ID.
	std::array<std::deque<QueryID>, PlayerColor::PLAYER_LIMIT_I> recentlyCompleted;
	static constexpr size_t RECENTLY_COMPLETED_LIMIT = 64;

	/// Number of query stack mutations currently in progress. Query hooks routinely
	/// add or remove further queries, so a single player action can nest several
	/// levels deep; deferred work runs only once the outermost one finishes.
	int mutationDepth = 0;

	/// Set while settle() is running, so that mutations it causes do not recurse
	/// back into it - its own loop picks them up instead.
	bool settling = false;

	/// Players whose stack changed. Cleared and re-read around the victory checks,
	/// which are the one piece of deferred work that does not report back directly.
	std::array<bool, PlayerColor::PLAYER_LIMIT_I> stackChanged = {};

	/// settle() gives up after this many rounds and logs an error, rather than
	/// spinning forever should two pieces of deferred work keep triggering each other.
	static constexpr int MAX_SETTLE_ROUNDS = 64;

	/// Steps a single routine may take in one go before the processor assumes it is
	/// stuck. Generous: a routine legitimately takes one step per unit of work, such
	/// as one per building visited in a town.
	static constexpr int MAX_ROUTINE_STEPS = 1000;

	void rememberCompleted(PlayerColor player, const QueryPtr & query);
	bool wasRecentlyCompleted(PlayerColor player, QueryID queryID) const;
	void markStackChanged(PlayerColor player);

	/// Steps every routine that is at the top of a player's stack, until it either
	/// finishes or suspends itself by pushing a child. Returns true if it changed
	/// anything.
	bool advanceRoutines();

	/// Pops every query at the top of a player's stack that has already been
	/// answered. Returns true if anything was removed.
	bool resolveAnsweredQueries();

	/// Starts the next waiting query of every player that has become idle.
	/// Returns true if it started anything.
	bool promoteWaitingQueries();

	/// Runs victory/loss checks for players that just became idle. Returns true if
	/// that changed a stack.
	bool runVictoryChecks();

	/// Runs everything that must not happen while the stacks are still moving:
	/// resolving answered queries, stepping routines, starting waiting queries and
	/// victory/loss checks.
	/// Loops until no further change, so callers always observe a settled state.
	void settle();

	/// RAII bracket around a public mutation. The outermost one settles on exit.
	class MutationScope : boost::noncopyable
	{
	public:
		explicit MutationScope(QueriesProcessor & owner);
		~MutationScope();

	private:
		QueriesProcessor & owner;
	};

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

	/// Adds a query once its players have nothing else to do. Use this for work that
	/// is not caused by the query the player is currently dealing with, so that it
	/// queues up behind it instead of interrupting it.
	void addQueryWhenIdle(QueryPtr query);

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
