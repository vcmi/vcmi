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
#include "IQueryStackListener.h"

class CQuery;
using QueryPtr = std::shared_ptr<CQuery>;

class QueriesProcessor
{
public:
	using QueriesStack = std::vector<QueryPtr>;
	using QueriesPerPlayer = std::array<QueriesStack, PlayerColor::PLAYER_LIMIT_I>;

	// Sets an optional listener notified when a player's query stack changes.
	void setListener(IQueryStackListener * listener);

private:
	void addQuery(PlayerColor player, QueryPtr query);
	void popQuery(PlayerColor player, QueryPtr query);

	QueriesPerPlayer queries;
	IQueryStackListener * queriesStackListener = nullptr;

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
