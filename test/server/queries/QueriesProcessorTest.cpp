#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../../server/battles/BattleProcessor.h"
#include "../../../server/queries/BattleQueries.h"
#include "../../../server/queries/CQuery.h"
#include "../../../server/queries/MapQueries.h"
#include "../../../server/queries/QueriesProcessor.h"
#include "CGameHandler.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyH3MBuilder.h"
#include "mock/TinyMapGameTest.h"

#include "lib/CPlayerState.h"
#include "lib/battle/BattleInfo.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGDwelling.h"
#include "lib/mapObjects/CGResource.h"
#include "lib/mapObjects/CGCreature.h"
#include "lib/mapObjects/CGPandoraBox.h"
#include "lib/bonuses/Bonus.h"
#include "lib/CPlayerState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"

namespace
{

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

	TestQuery(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, QueryType type)
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
	std::function<void()> onRemovalAction;

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

		if(onRemovalAction)
			onRemovalAction();

		if(sharedEventLog)
			sharedEventLog->push_back({this, QueryEvent::OnRemoval});
	}

	bool blocksPack(const CPackForServer * pack) const override
	{
		// Mirrors VisitQuery: everything is blocked except answering a question.
		if(getType() == QueryType::MapObjectVisit)
			return blockAllButReply(pack);

		return CQuery::blocksPack(pack);
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

/// A query that a player reply can end, so that reply routing can be tested without
/// standing up a dialog query and the netpack traffic that goes with it.
class TestDialogQuery : public CQuery
{
public:
	TestDialogQuery(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, QueryType type)
		: CQuery(gh, type)
	{
		for(auto player : affectedPlayers)
			players.push_back(player);
	}

	TestDialogQuery(CGameHandler * gh, PlayerColor player, QueryType type)
		: CQuery(gh, type)
	{
		players.push_back(player);
	}

	std::optional<int32_t> receivedReply;
	int setReplyCalls = 0;
	int onRemovalCalls = 0;

	bool endsByPlayerAnswer() const override { return true; }

	void setReply(std::optional<int32_t> reply) override
	{
		receivedReply = reply;
		setReplyCalls++;
	}

	void onRemoval(PlayerColor color) override
	{
		onRemovalCalls++;
	}
};

/// A routine that runs a fixed number of steps and can push a child query on a
/// chosen one, so that suspension and resumption can be driven deterministically.
class TestRoutine : public CQuery, public IRoutine
{
public:
	TestRoutine(CGameHandler * gh, PlayerColor player, int totalSteps)
		: CQuery(gh, QueryType::MapObjectVisit)
		, totalSteps(totalSteps)
	{
		players.push_back(player);
	}

	int totalSteps;
	int stepsTaken = 0;

	/// Step index on which to push childToPush, or -1 to never push.
	int pushChildOnStep = -1;
	QueryPtr childToPush;

	std::vector<QueryPtr> completedChildren;
	/// Step indices at which advance() was entered, to check resumption order.
	std::vector<int> stepLog;

	IRoutine * asRoutine() final { return this; }

	StepResult advance() final
	{
		if(stepsTaken >= totalSteps)
			return StepResult::Done;

		stepLog.push_back(stepsTaken);
		const int currentStep = stepsTaken++;

		if(currentStep == pushChildOnStep && childToPush)
			owner->addQuery(childToPush);

		return StepResult::Continue;
	}

	void onChildCompleted(const QueryPtr & child) final
	{
		completedChildren.push_back(child);
	}
};

/// A QueryReply pack from a given player, for checking what blocksPack() lets past.
inline const QueryReply & replyFromPlayer(PlayerColor player)
{
	static QueryReply reply;
	reply.player = player;
	return reply;
}

class QueriesProcessorTest : public ::testing::Test
{
protected:
	std::shared_ptr<CGameState> gameState = std::make_shared<CGameState>();
	GameHandlerTestServer server{gameState};
	CGameHandler gh{server, gameState};
	QueriesProcessor & queries = *gh.queries;
};

class NeutralDwellingBattleQueryTest : public TinyMapGameTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
			.heroGarrison({{CreatureID(0), 10}})
			.dwelling({7, 5, 0}, MapObjectSubID(0), PlayerColor(1));

		startWithMap(std::move(builder));
	}
};

class DeferredVictoryLossTest : public TinyMapGameTest
{
};

}

TEST_F(QueriesProcessorTest, topQuery_returnsNullWhenPlayerHasNoQueries)
{
	EXPECT_EQ(queries.topQuery(PlayerColor(1)), nullptr);
}

TEST_F(NeutralDwellingBattleQueryTest, ownedDwellingUsesNeutralBattleSideWithoutNeutralQuery)
{
	startGame();

	auto * hero = findHeroByOwner(PlayerColor(0));
	auto * dwelling = findFirst<CGDwelling>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(dwelling, nullptr);
	ASSERT_TRUE(dwelling->setCreature(SlotID(0), CreatureID(1), 10));

	GameHandlerTestServer server(gameState());
	CGameHandler gh(server, gameState());

	gh.battles->startBattle(hero, dwelling);

	const auto * battle = gameState()->getBattle(PlayerColor(0));
	ASSERT_NE(battle, nullptr);
	EXPECT_EQ(battle->getSide(BattleSide::DEFENDER).color, PlayerColor::NEUTRAL);

	const auto attackerQuery = gh.queries->topQuery(PlayerColor(0));
	ASSERT_NE(attackerQuery, nullptr);
	EXPECT_EQ(attackerQuery->getType(), QueryType::Battle);
	ASSERT_EQ(attackerQuery->players.size(), 1);
	EXPECT_EQ(attackerQuery->players.front(), PlayerColor(0));
	EXPECT_EQ(gh.queries->topQuery(PlayerColor(1)), nullptr);
}

TEST_F(DeferredVictoryLossTest, heroLevelUpDefersVictoryUntilQueryIsAnswered)
{
	const PlayerColor defeatedPlayer(0);
	const PlayerColor levelUpPlayer(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false);
	builder.playerActive(defeatedPlayer);
	builder.playerActive(levelUpPlayer);
	builder.hero(int3(5, 5, 0), HeroTypeID(0), levelUpPlayer);
	builder.heroExperience(999); // one experience point short of the next level
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(levelUpPlayer);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), levelUpPlayer);
	CGameHandler gameHandler(server, gameState());

	// The level-up has to exist before the other player is eliminated, otherwise
	// nothing is deferring anything and this would test the empty case.
	gameHandler.giveExperience(hero, 10);
	gameHandler.onAdvInterfaceReady(levelUpPlayer);

	auto levelUpQuery = gameHandler.queries->topQuery(levelUpPlayer);
	ASSERT_NE(levelUpQuery, nullptr);
	ASSERT_EQ(levelUpQuery->getType(), QueryType::HeroLevelUpDialog);

	// The player is mid-level-up, so winning is not announced yet.
	gameHandler.checkVictoryLossConditionsForPlayer(defeatedPlayer);
	EXPECT_EQ(gameState()->getPlayerState(defeatedPlayer)->status, EPlayerStatus::LOSER);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::INGAME);

	ASSERT_EQ(gameHandler.queries->submitReply(levelUpQuery->getActiveQuestionID(), levelUpPlayer, 0),
		ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.queries->topQuery(levelUpPlayer), nullptr);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::WINNER);
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


// --------------------------------------------------------------------------------
// Reply routing.
//
// The client is prompted for a query and answers it, but the server may push
// something else in between. These tests drive the processor through those
// orderings directly, because that is the shape of the client/server race that
// used to leave a player holding a query that had already been answered.
// --------------------------------------------------------------------------------

TEST_F(QueriesProcessorTest, submitReply_resolvesTopQuery)
{
	const PlayerColor player(1);
	auto query = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	queries.addQuery(query);

	EXPECT_EQ(queries.submitReply(query->queryID, player, 7), ReplyOutcome::Accepted);

	EXPECT_EQ(queries.topQuery(player), nullptr);
	EXPECT_EQ(query->receivedReply, std::optional<int32_t>(7));
	EXPECT_EQ(query->onRemovalCalls, 1);
}

TEST_F(QueriesProcessorTest, submitReply_acceptsReplyForBuriedQueryAndResolvesItOnceExposed)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	auto pushedAfterPrompt = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);

	queries.addQuery(dialog);
	// Server pushes something else after the dialog was sent to the client, but
	// before the client's answer arrives.
	queries.addQuery(pushedAfterPrompt);

	EXPECT_EQ(queries.submitReply(dialog->queryID, player, 3), ReplyOutcome::Accepted);

	// The dialog is answered but still buried, so it stays put for now.
	EXPECT_EQ(queries.topQuery(player), pushedAfterPrompt);
	EXPECT_TRUE(dialog->isAnswered());
	EXPECT_EQ(dialog->onRemovalCalls, 0);

	queries.popIfTop(pushedAfterPrompt);

	// Exposing it must resolve it rather than leave the player waiting forever.
	EXPECT_EQ(queries.topQuery(player), nullptr);
	EXPECT_EQ(dialog->receivedReply, std::optional<int32_t>(3));
	EXPECT_EQ(dialog->onRemovalCalls, 1);
}

TEST_F(QueriesProcessorTest, submitReply_resolvesSeveralStackedQueriesAnsweredOutOfOrder)
{
	const PlayerColor player(1);
	auto bottom = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	auto middle = std::make_shared<TestDialogQuery>(&gh, player, QueryType::TeleportDialog);
	auto top = std::make_shared<TestDialogQuery>(&gh, player, QueryType::GarrisonDialog);

	queries.addQuery(bottom);
	queries.addQuery(middle);
	queries.addQuery(top);

	// Answers arrive bottom-up - the exact opposite of the stack order.
	EXPECT_EQ(queries.submitReply(bottom->queryID, player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(queries.submitReply(middle->queryID, player, 2), ReplyOutcome::Accepted);
	EXPECT_EQ(queries.topQuery(player), top);

	// Answering the top must unwind all three, not just one.
	EXPECT_EQ(queries.submitReply(top->queryID, player, 3), ReplyOutcome::Accepted);

	EXPECT_EQ(queries.topQuery(player), nullptr);
	EXPECT_EQ(bottom->onRemovalCalls, 1);
	EXPECT_EQ(middle->onRemovalCalls, 1);
	EXPECT_EQ(top->onRemovalCalls, 1);
}

TEST_F(QueriesProcessorTest, submitReply_ignoresDuplicateReply)
{
	const PlayerColor player(1);
	auto query = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	auto blocker = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);

	queries.addQuery(query);
	queries.addQuery(blocker);

	EXPECT_EQ(queries.submitReply(query->queryID, player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(queries.submitReply(query->queryID, player, 2), ReplyOutcome::IgnoredAlreadyAnswered);

	// The second answer must not overwrite the first.
	EXPECT_EQ(query->setReplyCalls, 1);
	EXPECT_EQ(query->receivedReply, std::optional<int32_t>(1));
}

TEST_F(QueriesProcessorTest, submitReply_ignoresReplyToQueryThatAlreadyCompleted)
{
	const PlayerColor player(1);
	auto query = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	queries.addQuery(query);

	const QueryID queryID = query->queryID;
	queries.popIfTop(query); // removed by some other event while the reply was in flight

	EXPECT_EQ(queries.submitReply(queryID, player, 1), ReplyOutcome::IgnoredAlreadyCompleted);
}

TEST_F(QueriesProcessorTest, submitReply_rejectsUnknownQuery)
{
	const PlayerColor player(1);
	auto query = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	queries.addQuery(query);

	EXPECT_EQ(queries.submitReply(QueryID(12345), player, 1), ReplyOutcome::RejectedUnknownQuery);
}

TEST_F(QueriesProcessorTest, submitReply_rejectsReplyFromPlayerNotAffectedByQuery)
{
	const PlayerColor owner(1);
	const PlayerColor other(2);
	auto query = std::make_shared<TestDialogQuery>(&gh, owner, QueryType::BlockingDialog);
	queries.addQuery(query);

	EXPECT_EQ(queries.submitReply(query->queryID, other, 1), ReplyOutcome::RejectedWrongPlayer);
	EXPECT_FALSE(query->isAnswered());
	EXPECT_EQ(queries.topQuery(owner), query);
}

TEST_F(QueriesProcessorTest, submitReply_rejectsQueryThatCannotBeEndedByAnswer)
{
	const PlayerColor player(1);
	auto query = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);
	queries.addQuery(query);

	EXPECT_EQ(queries.submitReply(query->queryID, player, 1), ReplyOutcome::RejectedNotAnswerable);
	EXPECT_EQ(queries.topQuery(player), query);
}

TEST_F(QueriesProcessorTest, getQuery_scopedByPlayerDistinguishesSharedQueryIds)
{
	// QueryID::CLIENT is used by every pause query, so two players can legitimately
	// hold different queries carrying the same ID at the same time.
	const PlayerColor first(1);
	const PlayerColor second(2);

	auto firstQuery = std::make_shared<TestDialogQuery>(&gh, first, QueryType::TimerPause);
	auto secondQuery = std::make_shared<TestDialogQuery>(&gh, second, QueryType::TimerPause);
	firstQuery->queryID = QueryID::CLIENT;
	secondQuery->queryID = QueryID::CLIENT;

	queries.addQuery(firstQuery);
	queries.addQuery(secondQuery);

	EXPECT_EQ(queries.getQuery(QueryID::CLIENT, first), firstQuery);
	EXPECT_EQ(queries.getQuery(QueryID::CLIENT, second), secondQuery);

	// A reply must land on the replying player's own query.
	EXPECT_EQ(queries.submitReply(QueryID::CLIENT, second, 0), ReplyOutcome::Accepted);
	EXPECT_FALSE(firstQuery->isAnswered());
	EXPECT_EQ(queries.topQuery(first), firstQuery);
	EXPECT_EQ(queries.topQuery(second), nullptr);
}

TEST_F(QueriesProcessorTest, submitReply_sharedQueryIsRemovedFromEveryAffectedPlayer)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto shared = std::make_shared<TestDialogQuery>(&gh, std::vector<PlayerColor>{first, second}, QueryType::BattleDialog);

	queries.addQuery(shared);
	ASSERT_EQ(queries.countQuery(shared), 2);

	EXPECT_EQ(queries.submitReply(shared->queryID, first, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(queries.countQuery(shared), 0);
	EXPECT_EQ(queries.topQuery(first), nullptr);
	EXPECT_EQ(queries.topQuery(second), nullptr);
}

TEST_F(QueriesProcessorTest, submitReply_sharedQueryWaitsForPlayerWhoIsStillBusy)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto shared = std::make_shared<TestDialogQuery>(&gh, std::vector<PlayerColor>{first, second}, QueryType::BattleDialog);
	auto busy = std::make_shared<TestQuery>(&gh, second, QueryType::MapObjectVisit);

	queries.addQuery(shared);
	queries.addQuery(busy); // only the second player has something on top of it

	EXPECT_EQ(queries.submitReply(shared->queryID, first, 1), ReplyOutcome::Accepted);

	// Resolved for the player whose stack allows it...
	EXPECT_EQ(queries.topQuery(first), nullptr);
	// ...and still in place for the one who is busy, rather than silently skipped.
	EXPECT_EQ(queries.countQuery(shared), 1);
	EXPECT_EQ(queries.topQuery(second), busy);

	queries.popIfTop(busy);

	EXPECT_EQ(queries.topQuery(second), nullptr);
	EXPECT_EQ(queries.countQuery(shared), 0);
}

// --------------------------------------------------------------------------------
// Property test: no ordering of prompts and replies may leave a player holding a
// query that has already been answered. That invariant is exactly what used to
// fail in practice, and it is not reachable by enumerating cases by hand.
// --------------------------------------------------------------------------------

TEST_F(QueriesProcessorTest, noInterleavingLeavesPlayerHoldingAnAnsweredQuery)
{
	const PlayerColor player(1);

	for(uint32_t seed = 0; seed < 500; ++seed)
	{
		QueriesProcessor processor(gh);
		std::mt19937 rng(seed);
		std::vector<std::shared_ptr<TestDialogQuery>> live;
		std::vector<QueryID> answeredButLive;

		for(int step = 0; step < 40; ++step)
		{
			const bool canReply = !live.empty();
			const int action = rng() % (canReply ? 3 : 1);

			if(action == 0) // server pushes a new query
			{
				auto query = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
				processor.addQuery(query);
				live.push_back(query);
			}
			else if(action == 1) // client replies to some query it was prompted for
			{
				const auto & target = live[rng() % live.size()];
				processor.submitReply(target->queryID, player, 0);
			}
			else // server removes the top query for reasons of its own
			{
				if(auto top = processor.topQuery(player))
					processor.popIfTop(top);
			}

			// Invariant: an answered query is never left sitting at the top.
			if(auto top = processor.topQuery(player))
			{
				ASSERT_FALSE(top->isAnswered())
					<< "seed " << seed << " step " << step
					<< " left an answered query on top:\n" << processor.describeStacks();
			}

			vstd::erase_if(live, [&processor](const std::shared_ptr<TestDialogQuery> & q)
			{
				return processor.countQuery(q) == 0;
			});
		}

		// Draining the stack must always terminate with nothing answered left behind.
		while(auto top = processor.topQuery(player))
		{
			ASSERT_FALSE(top->isAnswered()) << "seed " << seed << ":\n" << processor.describeStacks();
			processor.popIfTop(top);
		}
	}
}


// --------------------------------------------------------------------------------
// Quiescence and queued work.
//
// Removing a query runs hooks that may add or remove further queries, so the stacks
// pass through states that are not meaningful - briefly empty, or holding a query
// that is about to be replaced. Work queued behind whatever the player is doing must
// start from the settled state, never from one of those.
// --------------------------------------------------------------------------------

TEST_F(QueriesProcessorTest, waitingQuery_doesNotStartWhilePlayerIsBusy)
{
	const PlayerColor player(1);
	auto busy = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);
	auto pending = std::make_shared<TestQuery>(&gh, player, QueryType::TurnStartVisit);

	queries.addQuery(busy);
	queries.addQueryWhenIdle(pending);

	EXPECT_EQ(queries.topQuery(player), busy);
	EXPECT_TRUE(pending->onAddedCalls.empty());

	queries.popIfTop(busy);

	EXPECT_EQ(queries.topQuery(player), pending);
	EXPECT_EQ(pending->onAddedCalls.size(), 1u);
}

TEST_F(QueriesProcessorTest, waitingQuery_startsOnceAfterTheStackFullyUnwinds)
{
	const PlayerColor player(1);
	auto bottom = std::make_shared<TestQuery>(&gh, player, QueryType::HeroMovement);
	auto top = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);
	bottom->popOnExposure = true; // exposing it unwinds the rest of the stack

	queries.addQuery(bottom);
	queries.addQuery(top);
	queries.addQueryWhenIdle(std::make_shared<TestQuery>(&gh, player, QueryType::TurnStartVisit));

	// Two removals happen here, but there is only one quiescent point, so the
	// queued query must be started exactly once.
	queries.popIfTop(top);

	auto started = queries.topQuery(player);
	ASSERT_NE(started, nullptr);
	EXPECT_EQ(started->getType(), QueryType::TurnStartVisit);
	EXPECT_EQ(std::dynamic_pointer_cast<TestQuery>(started)->onAddedCalls.size(), 1u);
}

TEST_F(QueriesProcessorTest, waitingQuery_doesNotSlipIntoTheGapOfAReplacementChain)
{
	const PlayerColor player(1);

	// A query that pushes a successor as it is removed - the shape of a level-up
	// chain, where the player is never really idle between the two.
	auto replacement = std::make_shared<TestQuery>(&gh, player, QueryType::HeroLevelUpDialog);
	auto original = std::make_shared<TestQuery>(&gh, player, QueryType::HeroLevelUpDialog);
	original->addReplacementOnRemoval = true;
	original->replacementQuery = replacement;

	auto pending = std::make_shared<TestQuery>(&gh, player, QueryType::TurnStartVisit);

	queries.addQuery(original);
	queries.addQueryWhenIdle(pending);

	queries.popIfTop(original);

	// The queued work must wait for the whole chain, not cut in between its links.
	EXPECT_EQ(queries.topQuery(player), replacement);
	EXPECT_TRUE(pending->onAddedCalls.empty());

	queries.popIfTop(replacement);
	EXPECT_EQ(queries.topQuery(player), pending);
}

TEST_F(QueriesProcessorTest, waitingQuery_sharedByTwoPlayersStartsOnlyWhenBothAreIdle)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto busy = std::make_shared<TestQuery>(&gh, second, QueryType::MapObjectVisit);
	auto pending = std::make_shared<TestQuery>(&gh, std::vector<PlayerColor>{first, second}, QueryType::Battle);

	queries.addQuery(busy);
	queries.addQueryWhenIdle(pending);

	// The first player is idle, but starting now would put the query on one stack
	// and not the other.
	EXPECT_TRUE(pending->onAddedCalls.empty());
	EXPECT_EQ(queries.topQuery(first), nullptr);

	queries.popIfTop(busy);

	EXPECT_EQ(queries.topQuery(first), pending);
	EXPECT_EQ(queries.topQuery(second), pending);
	EXPECT_EQ(queries.countQuery(pending), 2);
}

TEST_F(QueriesProcessorTest, settle_resolvesRepliesThatArrivedWhileStacksWereMoving)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	auto cover = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);

	queries.addQuery(dialog);
	queries.addQuery(cover);

	EXPECT_EQ(queries.submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(queries.topQuery(player), cover);

	// Removing the cover exposes an answered query; settle() must resolve it within
	// the same quiescent point rather than leaving it for some later mutation.
	queries.popIfTop(cover);

	EXPECT_EQ(queries.topQuery(player), nullptr);
	EXPECT_EQ(dialog->onRemovalCalls, 1);
}

TEST_F(QueriesProcessorTest, settle_givesUpInsteadOfLoopingForeverWhenDeferredWorkKeepsChanging)
{
	const PlayerColor player(1);

	// Every removal queues another query, which is started, removed, and queues
	// another... Must terminate rather than spin or recurse until the stack blows.
	std::function<void()> queueAnother = [&]()
	{
		auto next = std::make_shared<TestQuery>(&gh, player, QueryType::TurnStartVisit);
		next->onRemovalAction = queueAnother;
		queries.addQueryWhenIdle(next);
		queries.popIfTop(next);
	};

	auto first = std::make_shared<TestQuery>(&gh, player, QueryType::TurnStartVisit);
	first->onRemovalAction = queueAnother;

	queries.addQuery(first);
	queries.popIfTop(first);

	SUCCEED() << "settle() terminated instead of looping forever";
}

// --------------------------------------------------------------------------------
// Routines.
//
// A routine is a multi-step server-side activity - visiting the buildings of a town,
// visiting an object - that must be able to stop in the middle when a step needs the
// player, and carry on afterwards from where it left off. The processor drives it;
// the routine keeps its own position rather than inferring it from the stack.
// --------------------------------------------------------------------------------

TEST_F(QueriesProcessorTest, routine_isSteppedToCompletionAndThenRemoved)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);

	queries.addQuery(routine);

	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(routine->stepLog, std::vector<int>({0, 1, 2}));
	EXPECT_EQ(queries.topQuery(player), nullptr);
}

TEST_F(QueriesProcessorTest, routine_suspendsWhenAStepPushesAChild)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 4);
	auto child = std::make_shared<TestQuery>(&gh, player, QueryType::BlockingDialog);
	routine->pushChildOnStep = 1;
	routine->childToPush = child;

	queries.addQuery(routine);

	// Stopped on the step that pushed the child, with the child on top.
	EXPECT_EQ(routine->stepsTaken, 2);
	EXPECT_EQ(queries.topQuery(player), child);
}

TEST_F(QueriesProcessorTest, routine_resumesFromWhereItStoppedOnceTheChildFinishes)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 4);
	auto child = std::make_shared<TestQuery>(&gh, player, QueryType::BlockingDialog);
	routine->pushChildOnStep = 1;
	routine->childToPush = child;

	queries.addQuery(routine);
	ASSERT_EQ(routine->stepsTaken, 2);

	queries.popIfTop(child);

	// Carries on from step 2 - it does not restart, and does not skip a step.
	EXPECT_EQ(routine->stepLog, std::vector<int>({0, 1, 2, 3}));
	EXPECT_EQ(queries.topQuery(player), nullptr);

	ASSERT_EQ(routine->completedChildren.size(), 1u);
	EXPECT_EQ(routine->completedChildren.front(), child);
}

TEST_F(QueriesProcessorTest, routine_doesNotReceiveTheGenericExposureHook)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 2);
	auto child = std::make_shared<TestQuery>(&gh, player, QueryType::BlockingDialog);
	routine->pushChildOnStep = 0;
	routine->childToPush = child;

	queries.addQuery(routine);
	queries.popIfTop(child);

	// Child completion is reported through onChildCompleted only, so a routine
	// cannot accidentally implement resumption twice.
	EXPECT_EQ(routine->completedChildren.size(), 1u);
}

TEST_F(QueriesProcessorTest, routine_waitsForAChildThatPushesAChildOfItsOwn)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);
	auto child = std::make_shared<TestQuery>(&gh, player, QueryType::BlockingDialog);
	auto grandchild = std::make_shared<TestQuery>(&gh, player, QueryType::Battle);
	routine->pushChildOnStep = 0;
	routine->childToPush = child;

	queries.addQuery(routine);
	ASSERT_EQ(queries.topQuery(player), child);

	queries.addQuery(grandchild);
	EXPECT_EQ(routine->stepsTaken, 1); // still suspended, two levels down now

	queries.popIfTop(grandchild);
	EXPECT_EQ(routine->stepsTaken, 1); // the child is still unfinished

	queries.popIfTop(child);
	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(queries.topQuery(player), nullptr);
}

TEST_F(QueriesProcessorTest, routine_underneathAnAnsweredQueryResumesAfterItResolves)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);
	auto dialog = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	routine->pushChildOnStep = 0;
	routine->childToPush = dialog;

	queries.addQuery(routine);
	ASSERT_EQ(queries.topQuery(player), dialog);

	// Answering the dialog must both resolve it and let the routine continue,
	// within the same quiescent point.
	EXPECT_EQ(queries.submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(queries.topQuery(player), nullptr);
}

// --------------------------------------------------------------------------------
// Object visits driven end to end.
//
// The visit routine hands control to the object, which may finish immediately or
// start a battle. These exercise the real pipeline rather than a stand-in routine.
// --------------------------------------------------------------------------------

namespace
{
class MapObjectVisitTest : public TinyMapGameTest
{
};
}

TEST_F(MapObjectVisitTest, unguardedVisitFinishesAndLeavesNothingOnTheStack)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.resource(int3(6, 5, 0), GameResID(GameResID::GOLD), 1000);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	const auto goldBefore = gameState()->getPlayerState(player)->resources[GameResID::GOLD];
	const auto resourceID = resource->id;

	gameHandler.objectVisited(resource, hero);

	// The whole visit runs within the call: reward granted, object gone, no query left.
	EXPECT_GT(gameState()->getPlayerState(player)->resources[GameResID::GOLD], goldBefore);
	EXPECT_EQ(gameState()->getObjInstance(resourceID), nullptr);
	EXPECT_EQ(gameHandler.queries->topQuery(player), nullptr);
}

TEST_F(MapObjectVisitTest, visitResumesAndCompletesAfterTheDialogItStarted)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.pandora(int3(6, 5, 0));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	const auto pandoraID = pandora->id;

	gameHandler.objectVisited(pandora, hero);

	// The object opens a dialog, so the visit suspends rather than finishing.
	auto dialog = gameHandler.queries->topQuery(player);
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(dialog->getType(), QueryType::BlockingDialog);
	EXPECT_NE(gameHandler.getVisitingHero(pandora), nullptr);

	// Answering resumes the visit, which then runs to the end and unwinds fully.
	ASSERT_EQ(gameHandler.queries->submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.queries->topQuery(player), nullptr);
	EXPECT_EQ(gameState()->getObjInstance(pandoraID), nullptr);
}

TEST_F(MapObjectVisitTest, visitStaysSuspendedAcrossAChainOfChildQueries)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID(0), 200}})
		.pandora(int3(6, 5, 0));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

	// Guards make the visit go dialog -> battle, with no die roll deciding either.
	ASSERT_TRUE(pandora->setCreature(SlotID(0), CreatureID(0), 1));

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	auto visitIsPending = [&]()
	{
		for(const auto & query : gameHandler.queries->allQueries())
			if(query->getType() == QueryType::MapObjectVisit)
				return true;
		return false;
	};

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.queries->topQuery(player);
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(dialog->getType(), QueryType::BlockingDialog);
	EXPECT_TRUE(visitIsPending());
	EXPECT_EQ(gameHandler.getVisitingHero(pandora), hero);

	// Answering starts a battle one level deeper. The visit must still be waiting
	// underneath it, so the object can be told the result when the battle ends.
	ASSERT_EQ(gameHandler.queries->submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);

	auto battle = gameHandler.queries->topQuery(player);
	ASSERT_NE(battle, nullptr);
	EXPECT_EQ(battle->getType(), QueryType::Battle);
	EXPECT_TRUE(visitIsPending());
	EXPECT_EQ(gameHandler.getVisitingHero(pandora), hero);
}

TEST_F(MapObjectVisitTest, visitIsRefusedWhileAnotherHeroIsVisitingTheSameObject)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID(0), 200}})
		.pandora(int3(6, 5, 0));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);
	ASSERT_NE(gameHandler.getVisitingHero(pandora), nullptr);

	// The visit query must be findable on the stack for the whole visit - object code
	// relies on that through removeAfterVisit() and isVisitCoveredByAnotherQuery().
	EXPECT_THROW(gameHandler.objectVisited(pandora, hero), std::runtime_error);
}

TEST_F(MapObjectVisitTest, levelUpFromBattleExperienceDoesNotGrantTheObjectRewardAgain)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID(0), 200}})
		.heroExperience(999) // one experience point short of the next level
		.pandora(int3(6, 5, 0));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

	// A reward granted in the after-level-up half of the pipeline, so that granting
	// it twice is visible, plus guards so the visit has to go through a battle.
	ASSERT_FALSE(pandora->configuration.info.empty());
	pandora->configuration.info.at(0).reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));
	ASSERT_TRUE(pandora->setCreature(SlotID(0), CreatureID(0), 1));

	auto rewardsGranted = [&]()
	{
		return hero->getBonuses([](const Bonus * b)
		{
			return b->type == BonusType::MORALE && b->source == BonusSource::OBJECT_TYPE;
		})->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	// Level-up prompts are only sent once the client's interface is ready, and the
	// hero's level is applied by that pack - without this the hero never levels up.
	gameHandler.onAdvInterfaceReady(player);

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.queries->topQuery(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), QueryType::BlockingDialog);
	ASSERT_EQ(gameHandler.queries->submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);

	ASSERT_EQ(gameHandler.queries->topQuery(player)->getType(), QueryType::Battle);
	gameHandler.battles->cheatBattleVictory(player);

	// Accept the result rather than replaying the battle.
	auto resultDialog = gameHandler.queries->topQuery(player);
	ASSERT_NE(resultDialog, nullptr);
	ASSERT_EQ(resultDialog->getType(), QueryType::BattleDialog);
	ASSERT_EQ(gameHandler.queries->submitReply(resultDialog->queryID, player, 0), ReplyOutcome::Accepted);

	// The object has applied the battle result and granted its reward once. The
	// level-up earned from battle experience is only now offered, on top of the visit.
	ASSERT_EQ(rewardsGranted(), 1u);
	auto levelUp = gameHandler.queries->topQuery(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), QueryType::HeroLevelUpDialog);

	// The hero may gain several levels at once, each prompting in turn.
	int levelUpsAnswered = 0;
	while(auto pending = gameHandler.queries->topQuery(player))
	{
		if(pending->getType() != QueryType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.queries->submitReply(pending->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++levelUpsAnswered, 10) << "level-up chain did not terminate";
	}

	EXPECT_GE(levelUpsAnswered, 1);

	// None of those level-ups is part of the object's reward pipeline, so the object
	// must not be told about them - otherwise heroLevelUpDone() grants the reward
	// once more for each one.
	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.queries->topQuery(player), nullptr);
}

// --------------------------------------------------------------------------------
// Locating the battle query.
//
// A battle query is one object on both belligerents' stacks. Most callers look at
// the attacker first and fall back to the defender, but they disagree on whether an
// AI defender counts - a difference that used to be spelled out four times over.
// --------------------------------------------------------------------------------

namespace
{
class TwoPlayerBattleTest : public TinyMapGameTest
{
protected:
	/// Colour that is played by the computer rather than a person.
	static constexpr int AI_PLAYER = 1;

	void configurePlayer(PlayerSettings & settings) const override
	{
		if(settings.color == PlayerColor(AI_PLAYER))
			settings.connectedPlayerIDs.clear(); // a player with no connection is an AI
	}

	void buildTwoPlayerMap()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero(int3(5, 5, 0), HeroTypeID(0), PlayerColor(0))
			.heroGarrison({{CreatureID(0), 10}})
			.hero(int3(8, 8, 0), HeroTypeID(1), PlayerColor(1))
			.heroGarrison({{CreatureID(0), 10}});
		startWithMap(std::move(builder));
	}
};
}

TEST_F(TwoPlayerBattleTest, findBattleQueryLooksAtBothSidesForTheOneSharedQuery)
{
	buildTwoPlayerMap();

	auto * attacker = findHeroByOwner(PlayerColor(0));
	auto * defender = findHeroByOwner(PlayerColor(AI_PLAYER));
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	GameHandlerTestServer server(gameState(), PlayerColor(0));
	CGameHandler gameHandler(server, gameState());

	gameHandler.battles->startBattle(attacker, defender);
	const auto * battle = gameState()->getBattle(PlayerColor(0));
	ASSERT_NE(battle, nullptr);

	auto * found = gameHandler.battles->findBattleQuery(*battle);
	ASSERT_NE(found, nullptr);

	// One object, on both belligerents' stacks - and only one per player.
	EXPECT_EQ(gameHandler.queries->findSoleQuery<CBattleQuery>(PlayerColor(0)), found);
	EXPECT_EQ(gameHandler.queries->findSoleQuery<CBattleQuery>(PlayerColor(AI_PLAYER)), found);
}

TEST_F(MapObjectVisitTest, visitByAMonsterThatAlwaysFightsSuspendsDirectlyUnderTheBattle)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID(0), 1}})
		// Savage is the one disposition with a fixed aggression rather than a rolled
		// one, and a hero this weak cannot talk its way out, so the monster always
		// fights and the outcome does not depend on the die.
		.monster(int3(6, 5, 0), CreatureID(0), 100,
			static_cast<int8_t>(CGCreature::Character::SAVAGE));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * monster = findFirst<CGCreature>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(monster, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(monster, hero);

	// No parley, so the battle covers the visit straight away.
	auto top = gameHandler.queries->topQuery(player);
	ASSERT_NE(top, nullptr);
	EXPECT_EQ(top->getType(), QueryType::Battle);
	EXPECT_EQ(gameHandler.getVisitingHero(monster), hero);
}

// --------------------------------------------------------------------------------
// Repeated questions.
//
// A hero can earn several levels from one reward, and used to be asked about each
// through a separate query pushed as the previous one was removed. It is now one
// query that asks repeatedly, so the player is never briefly free between levels
// and whatever waits underneath is told once, at the end.
// --------------------------------------------------------------------------------

namespace
{
class LevelUpQueryTest : public TinyMapGameTest
{
protected:
	void buildHeroAboutToLevel(uint32_t experience)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PlayerColor(0))
			.hero(int3(5, 5, 0), HeroTypeID(0), PlayerColor(0))
			.heroExperience(experience);
		startWithMap(std::move(builder));
	}
};
}

TEST_F(LevelUpQueryTest, severalLevelsAreAskedAboutByOneQueryThatStaysOnTheStack)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	gameHandler.onAdvInterfaceReady(player);

	const int levelBefore = hero->level;
	gameHandler.giveExperience(hero, 100000); // worth several levels at once

	auto query = gameHandler.queries->topQuery(player);
	ASSERT_NE(query, nullptr);
	ASSERT_EQ(query->getType(), QueryType::HeroLevelUpDialog);

	std::set<QueryID> questionsAsked;
	int answers = 0;

	while(auto pending = gameHandler.queries->topQuery(player))
	{
		// Always the same query object, however many times it asks.
		ASSERT_EQ(pending, query) << "a second query was pushed instead of asking again";

		const auto questionID = pending->getActiveQuestionID();
		EXPECT_TRUE(questionsAsked.insert(questionID).second) << "a question id was reused";

		ASSERT_EQ(gameHandler.queries->submitReply(questionID, player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++answers, 50) << "level-up sequence did not terminate";
	}

	EXPECT_GT(answers, 1) << "expected more than one level from this much experience";
	EXPECT_GT(hero->level, levelBefore + 1);
	EXPECT_EQ(gameHandler.queries->topQuery(player), nullptr);
}

TEST_F(LevelUpQueryTest, answerNamingASupersededQuestionIsIgnored)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	gameHandler.onAdvInterfaceReady(player);

	gameHandler.giveExperience(hero, 100000);

	auto query = gameHandler.queries->topQuery(player);
	ASSERT_NE(query, nullptr);

	const auto firstQuestion = query->getActiveQuestionID();
	ASSERT_EQ(gameHandler.queries->submitReply(firstQuestion, player, 0), ReplyOutcome::Accepted);

	// The next question is now outstanding; the previous one is history.
	ASSERT_EQ(gameHandler.queries->topQuery(player), query);
	ASSERT_NE(query->getActiveQuestionID(), firstQuestion);

	EXPECT_EQ(gameHandler.queries->submitReply(firstQuestion, player, 0),
		ReplyOutcome::IgnoredAlreadyCompleted);

	// Naming the query rather than the question is not good enough either.
	EXPECT_EQ(gameHandler.queries->submitReply(query->queryID, player, 0),
		ReplyOutcome::IgnoredAlreadyCompleted);

	// Neither stale answer consumed the outstanding question.
	EXPECT_EQ(gameHandler.queries->topQuery(player), query);
}

TEST_F(LevelUpQueryTest, nothingIsAskedBeforeThePlayersInterfaceIsReady)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	// Deliberately no onAdvInterfaceReady().
	gameHandler.giveExperience(hero, 10);

	auto query = gameHandler.queries->topQuery(player);
	ASSERT_NE(query, nullptr);
	EXPECT_FALSE(query->hasOutstandingQuestion()) << "asked before the client could show it";

	// The level is applied by the dialog pack, so it is still pending too.
	EXPECT_TRUE(hero->gainsLevel());

	gameHandler.onAdvInterfaceReady(player);

	EXPECT_TRUE(query->hasOutstandingQuestion());
	EXPECT_FALSE(hero->gainsLevel());
}

TEST_F(LevelUpQueryTest, everyQuestionSentToTheClientIsReportedResolved)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	gameHandler.onAdvInterfaceReady(player);

	gameHandler.giveExperience(hero, 100000); // worth several levels at once

	while(auto pending = gameHandler.queries->topQuery(player))
	{
		ASSERT_EQ(gameHandler.queries->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
	}

	// The client keeps a query-backed dialog open until the server reports that
	// question resolved, so every prompt sent has to come back resolved - by the id
	// the client was given, not by the id of the query behind it.
	ASSERT_GT(server.levelUpPromptIDs.size(), 1u) << "expected several levels";
	EXPECT_EQ(server.resolvedQueryIDs, server.levelUpPromptIDs);
}

TEST_F(TwoPlayerBattleTest, battleQueryIsStillFoundWhenThePlayerPausesMidBattle)
{
	buildTwoPlayerMap();

	auto * attacker = findHeroByOwner(PlayerColor(0));
	auto * defender = findHeroByOwner(PlayerColor(AI_PLAYER));
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	GameHandlerTestServer server(gameState(), PlayerColor(0));
	CGameHandler gameHandler(server, gameState());

	gameHandler.battles->startBattle(attacker, defender);
	const auto * battle = gameState()->getBattle(PlayerColor(0));
	ASSERT_NE(battle, nullptr);

	auto * expected = gameHandler.battles->findBattleQuery(*battle);
	ASSERT_NE(expected, nullptr);

	// CBattleQuery lets GamePause through, so a player may pause mid-battle, which
	// puts a TimerPauseQuery on top of the battle query. That is legal, and the
	// battle query must still be found - looking only at the top of the stack lost
	// it, and the battle then ended with "Cannot find battle query!".
	auto pause = std::make_shared<TimerPauseQuery>(&gameHandler, PlayerColor(0));
	gameHandler.queries->addQuery(pause);
	ASSERT_EQ(gameHandler.queries->topQuery(PlayerColor(0)), pause);

	EXPECT_EQ(gameHandler.battles->findBattleQuery(*battle), expected);
}

TEST_F(QueriesProcessorTest, settle_completesALongRunOfQueuedWork)
{
	const PlayerColor player(1);

	// A turn start can queue a lot in one go: one visit per town, each running a
	// routine that visits several buildings. All of it lands in a single settle(),
	// so the round limit must not act as a budget for legitimate work.
	constexpr int queuedItems = 60;
	std::vector<std::shared_ptr<TestRoutine>> queued;

	auto seeder = std::make_shared<TestQuery>(&gh, player, QueryType::HeroMovement);
	seeder->onRemovalAction = [&]()
	{
		for(int i = 0; i < queuedItems; ++i)
		{
			auto routine = std::make_shared<TestRoutine>(&gh, player, 2);
			queued.push_back(routine);
			queries.addQueryWhenIdle(routine);
		}
	};

	queries.addQuery(seeder);
	queries.popIfTop(seeder); // everything above is queued within this one mutation

	EXPECT_EQ(queries.topQuery(player), nullptr) << "work was left unfinished";

	int unfinished = 0;
	for(const auto & routine : queued)
		if(routine->stepsTaken != 2)
			unfinished++;

	EXPECT_EQ(unfinished, 0) << unfinished << " of " << queuedItems << " queued routines never ran";
}

TEST_F(QueriesProcessorTest, replyIsAcceptedWhileAVisitSitsOnTop)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	auto visit = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);

	queries.addQuery(dialog);
	queries.addQuery(visit);

	// A visit blocks every action, but answering a question is not an action - the
	// reply may well be for a query the visit is sitting on top of.
	EXPECT_FALSE(visit->blocksPack(&replyFromPlayer(player)));

	EXPECT_EQ(queries.submitReply(dialog->queryID, player, 1), ReplyOutcome::Accepted);
	EXPECT_TRUE(dialog->isAnswered());
}

TEST_F(QueriesProcessorTest, aVisitStillBlocksOrdinaryActions)
{
	const PlayerColor player(1);
	auto visit = std::make_shared<TestQuery>(&gh, player, QueryType::MapObjectVisit);
	queries.addQuery(visit);

	SaveGame save;
	save.player = player;
	EXPECT_TRUE(visit->blocksPack(&save)) << "saving mid-visit must still be refused";
}

TEST_F(QueriesProcessorTest, submitReply_rejectsAnAnswerWithNoValueWhereOneIsNeeded)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogQuery>(&gh, player, QueryType::BlockingDialog);
	queries.addQuery(dialog);

	// Only a query that offers a way out may be answered with nothing. Accepting it
	// here would resolve the dialog with no answer for the object to act on.
	EXPECT_EQ(queries.submitReply(dialog->queryID, player, std::nullopt),
		ReplyOutcome::RejectedMissingAnswer);
	EXPECT_FALSE(dialog->isAnswered());
	EXPECT_EQ(queries.topQuery(player), dialog);
}
