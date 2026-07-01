#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../../server/queries/CQuery.h"
#include "../../../server/IGameServer.h"
#include "../../../server/queries/QueriesProcessor.h"
#include "CGameHandler.h"

namespace
{

class DummyGameServer : public IGameServer
{
public:
	void setState(EServerState value) override
	{
		state = value;
	}

	EServerState getState() const override
	{
		return state;
	}

	bool isPlayerHost(const PlayerColor & color) const override
	{
		return false;
	}

	bool hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const override
	{
		return false;
	}

	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const override
	{
		return false;
	}

	void applyPack(CPackForClient & pack) override
	{
	}

	void sendPack(CPackForClient & pack, GameConnectionID connectionID) override
	{
	}

private:
	EServerState state{};
};

enum class QueryEvent
{
	OnAdding,
	OnAdded,
	OnRemoval,
	OnExposure
};

class TestQuery;

struct RecordedEvent
{
	TestQuery * query;
	QueryEvent event;
};

class TestQuery : public CQuery
{
public:
	TestQuery(CGameHandler * gh, std::initializer_list<PlayerColor> affectedPlayers, QueryType type)
		: CQuery(gh, type)
	{
		for(auto player : affectedPlayers)
			players.push_back(player);
	}

	TestQuery(CGameHandler * gh, PlayerColor player, QueryType type)
		: CQuery(gh, type)
	{
		players.push_back(player);
	}

	std::vector<RecordedEvent> * sharedEventLog = nullptr;
	std::vector<QueryEvent> events;
	std::vector<PlayerColor> onAddingCalls;
	std::vector<PlayerColor> onAddedCalls;
	std::vector<PlayerColor> onRemovalCalls;
	std::vector<QueryPtr> exposureArgs;
	bool popOnExposure = false;
	bool addReplacementOnRemoval = false;
	QueryPtr replacementQuery;

	void onAdding(PlayerColor color) override
	{
		events.push_back(QueryEvent::OnAdding);
		onAddingCalls.push_back(color);
		if(sharedEventLog)
			sharedEventLog->push_back({this, QueryEvent::OnAdding});
	}

	void onAdded(PlayerColor color) override
	{
		events.push_back(QueryEvent::OnAdded);
		onAddedCalls.push_back(color);
		if(sharedEventLog)
			sharedEventLog->push_back({this, QueryEvent::OnAdded});
	}

	void onRemoval(PlayerColor color) override
	{
		events.push_back(QueryEvent::OnRemoval);
		onRemovalCalls.push_back(color);

		if(addReplacementOnRemoval && replacementQuery)
			owner->addQuery(replacementQuery);

		if(sharedEventLog)
			sharedEventLog->push_back({this, QueryEvent::OnRemoval});
	}

	void onExposure(QueryPtr topQuery) override
	{
		events.push_back(QueryEvent::OnExposure);
		exposureArgs.push_back(topQuery);

		if(sharedEventLog)
			sharedEventLog->push_back({this, QueryEvent::OnExposure});

		if(popOnExposure)
			owner->popIfTop(*this);
	}
};

class QueriesProcessorTest : public ::testing::Test
{
protected:
	DummyGameServer server;
	CGameHandler gh{server};
	QueriesProcessor & queries = *gh.queries;
};

}

TEST_F(QueriesProcessorTest, topQuery_returnsNullWhenPlayerHasNoQueries)
{
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
}

TEST_F(QueriesProcessorTest, popIfTop_removesTopQuery)
{
	auto query = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);

	queries.addQuery(query);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), query);
	EXPECT_EQ(queries.countQuery(query), 1);

	queries.popIfTop(query);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
	EXPECT_EQ(queries.countQuery(query), 0);
}

TEST_F(QueriesProcessorTest, popIfTop_doesNothingWhenQueryIsNotPresent)
{
	auto addedQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto missingQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(addedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), addedQuery);
	EXPECT_EQ(queries.countQuery(addedQuery), 1);
	EXPECT_EQ(queries.countQuery(missingQuery), 0);

	queries.popIfTop(missingQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), addedQuery);
	EXPECT_EQ(queries.countQuery(addedQuery), 1);
	EXPECT_EQ(queries.countQuery(missingQuery), 0);
}

TEST_F(QueriesProcessorTest, popIfTop_skipsWhenNestedQueryIsAbove_andLaterSucceedsAfterUnwind)
{
	auto movementQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto visitQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(movementQuery);
	queries.addQuery(visitQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), visitQuery);
	EXPECT_EQ(queries.countQuery(movementQuery), 1);
	EXPECT_EQ(queries.countQuery(visitQuery), 1);

	queries.popIfTop(movementQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), visitQuery);
	EXPECT_EQ(queries.countQuery(movementQuery), 1);
	EXPECT_EQ(queries.countQuery(visitQuery), 1);

	queries.popIfTop(visitQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), movementQuery);
	EXPECT_EQ(queries.countQuery(movementQuery), 1);
	EXPECT_EQ(queries.countQuery(visitQuery), 0);

	queries.popIfTop(movementQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
	EXPECT_EQ(queries.countQuery(movementQuery), 0);
	EXPECT_EQ(queries.countQuery(visitQuery), 0);
}

TEST_F(QueriesProcessorTest, popIfTop_exposesQueryBelowWithRemovedQueryAsArgument)
{
	auto bottomQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto topQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(bottomQuery);
	queries.addQuery(topQuery);

	queries.popIfTop(topQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), bottomQuery);

	EXPECT_EQ(bottomQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded,
		QueryEvent::OnExposure
	}));

	EXPECT_EQ(bottomQuery->onAddingCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_EQ(bottomQuery->onAddedCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_TRUE(bottomQuery->onRemovalCalls.empty());

	ASSERT_EQ(bottomQuery->exposureArgs.size(), 1);
	EXPECT_EQ(bottomQuery->exposureArgs[0], topQuery);

	EXPECT_EQ(topQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded,
		QueryEvent::OnRemoval
	}));
}

TEST_F(QueriesProcessorTest, popIfTop_allowsExposedQueryToPopItself)
{
	auto bottomQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto topQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	bottomQuery->popOnExposure = true;

	queries.addQuery(bottomQuery);
	queries.addQuery(topQuery);

	queries.popIfTop(topQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
	EXPECT_EQ(queries.countQuery(bottomQuery), 0);
	EXPECT_EQ(queries.countQuery(topQuery), 0);

	EXPECT_EQ(bottomQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded,
		QueryEvent::OnExposure,
		QueryEvent::OnRemoval
	}));

	ASSERT_EQ(bottomQuery->exposureArgs.size(), 1);
	EXPECT_EQ(bottomQuery->exposureArgs[0], topQuery);

	EXPECT_EQ(bottomQuery->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(1)}));

	EXPECT_EQ(topQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded,
		QueryEvent::OnRemoval
	}));
}

TEST_F(QueriesProcessorTest, popIfTop_removesMultiPlayerQueryOnlyWhereItIsTop)
{
	auto sharedQuery = std::make_shared<TestQuery>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		QueryType::Generic);

	auto blueTopQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(sharedQuery);
	queries.addQuery(blueTopQuery);

	queries.popIfTop(sharedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), nullptr);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), blueTopQuery);
	EXPECT_EQ(queries.countQuery(sharedQuery), 1);
	EXPECT_EQ(queries.countQuery(blueTopQuery), 1);

	EXPECT_EQ(sharedQuery->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(0)}));
}

TEST_F(QueriesProcessorTest, popIfTop_removesMultiPlayerQueryAfterItBecomesTopAgain)
{
	auto sharedQuery = std::make_shared<TestQuery>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		QueryType::Generic);

	auto blueTopQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(sharedQuery);
	queries.addQuery(blueTopQuery);

	queries.popIfTop(sharedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), nullptr);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), blueTopQuery);
	EXPECT_EQ(queries.countQuery(sharedQuery), 1);

	queries.popIfTop(blueTopQuery);

	ASSERT_EQ(sharedQuery->exposureArgs.size(), 1);
	EXPECT_EQ(sharedQuery->exposureArgs[0], blueTopQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), sharedQuery);
	EXPECT_EQ(queries.countQuery(blueTopQuery), 0);
	EXPECT_EQ(queries.countQuery(sharedQuery), 1);

	queries.popIfTop(sharedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), nullptr);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
	EXPECT_EQ(queries.countQuery(sharedQuery), 0);

	EXPECT_EQ(sharedQuery->onRemovalCalls, std::vector<PlayerColor>({
		PlayerColor(0),
		PlayerColor(1)
	}));
}

TEST_F(QueriesProcessorTest, popQuery_removesMultiPlayerQueryOnlyWhereItIsTop)
{
	auto sharedQuery = std::make_shared<TestQuery>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		QueryType::Generic);

	auto blueTopQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(sharedQuery);
	queries.addQuery(blueTopQuery);

	queries.popQuery(*sharedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), nullptr);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), blueTopQuery);
	EXPECT_EQ(queries.countQuery(sharedQuery), 1);
	EXPECT_EQ(sharedQuery->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(0)}));
}

TEST_F(QueriesProcessorTest, popQuery_removesRemainingMultiPlayerQueryAfterItBecomesTop)
{
	auto sharedQuery = std::make_shared<TestQuery>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		QueryType::Generic);

	auto blueTopQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	queries.addQuery(sharedQuery);
	queries.addQuery(blueTopQuery);

	queries.popQuery(*sharedQuery);
	queries.popIfTop(blueTopQuery);
	queries.popQuery(*sharedQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), nullptr);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
	EXPECT_EQ(queries.countQuery(sharedQuery), 0);
	EXPECT_EQ(sharedQuery->onRemovalCalls, std::vector<PlayerColor>({
		PlayerColor(0),
		PlayerColor(1)
	}));
}

TEST_F(QueriesProcessorTest, popIfTop_callsRemovalBeforeExposure)
{
	std::vector<RecordedEvent> eventLog;

	auto bottomQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto topQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	bottomQuery->sharedEventLog = &eventLog;
	topQuery->sharedEventLog = &eventLog;

	queries.addQuery(bottomQuery);
	queries.addQuery(topQuery);

	eventLog.clear();

	queries.popIfTop(topQuery);

	ASSERT_EQ(eventLog.size(), 2);
	EXPECT_EQ(eventLog[0].query, topQuery.get());
	EXPECT_EQ(eventLog[0].event, QueryEvent::OnRemoval);
	EXPECT_EQ(eventLog[1].query, bottomQuery.get());
	EXPECT_EQ(eventLog[1].event, QueryEvent::OnExposure);
}

TEST_F(QueriesProcessorTest, popQuery_callsRemovalBeforeExposure)
{
	std::vector<RecordedEvent> eventLog;

	auto bottomQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto topQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);

	bottomQuery->sharedEventLog = &eventLog;
	topQuery->sharedEventLog = &eventLog;

	queries.addQuery(bottomQuery);
	queries.addQuery(topQuery);

	eventLog.clear();

	queries.popQuery(*topQuery);

	ASSERT_EQ(eventLog.size(), 2);
	EXPECT_EQ(eventLog[0].query, topQuery.get());
	EXPECT_EQ(eventLog[0].event, QueryEvent::OnRemoval);
	EXPECT_EQ(eventLog[1].query, bottomQuery.get());
	EXPECT_EQ(eventLog[1].event, QueryEvent::OnExposure);
}

TEST_F(QueriesProcessorTest, popIfTop_skipsExposureWhenRemovalAddsNewTopQuery)
{
	auto bottomQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);
	auto topQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::MapObjectVisit);
	auto replacementQuery = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::Generic);

	topQuery->addReplacementOnRemoval = true;
	topQuery->replacementQuery = replacementQuery;

	queries.addQuery(bottomQuery);
	queries.addQuery(topQuery);

	queries.popIfTop(topQuery);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), replacementQuery);
	EXPECT_EQ(queries.countQuery(bottomQuery), 1);
	EXPECT_EQ(queries.countQuery(topQuery), 0);
	EXPECT_EQ(queries.countQuery(replacementQuery), 1);

	EXPECT_EQ(bottomQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded
	}));
	EXPECT_TRUE(bottomQuery->exposureArgs.empty());

	EXPECT_EQ(topQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded,
		QueryEvent::OnRemoval
	}));

	EXPECT_EQ(replacementQuery->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded
	}));
}

TEST_F(QueriesProcessorTest, addQuery_addsSameQueryForAllAffectedPlayers)
{
	auto query = std::make_shared<TestQuery>(&gh, std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)}, QueryType::Generic);

	queries.addQuery(query);

	EXPECT_EQ(queries.topQuery(PlayerColor(0)), query);
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), query);
	EXPECT_EQ(queries.countQuery(query), 2);

	EXPECT_EQ(query->events, std::vector<QueryEvent>({
	QueryEvent::OnAdding,
	QueryEvent::OnAdded,
	QueryEvent::OnAdding,
	QueryEvent::OnAdded
	}));

	EXPECT_EQ(query->onAddingCalls, std::vector<PlayerColor>({PlayerColor(0), PlayerColor(1)}));
	EXPECT_EQ(query->onAddedCalls, std::vector<PlayerColor>({PlayerColor(0), PlayerColor(1)}));
	EXPECT_TRUE(query->onRemovalCalls.empty());
	EXPECT_TRUE(query->exposureArgs.empty());
}

TEST_F(QueriesProcessorTest, addQuery_skipsDuplicateBackWithoutRepeatingOnAdded)
{
	auto query = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);

	queries.addQuery(query);
	queries.addQuery(query);

	EXPECT_EQ(queries.topQuery(PlayerColor(1)), query);
	EXPECT_EQ(queries.countQuery(query), 1);

	EXPECT_EQ(query->events, std::vector<QueryEvent>({
		QueryEvent::OnAdding,
		QueryEvent::OnAdded
	}));

	EXPECT_EQ(query->onAddingCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_EQ(query->onAddedCalls, std::vector<PlayerColor>({PlayerColor(1)}));
}

TEST_F(QueriesProcessorTest, getQuery_returnsNullForUnknownQueryId)
{
	auto query = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);

	queries.addQuery(query);

	EXPECT_EQ(queries.getQuery(QueryID(query->queryID.getNum() + 1)), nullptr);
}

TEST_F(QueriesProcessorTest, getQuery_returnsAddedQueryAndNullAfterRemoval)
{
	auto query = std::make_shared<TestQuery>(&gh, PlayerColor(1), QueryType::HeroMovement);

	queries.addQuery(query);

	EXPECT_EQ(queries.getQuery(query->queryID), query);

	queries.popIfTop(query);

	EXPECT_EQ(queries.getQuery(query->queryID), nullptr);
}

TEST_F(QueriesProcessorTest, countQuery_returnsZeroForNullptr)
{
	EXPECT_EQ(queries.countQuery(nullptr), 0);
}

