#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../../server/battles/BattleProcessor.h"
#include "../../../server/activities/BattleActivities.h"
#include "../../../server/activities/Activity.h"
#include "../../../server/activities/MapActivities.h"
#include "../../../server/activities/ActivityProcessor.h"
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

enum class ActivityEvent
{
	OnAdding,
	OnAdded,
	OnRemoval,
	OnExposure
};

class TestActivity;

struct RecordedEvent
{
	TestActivity * activity;
	ActivityEvent event;
};

class TestActivity : public Activity
{
public:
	TestActivity(CGameHandler * gh, std::initializer_list<PlayerColor> affectedPlayers, ActivityType type)
		: Activity(gh, type)
	{
		for(auto player : affectedPlayers)
			players.push_back(player);
	}

	TestActivity(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, ActivityType type)
		: Activity(gh, type)
	{
		for(auto player : affectedPlayers)
			players.push_back(player);
	}

	TestActivity(CGameHandler * gh, PlayerColor player, ActivityType type)
		: Activity(gh, type)
	{
		players.push_back(player);
	}

	std::vector<RecordedEvent> * sharedEventLog = nullptr;
	std::vector<ActivityEvent> events;
	std::vector<PlayerColor> onAddingCalls;
	std::vector<PlayerColor> onAddedCalls;
	std::vector<PlayerColor> onRemovalCalls;
	std::vector<ActivityPtr> exposureArgs;
	bool popOnExposure = false;
	bool addReplacementOnRemoval = false;
	ActivityPtr replacementActivity;
	std::function<void()> onRemovalAction;

	void onAdding(PlayerColor color) override
	{
		events.push_back(ActivityEvent::OnAdding);
		onAddingCalls.push_back(color);
		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnAdding});
	}

	void onAdded(PlayerColor color) override
	{
		events.push_back(ActivityEvent::OnAdded);
		onAddedCalls.push_back(color);
		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnAdded});
	}

	void onRemoval(PlayerColor color) override
	{
		events.push_back(ActivityEvent::OnRemoval);
		onRemovalCalls.push_back(color);

		if(addReplacementOnRemoval && replacementActivity)
			owner->addActivity(replacementActivity);

		if(onRemovalAction)
			onRemovalAction();

		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnRemoval});
	}

	bool blocksPack(const CPackForServer * pack) const override
	{
		// Mirrors VisitActivity: everything is blocked except answering a question.
		if(getType() == ActivityType::MapObjectVisit)
			return blockAllButReply(pack);

		return Activity::blocksPack(pack);
	}

	void onExposure(ActivityPtr topActivity) override
	{
		events.push_back(ActivityEvent::OnExposure);
		exposureArgs.push_back(topActivity);

		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnExposure});

		if(popOnExposure)
			owner->popIfTop(*this);
	}
};

/// An activity that a player reply can end, so that reply routing can be tested without
/// standing up a dialog activity and the netpack traffic that goes with it.
class TestDialogActivity : public Activity
{
public:
	TestDialogActivity(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, ActivityType type)
		: Activity(gh, type)
	{
		for(auto player : affectedPlayers)
			players.push_back(player);

		askQuestion(); // a dialog stands for a question already put to the player
	}

	TestDialogActivity(CGameHandler * gh, PlayerColor player, ActivityType type)
		: Activity(gh, type)
	{
		players.push_back(player);

		askQuestion(); // a dialog stands for a question already put to the player
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

/// A routine that runs a fixed number of steps and can push a child activity on a
/// chosen one, so that suspension and resumption can be driven deterministically.
class TestRoutine : public Activity, public IRoutine
{
public:
	TestRoutine(CGameHandler * gh, PlayerColor player, int totalSteps)
		: Activity(gh, ActivityType::MapObjectVisit)
		, totalSteps(totalSteps)
	{
		players.push_back(player);
	}

	int totalSteps;
	int stepsTaken = 0;

	/// Step index on which to push childToPush, or -1 to never push.
	int pushChildOnStep = -1;
	ActivityPtr childToPush;

	std::vector<ActivityPtr> completedChildren;
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
			owner->addActivity(childToPush);

		return StepResult::Continue;
	}

	void onChildCompleted(const ActivityPtr & child) final
	{
		completedChildren.push_back(child);
	}
};

/// A QuestionAnswer pack from a given player, for checking what blocksPack() lets past.
inline const QuestionAnswer & replyFromPlayer(PlayerColor player)
{
	static QuestionAnswer reply;
	reply.player = player;
	return reply;
}

class ActivityProcessorTest : public ::testing::Test
{
protected:
	std::shared_ptr<CGameState> gameState = std::make_shared<CGameState>();
	GameHandlerTestServer server{gameState};
	CGameHandler gh{server, gameState};
	ActivityProcessor & activities = *gh.activities;
};

class NeutralDwellingBattleActivityTest : public TinyMapGameTest
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

TEST_F(ActivityProcessorTest, topActivity_returnsNullWhenPlayerHasNothingPending)
{
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
}

TEST_F(NeutralDwellingBattleActivityTest, ownedDwellingUsesNeutralBattleSideWithoutNeutralActivity)
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

	const auto attackerActivity = gh.activities->topActivity(PlayerColor(0));
	ASSERT_NE(attackerActivity, nullptr);
	EXPECT_EQ(attackerActivity->getType(), ActivityType::Battle);
	ASSERT_EQ(attackerActivity->players.size(), 1);
	EXPECT_EQ(attackerActivity->players.front(), PlayerColor(0));
	EXPECT_EQ(gh.activities->topActivity(PlayerColor(1)), nullptr);
}

TEST_F(DeferredVictoryLossTest, heroLevelUpDefersVictoryUntilActivityIsAnswered)
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

	auto levelUpActivity = gameHandler.activities->topActivity(levelUpPlayer);
	ASSERT_NE(levelUpActivity, nullptr);
	ASSERT_EQ(levelUpActivity->getType(), ActivityType::HeroLevelUpDialog);

	// The player is mid-level-up, so winning is not announced yet.
	gameHandler.checkVictoryLossConditionsForPlayer(defeatedPlayer);
	EXPECT_EQ(gameState()->getPlayerState(defeatedPlayer)->status, EPlayerStatus::LOSER);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::INGAME);

	ASSERT_EQ(gameHandler.activities->submitReply(levelUpActivity->getActiveQuestionID(), levelUpPlayer, 0),
		ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.activities->topActivity(levelUpPlayer), nullptr);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::WINNER);
}

TEST_F(ActivityProcessorTest, popIfTop_removesTopActivity)
{
	auto activity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);

	activities.addActivity(activity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), activity);
	EXPECT_EQ(activities.countActivity(activity.get()), 1);

	activities.popIfTop(activity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(activity.get()), 0);
}

TEST_F(ActivityProcessorTest, popIfTop_doesNothingWhenActivityIsNotPresent)
{
	auto addedActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto missingActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(addedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), addedActivity);
	EXPECT_EQ(activities.countActivity(addedActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(missingActivity.get()), 0);

	activities.popIfTop(missingActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), addedActivity);
	EXPECT_EQ(activities.countActivity(addedActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(missingActivity.get()), 0);
}

TEST_F(ActivityProcessorTest, popIfTop_skipsWhenNestedActivityIsAbove_andLaterSucceedsAfterUnwind)
{
	auto movementActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto visitActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(movementActivity);
	activities.addActivity(visitActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), visitActivity);
	EXPECT_EQ(activities.countActivity(movementActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(visitActivity.get()), 1);

	activities.popIfTop(movementActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), visitActivity);
	EXPECT_EQ(activities.countActivity(movementActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(visitActivity.get()), 1);

	activities.popIfTop(visitActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), movementActivity);
	EXPECT_EQ(activities.countActivity(movementActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(visitActivity.get()), 0);

	activities.popIfTop(movementActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(movementActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(visitActivity.get()), 0);
}

TEST_F(ActivityProcessorTest, popIfTop_exposesActivityBelowWithRemovedActivityAsArgument)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	activities.popIfTop(topActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), bottomActivity);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded,
		ActivityEvent::OnExposure
	}));

	EXPECT_EQ(bottomActivity->onAddingCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_EQ(bottomActivity->onAddedCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_TRUE(bottomActivity->onRemovalCalls.empty());

	ASSERT_EQ(bottomActivity->exposureArgs.size(), 1);
	EXPECT_EQ(bottomActivity->exposureArgs[0], topActivity);

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));
}

TEST_F(ActivityProcessorTest, popIfTop_allowsExposedActivityToPopItself)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	bottomActivity->popOnExposure = true;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	activities.popIfTop(topActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(bottomActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(topActivity.get()), 0);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded,
		ActivityEvent::OnExposure,
		ActivityEvent::OnRemoval
	}));

	ASSERT_EQ(bottomActivity->exposureArgs.size(), 1);
	EXPECT_EQ(bottomActivity->exposureArgs[0], topActivity);

	EXPECT_EQ(bottomActivity->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(1)}));

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));
}

TEST_F(ActivityProcessorTest, popIfTop_removesMultiPlayerActivityOnlyWhereItIsTop)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::Generic);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	activities.popIfTop(sharedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), blueTopActivity);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(blueTopActivity.get()), 1);

	EXPECT_EQ(sharedActivity->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(0)}));
}

TEST_F(ActivityProcessorTest, popIfTop_removesMultiPlayerActivityAfterItBecomesTopAgain)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::Generic);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	activities.popIfTop(sharedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), blueTopActivity);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);

	activities.popIfTop(blueTopActivity);

	ASSERT_EQ(sharedActivity->exposureArgs.size(), 1);
	EXPECT_EQ(sharedActivity->exposureArgs[0], blueTopActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), sharedActivity);
	EXPECT_EQ(activities.countActivity(blueTopActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);

	activities.popIfTop(sharedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 0);

	EXPECT_EQ(sharedActivity->onRemovalCalls, std::vector<PlayerColor>({
		PlayerColor(0),
		PlayerColor(1)
	}));
}

TEST_F(ActivityProcessorTest, popActivity_removesMultiPlayerActivityOnlyWhereItIsTop)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::Generic);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	activities.popActivity(*sharedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), blueTopActivity);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);
	EXPECT_EQ(sharedActivity->onRemovalCalls, std::vector<PlayerColor>({PlayerColor(0)}));
}

TEST_F(ActivityProcessorTest, popActivity_removesRemainingMultiPlayerActivityAfterItBecomesTop)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::Generic);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	activities.popActivity(*sharedActivity);
	activities.popIfTop(blueTopActivity);
	activities.popActivity(*sharedActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 0);
	EXPECT_EQ(sharedActivity->onRemovalCalls, std::vector<PlayerColor>({
		PlayerColor(0),
		PlayerColor(1)
	}));
}

TEST_F(ActivityProcessorTest, popIfTop_callsRemovalBeforeExposure)
{
	std::vector<RecordedEvent> eventLog;

	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	bottomActivity->sharedEventLog = &eventLog;
	topActivity->sharedEventLog = &eventLog;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	eventLog.clear();

	activities.popIfTop(topActivity);

	ASSERT_EQ(eventLog.size(), 2);
	EXPECT_EQ(eventLog[0].activity, topActivity.get());
	EXPECT_EQ(eventLog[0].event, ActivityEvent::OnRemoval);
	EXPECT_EQ(eventLog[1].activity, bottomActivity.get());
	EXPECT_EQ(eventLog[1].event, ActivityEvent::OnExposure);
}

TEST_F(ActivityProcessorTest, popActivity_callsRemovalBeforeExposure)
{
	std::vector<RecordedEvent> eventLog;

	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	bottomActivity->sharedEventLog = &eventLog;
	topActivity->sharedEventLog = &eventLog;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	eventLog.clear();

	activities.popActivity(*topActivity);

	ASSERT_EQ(eventLog.size(), 2);
	EXPECT_EQ(eventLog[0].activity, topActivity.get());
	EXPECT_EQ(eventLog[0].event, ActivityEvent::OnRemoval);
	EXPECT_EQ(eventLog[1].activity, bottomActivity.get());
	EXPECT_EQ(eventLog[1].event, ActivityEvent::OnExposure);
}

TEST_F(ActivityProcessorTest, popIfTop_skipsExposureWhenRemovalAddsNewTopActivity)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);
	auto replacementActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::Generic);

	topActivity->addReplacementOnRemoval = true;
	topActivity->replacementActivity = replacementActivity;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	activities.popIfTop(topActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), replacementActivity);
	EXPECT_EQ(activities.countActivity(bottomActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(topActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(replacementActivity.get()), 1);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded
	}));
	EXPECT_TRUE(bottomActivity->exposureArgs.empty());

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));

	EXPECT_EQ(replacementActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded
	}));
}

TEST_F(ActivityProcessorTest, addActivity_addsSameActivityForAllAffectedPlayers)
{
	auto activity = std::make_shared<TestActivity>(&gh, std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)}, ActivityType::Generic);

	activities.addActivity(activity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), activity);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), activity);
	EXPECT_EQ(activities.countActivity(activity.get()), 2);

	EXPECT_EQ(activity->events, std::vector<ActivityEvent>({
	ActivityEvent::OnAdding,
	ActivityEvent::OnAdded,
	ActivityEvent::OnAdding,
	ActivityEvent::OnAdded
	}));

	EXPECT_EQ(activity->onAddingCalls, std::vector<PlayerColor>({PlayerColor(0), PlayerColor(1)}));
	EXPECT_EQ(activity->onAddedCalls, std::vector<PlayerColor>({PlayerColor(0), PlayerColor(1)}));
	EXPECT_TRUE(activity->onRemovalCalls.empty());
	EXPECT_TRUE(activity->exposureArgs.empty());
}

TEST_F(ActivityProcessorTest, addActivity_skipsDuplicateBackWithoutRepeatingOnAdded)
{
	auto activity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);

	activities.addActivity(activity);
	activities.addActivity(activity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), activity);
	EXPECT_EQ(activities.countActivity(activity.get()), 1);

	EXPECT_EQ(activity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdding,
		ActivityEvent::OnAdded
	}));

	EXPECT_EQ(activity->onAddingCalls, std::vector<PlayerColor>({PlayerColor(1)}));
	EXPECT_EQ(activity->onAddedCalls, std::vector<PlayerColor>({PlayerColor(1)}));
}

TEST_F(ActivityProcessorTest, getActivity_returnsNullForUnknownActivityId)
{
	auto activity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);

	activities.addActivity(activity);

	EXPECT_EQ(activities.getActivity(QuestionID(12345)), nullptr);
}

TEST_F(ActivityProcessorTest, getActivity_findsAnActivityByTheQuestionItAsked)
{
	auto activity = std::make_shared<TestDialogActivity>(&gh, PlayerColor(1), ActivityType::BlockingDialog);
	const auto question = activity->getActiveQuestionID();

	activities.addActivity(activity);

	EXPECT_EQ(activities.getActivity(question), activity);

	activities.popIfTop(activity);

	EXPECT_EQ(activities.getActivity(question), nullptr);
}

TEST_F(ActivityProcessorTest, countActivity_returnsZeroForNullptr)
{
	EXPECT_EQ(activities.countActivity(nullptr), 0);
}


// --------------------------------------------------------------------------------
// Reply routing.
//
// The client is prompted for an activity and answers it, but the server may push
// something else in between. These tests drive the processor through those
// orderings directly, because that is the shape of the client/server race that
// used to leave a player holding an activity that had already been answered.
// --------------------------------------------------------------------------------

TEST_F(ActivityProcessorTest, submitReply_resolvesTopActivity)
{
	const PlayerColor player(1);
	auto activity = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	activities.addActivity(activity);

	EXPECT_EQ(activities.submitReply(activity->getActiveQuestionID(), player, 7), ReplyOutcome::Accepted);

	EXPECT_EQ(activities.topActivity(player), nullptr);
	EXPECT_EQ(activity->receivedReply, std::optional<int32_t>(7));
	EXPECT_EQ(activity->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, submitReply_acceptsReplyForBuriedActivityAndResolvesItOnceExposed)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto pushedAfterPrompt = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(dialog);
	// Server pushes something else after the dialog was sent to the client, but
	// before the client's answer arrives.
	activities.addActivity(pushedAfterPrompt);

	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 3), ReplyOutcome::Accepted);

	// The dialog is answered but still buried, so it stays put for now.
	EXPECT_EQ(activities.topActivity(player), pushedAfterPrompt);
	EXPECT_TRUE(dialog->isAnswered());
	EXPECT_EQ(dialog->onRemovalCalls, 0);

	activities.popIfTop(pushedAfterPrompt);

	// Exposing it must resolve it rather than leave the player waiting forever.
	EXPECT_EQ(activities.topActivity(player), nullptr);
	EXPECT_EQ(dialog->receivedReply, std::optional<int32_t>(3));
	EXPECT_EQ(dialog->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, submitReply_resolvesSeveralStackedActivitiesAnsweredOutOfOrder)
{
	const PlayerColor player(1);
	auto bottom = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto middle = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::TeleportDialog);
	auto top = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::GarrisonDialog);

	activities.addActivity(bottom);
	activities.addActivity(middle);
	activities.addActivity(top);

	// Answers arrive bottom-up - the exact opposite of the stack order.
	EXPECT_EQ(activities.submitReply(bottom->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(activities.submitReply(middle->getActiveQuestionID(), player, 2), ReplyOutcome::Accepted);
	EXPECT_EQ(activities.topActivity(player), top);

	// Answering the top must unwind all three, not just one.
	EXPECT_EQ(activities.submitReply(top->getActiveQuestionID(), player, 3), ReplyOutcome::Accepted);

	EXPECT_EQ(activities.topActivity(player), nullptr);
	EXPECT_EQ(bottom->onRemovalCalls, 1);
	EXPECT_EQ(middle->onRemovalCalls, 1);
	EXPECT_EQ(top->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, submitReply_ignoresDuplicateReply)
{
	const PlayerColor player(1);
	auto activity = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto blocker = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(activity);
	activities.addActivity(blocker);

	EXPECT_EQ(activities.submitReply(activity->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(activities.submitReply(activity->getActiveQuestionID(), player, 2), ReplyOutcome::IgnoredAlreadyAnswered);

	// The second answer must not overwrite the first.
	EXPECT_EQ(activity->setReplyCalls, 1);
	EXPECT_EQ(activity->receivedReply, std::optional<int32_t>(1));
}

TEST_F(ActivityProcessorTest, submitReply_ignoresReplyToActivityThatAlreadyCompleted)
{
	const PlayerColor player(1);
	auto activity = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	activities.addActivity(activity);

	const QuestionID questionID = activity->getActiveQuestionID();
	activities.popIfTop(activity); // removed by some other event while the reply was in flight

	EXPECT_EQ(activities.submitReply(questionID, player, 1), ReplyOutcome::IgnoredAlreadyCompleted);
}

TEST_F(ActivityProcessorTest, submitReply_rejectsUnknownActivity)
{
	const PlayerColor player(1);
	auto activity = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	activities.addActivity(activity);

	EXPECT_EQ(activities.submitReply(QuestionID(12345), player, 1), ReplyOutcome::RejectedUnknownActivity);
}

TEST_F(ActivityProcessorTest, submitReply_rejectsReplyFromPlayerNotAffectedByActivity)
{
	const PlayerColor owner(1);
	const PlayerColor other(2);
	auto activity = std::make_shared<TestDialogActivity>(&gh, owner, ActivityType::BlockingDialog);
	activities.addActivity(activity);

	EXPECT_EQ(activities.submitReply(activity->getActiveQuestionID(), other, 1), ReplyOutcome::RejectedWrongPlayer);
	EXPECT_FALSE(activity->isAnswered());
	EXPECT_EQ(activities.topActivity(owner), activity);
}

TEST_F(ActivityProcessorTest, submitReply_rejectsActivityThatCannotBeEndedByAnswer)
{
	const PlayerColor player(1);
	auto activity = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);
	activities.addActivity(activity);

	EXPECT_EQ(activities.submitReply(activity->getActiveQuestionID(), player, 1), ReplyOutcome::RejectedNotAnswerable);
	EXPECT_EQ(activities.topActivity(player), activity);
}

TEST_F(ActivityProcessorTest, getActivity_scopedByPlayerDistinguishesSharedActivityIds)
{
	// QuestionID::CLIENT is used by every pause activity, so two players can legitimately
	// hold different activities carrying the same ID at the same time.
	const PlayerColor first(1);
	const PlayerColor second(2);

	auto firstActivity = std::make_shared<TestDialogActivity>(&gh, first, ActivityType::TimerPause);
	auto secondActivity = std::make_shared<TestDialogActivity>(&gh, second, ActivityType::TimerPause);
	firstActivity->expectAnswerTo(QuestionID::CLIENT);
	secondActivity->expectAnswerTo(QuestionID::CLIENT);

	activities.addActivity(firstActivity);
	activities.addActivity(secondActivity);

	EXPECT_EQ(activities.getActivity(QuestionID::CLIENT, first), firstActivity);
	EXPECT_EQ(activities.getActivity(QuestionID::CLIENT, second), secondActivity);

	// A reply must land on the replying player's own activity.
	EXPECT_EQ(activities.submitReply(QuestionID::CLIENT, second, 0), ReplyOutcome::Accepted);
	EXPECT_FALSE(firstActivity->isAnswered());
	EXPECT_EQ(activities.topActivity(first), firstActivity);
	EXPECT_EQ(activities.topActivity(second), nullptr);
}

TEST_F(ActivityProcessorTest, submitReply_sharedActivityIsRemovedFromEveryAffectedPlayer)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto shared = std::make_shared<TestDialogActivity>(&gh, std::vector<PlayerColor>{first, second}, ActivityType::BattleDialog);

	activities.addActivity(shared);
	ASSERT_EQ(activities.countActivity(shared.get()), 2);

	EXPECT_EQ(activities.submitReply(shared->getActiveQuestionID(), first, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(activities.countActivity(shared.get()), 0);
	EXPECT_EQ(activities.topActivity(first), nullptr);
	EXPECT_EQ(activities.topActivity(second), nullptr);
}

TEST_F(ActivityProcessorTest, submitReply_sharedActivityWaitsForPlayerWhoIsStillBusy)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto shared = std::make_shared<TestDialogActivity>(&gh, std::vector<PlayerColor>{first, second}, ActivityType::BattleDialog);
	auto busy = std::make_shared<TestActivity>(&gh, second, ActivityType::MapObjectVisit);

	activities.addActivity(shared);
	activities.addActivity(busy); // only the second player has something on top of it

	EXPECT_EQ(activities.submitReply(shared->getActiveQuestionID(), first, 1), ReplyOutcome::Accepted);

	// Resolved for the player whose stack allows it...
	EXPECT_EQ(activities.topActivity(first), nullptr);
	// ...and still in place for the one who is busy, rather than silently skipped.
	EXPECT_EQ(activities.countActivity(shared.get()), 1);
	EXPECT_EQ(activities.topActivity(second), busy);

	activities.popIfTop(busy);

	EXPECT_EQ(activities.topActivity(second), nullptr);
	EXPECT_EQ(activities.countActivity(shared.get()), 0);
}

// --------------------------------------------------------------------------------
// Property test: no ordering of prompts and replies may leave a player holding a
// activity that has already been answered. That invariant is exactly what used to
// fail in practice, and it is not reachable by enumerating cases by hand.
// --------------------------------------------------------------------------------

TEST_F(ActivityProcessorTest, noInterleavingLeavesPlayerHoldingAnAnsweredActivity)
{
	const PlayerColor player(1);

	for(uint32_t seed = 0; seed < 500; ++seed)
	{
		ActivityProcessor processor(gh);
		std::mt19937 rng(seed);
		std::vector<std::shared_ptr<TestDialogActivity>> live;
		std::vector<QuestionID> answeredButLive;

		for(int step = 0; step < 40; ++step)
		{
			const bool canReply = !live.empty();
			const int action = rng() % (canReply ? 3 : 1);

			if(action == 0) // server pushes a new activity
			{
				auto activity = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
				processor.addActivity(activity);
				live.push_back(activity);
			}
			else if(action == 1) // client replies to some activity it was prompted for
			{
				const auto & target = live[rng() % live.size()];
				processor.submitReply(target->getActiveQuestionID(), player, 0);
			}
			else // server removes the top activity for reasons of its own
			{
				if(auto top = processor.topActivity(player))
					processor.popIfTop(top);
			}

			// Invariant: an answered activity is never left sitting at the top.
			if(auto top = processor.topActivity(player))
			{
				ASSERT_FALSE(top->isAnswered())
					<< "seed " << seed << " step " << step
					<< " left an answered activity on top:\n" << processor.describeStacks();
			}

			vstd::erase_if(live, [&processor](const std::shared_ptr<TestDialogActivity> & q)
			{
				return processor.countActivity(q.get()) == 0;
			});
		}

		// Draining the stack must always terminate with nothing answered left behind.
		while(auto top = processor.topActivity(player))
		{
			ASSERT_FALSE(top->isAnswered()) << "seed " << seed << ":\n" << processor.describeStacks();
			processor.popIfTop(top);
		}
	}
}


// --------------------------------------------------------------------------------
// Quiescence and queued work.
//
// Removing an activity runs hooks that may add or remove further activities, so the stacks
// pass through states that are not meaningful - briefly empty, or holding an activity
// that is about to be replaced. Work queued behind whatever the player is doing must
// start from the settled state, never from one of those.
// --------------------------------------------------------------------------------

TEST_F(ActivityProcessorTest, waitingActivity_doesNotStartWhilePlayerIsBusy)
{
	const PlayerColor player(1);
	auto busy = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);
	auto pending = std::make_shared<TestActivity>(&gh, player, ActivityType::TurnStartVisit);

	activities.addActivity(busy);
	activities.addActivityWhenIdle(pending);

	EXPECT_EQ(activities.topActivity(player), busy);
	EXPECT_TRUE(pending->onAddedCalls.empty());

	activities.popIfTop(busy);

	EXPECT_EQ(activities.topActivity(player), pending);
	EXPECT_EQ(pending->onAddedCalls.size(), 1u);
}

TEST_F(ActivityProcessorTest, waitingActivity_startsOnceAfterTheStackFullyUnwinds)
{
	const PlayerColor player(1);
	auto bottom = std::make_shared<TestActivity>(&gh, player, ActivityType::HeroMovement);
	auto top = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);
	bottom->popOnExposure = true; // exposing it unwinds the rest of the stack

	activities.addActivity(bottom);
	activities.addActivity(top);
	activities.addActivityWhenIdle(std::make_shared<TestActivity>(&gh, player, ActivityType::TurnStartVisit));

	// Two removals happen here, but there is only one quiescent point, so the
	// queued activity must be started exactly once.
	activities.popIfTop(top);

	auto started = activities.topActivity(player);
	ASSERT_NE(started, nullptr);
	EXPECT_EQ(started->getType(), ActivityType::TurnStartVisit);
	EXPECT_EQ(std::dynamic_pointer_cast<TestActivity>(started)->onAddedCalls.size(), 1u);
}

TEST_F(ActivityProcessorTest, waitingActivity_doesNotSlipIntoTheGapOfAReplacementChain)
{
	const PlayerColor player(1);

	// An activity that pushes a successor as it is removed - the shape of a level-up
	// chain, where the player is never really idle between the two.
	auto replacement = std::make_shared<TestActivity>(&gh, player, ActivityType::HeroLevelUpDialog);
	auto original = std::make_shared<TestActivity>(&gh, player, ActivityType::HeroLevelUpDialog);
	original->addReplacementOnRemoval = true;
	original->replacementActivity = replacement;

	auto pending = std::make_shared<TestActivity>(&gh, player, ActivityType::TurnStartVisit);

	activities.addActivity(original);
	activities.addActivityWhenIdle(pending);

	activities.popIfTop(original);

	// The queued work must wait for the whole chain, not cut in between its links.
	EXPECT_EQ(activities.topActivity(player), replacement);
	EXPECT_TRUE(pending->onAddedCalls.empty());

	activities.popIfTop(replacement);
	EXPECT_EQ(activities.topActivity(player), pending);
}

TEST_F(ActivityProcessorTest, waitingActivity_sharedByTwoPlayersStartsOnlyWhenBothAreIdle)
{
	const PlayerColor first(1);
	const PlayerColor second(2);
	auto busy = std::make_shared<TestActivity>(&gh, second, ActivityType::MapObjectVisit);
	auto pending = std::make_shared<TestActivity>(&gh, std::vector<PlayerColor>{first, second}, ActivityType::Battle);

	activities.addActivity(busy);
	activities.addActivityWhenIdle(pending);

	// The first player is idle, but starting now would put the activity on one stack
	// and not the other.
	EXPECT_TRUE(pending->onAddedCalls.empty());
	EXPECT_EQ(activities.topActivity(first), nullptr);

	activities.popIfTop(busy);

	EXPECT_EQ(activities.topActivity(first), pending);
	EXPECT_EQ(activities.topActivity(second), pending);
	EXPECT_EQ(activities.countActivity(pending.get()), 2);
}

TEST_F(ActivityProcessorTest, settle_resolvesRepliesThatArrivedWhileStacksWereMoving)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto cover = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(dialog);
	activities.addActivity(cover);

	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(activities.topActivity(player), cover);

	// Removing the cover exposes an answered activity; settle() must resolve it within
	// the same quiescent point rather than leaving it for some later mutation.
	activities.popIfTop(cover);

	EXPECT_EQ(activities.topActivity(player), nullptr);
	EXPECT_EQ(dialog->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, settle_givesUpInsteadOfLoopingForeverWhenDeferredWorkKeepsChanging)
{
	const PlayerColor player(1);

	// Every removal queues another activity, which is started, removed, and queues
	// another... Must terminate rather than spin or recurse until the stack blows.
	std::function<void()> queueAnother = [&]()
	{
		auto next = std::make_shared<TestActivity>(&gh, player, ActivityType::TurnStartVisit);
		next->onRemovalAction = queueAnother;
		activities.addActivityWhenIdle(next);
		activities.popIfTop(next);
	};

	auto first = std::make_shared<TestActivity>(&gh, player, ActivityType::TurnStartVisit);
	first->onRemovalAction = queueAnother;

	activities.addActivity(first);
	activities.popIfTop(first);

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

TEST_F(ActivityProcessorTest, routine_isSteppedToCompletionAndThenRemoved)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);

	activities.addActivity(routine);

	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(routine->stepLog, std::vector<int>({0, 1, 2}));
	EXPECT_EQ(activities.topActivity(player), nullptr);
}

TEST_F(ActivityProcessorTest, routine_suspendsWhenAStepPushesAChild)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 4);
	auto child = std::make_shared<TestActivity>(&gh, player, ActivityType::BlockingDialog);
	routine->pushChildOnStep = 1;
	routine->childToPush = child;

	activities.addActivity(routine);

	// Stopped on the step that pushed the child, with the child on top.
	EXPECT_EQ(routine->stepsTaken, 2);
	EXPECT_EQ(activities.topActivity(player), child);
}

TEST_F(ActivityProcessorTest, routine_resumesFromWhereItStoppedOnceTheChildFinishes)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 4);
	auto child = std::make_shared<TestActivity>(&gh, player, ActivityType::BlockingDialog);
	routine->pushChildOnStep = 1;
	routine->childToPush = child;

	activities.addActivity(routine);
	ASSERT_EQ(routine->stepsTaken, 2);

	activities.popIfTop(child);

	// Carries on from step 2 - it does not restart, and does not skip a step.
	EXPECT_EQ(routine->stepLog, std::vector<int>({0, 1, 2, 3}));
	EXPECT_EQ(activities.topActivity(player), nullptr);

	ASSERT_EQ(routine->completedChildren.size(), 1u);
	EXPECT_EQ(routine->completedChildren.front(), child);
}

TEST_F(ActivityProcessorTest, routine_doesNotReceiveTheGenericExposureHook)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 2);
	auto child = std::make_shared<TestActivity>(&gh, player, ActivityType::BlockingDialog);
	routine->pushChildOnStep = 0;
	routine->childToPush = child;

	activities.addActivity(routine);
	activities.popIfTop(child);

	// Child completion is reported through onChildCompleted only, so a routine
	// cannot accidentally implement resumption twice.
	EXPECT_EQ(routine->completedChildren.size(), 1u);
}

TEST_F(ActivityProcessorTest, routine_waitsForAChildThatPushesAChildOfItsOwn)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);
	auto child = std::make_shared<TestActivity>(&gh, player, ActivityType::BlockingDialog);
	auto grandchild = std::make_shared<TestActivity>(&gh, player, ActivityType::Battle);
	routine->pushChildOnStep = 0;
	routine->childToPush = child;

	activities.addActivity(routine);
	ASSERT_EQ(activities.topActivity(player), child);

	activities.addActivity(grandchild);
	EXPECT_EQ(routine->stepsTaken, 1); // still suspended, two levels down now

	activities.popIfTop(grandchild);
	EXPECT_EQ(routine->stepsTaken, 1); // the child is still unfinished

	activities.popIfTop(child);
	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(activities.topActivity(player), nullptr);
}

TEST_F(ActivityProcessorTest, routine_underneathAnAnsweredActivityResumesAfterItResolves)
{
	const PlayerColor player(1);
	auto routine = std::make_shared<TestRoutine>(&gh, player, 3);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	routine->pushChildOnStep = 0;
	routine->childToPush = dialog;

	activities.addActivity(routine);
	ASSERT_EQ(activities.topActivity(player), dialog);

	// Answering the dialog must both resolve it and let the routine continue,
	// within the same quiescent point.
	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(activities.topActivity(player), nullptr);
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

	// The whole visit runs within the call: reward granted, object gone, no activity left.
	EXPECT_GT(gameState()->getPlayerState(player)->resources[GameResID::GOLD], goldBefore);
	EXPECT_EQ(gameState()->getObjInstance(resourceID), nullptr);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
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
	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	EXPECT_NE(gameHandler.getVisitingHero(pandora), nullptr);

	// Answering resumes the visit, which then runs to the end and unwinds fully.
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
	EXPECT_EQ(gameState()->getObjInstance(pandoraID), nullptr);
}

TEST_F(MapObjectVisitTest, visitStaysSuspendedAcrossAChainOfChildActivities)
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
		for(const auto & activity : gameHandler.activities->allActivities())
			if(activity->getType() == ActivityType::MapObjectVisit)
				return true;
		return false;
	};

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	EXPECT_TRUE(visitIsPending());
	EXPECT_EQ(gameHandler.getVisitingHero(pandora), hero);

	// Answering starts a battle one level deeper. The visit must still be waiting
	// underneath it, so the object can be told the result when the battle ends.
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	auto battle = gameHandler.activities->topActivity(player);
	ASSERT_NE(battle, nullptr);
	EXPECT_EQ(battle->getType(), ActivityType::Battle);
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

	// The visit activity must be findable on the stack for the whole visit - object code
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

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	ASSERT_EQ(gameHandler.activities->topActivity(player)->getType(), ActivityType::Battle);
	gameHandler.battles->cheatBattleVictory(player);

	// Accept the result rather than replaying the battle.
	auto resultDialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(resultDialog, nullptr);
	ASSERT_EQ(resultDialog->getType(), ActivityType::BattleDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(resultDialog->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);

	// The object has applied the battle result and granted its reward once. The
	// level-up earned from battle experience is only now offered, on top of the visit.
	ASSERT_EQ(rewardsGranted(), 1u);
	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);

	// The hero may gain several levels at once, each prompting in turn.
	int levelUpsAnswered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		if(pending->getType() != ActivityType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++levelUpsAnswered, 10) << "level-up chain did not terminate";
	}

	EXPECT_GE(levelUpsAnswered, 1);

	// None of those level-ups is part of the object's reward pipeline, so the object
	// must not be told about them - otherwise heroLevelUpDone() grants the reward
	// once more for each one.
	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

// --------------------------------------------------------------------------------
// Locating the battle activity.
//
// A battle activity is one object on both belligerents' stacks. Most callers look at
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

TEST_F(TwoPlayerBattleTest, findBattleActivityLooksAtBothSidesForTheOneSharedActivity)
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

	auto * found = gameHandler.battles->findBattleActivity(*battle);
	ASSERT_NE(found, nullptr);

	// One object, on both belligerents' stacks - and only one per player.
	EXPECT_EQ(gameHandler.activities->findSoleActivity<BattleActivity>(PlayerColor(0)), found);
	EXPECT_EQ(gameHandler.activities->findSoleActivity<BattleActivity>(PlayerColor(AI_PLAYER)), found);
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
	auto top = gameHandler.activities->topActivity(player);
	ASSERT_NE(top, nullptr);
	EXPECT_EQ(top->getType(), ActivityType::Battle);
	EXPECT_EQ(gameHandler.getVisitingHero(monster), hero);
}

// --------------------------------------------------------------------------------
// Repeated questions.
//
// A hero can earn several levels from one reward, and used to be asked about each
// through a separate activity pushed as the previous one was removed. It is now one
// activity that asks repeatedly, so the player is never briefly free between levels
// and whatever waits underneath is told once, at the end.
// --------------------------------------------------------------------------------

namespace
{
class LevelUpActivityTest : public TinyMapGameTest
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

TEST_F(LevelUpActivityTest, severalLevelsAreAskedAboutByOneActivityThatStaysOnTheStack)
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

	auto activity = gameHandler.activities->topActivity(player);
	ASSERT_NE(activity, nullptr);
	ASSERT_EQ(activity->getType(), ActivityType::HeroLevelUpDialog);

	std::set<QuestionID> questionsAsked;
	int answers = 0;

	while(auto pending = gameHandler.activities->topActivity(player))
	{
		// Always the same activity object, however many times it asks.
		ASSERT_EQ(pending, activity) << "a second activity was pushed instead of asking again";

		const auto questionID = pending->getActiveQuestionID();
		EXPECT_TRUE(questionsAsked.insert(questionID).second) << "a question id was reused";

		ASSERT_EQ(gameHandler.activities->submitReply(questionID, player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++answers, 50) << "level-up sequence did not terminate";
	}

	EXPECT_GT(answers, 1) << "expected more than one level from this much experience";
	EXPECT_GT(hero->level, levelBefore + 1);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

TEST_F(LevelUpActivityTest, answerNamingASupersededQuestionIsIgnored)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	gameHandler.onAdvInterfaceReady(player);

	gameHandler.giveExperience(hero, 100000);

	auto activity = gameHandler.activities->topActivity(player);
	ASSERT_NE(activity, nullptr);

	const auto firstQuestion = activity->getActiveQuestionID();
	ASSERT_EQ(gameHandler.activities->submitReply(firstQuestion, player, 0), ReplyOutcome::Accepted);

	// The next question is now outstanding; the previous one is history.
	ASSERT_EQ(gameHandler.activities->topActivity(player), activity);
	ASSERT_NE(activity->getActiveQuestionID(), firstQuestion);

	EXPECT_EQ(gameHandler.activities->submitReply(firstQuestion, player, 0),
		ReplyOutcome::IgnoredAlreadyCompleted);

	// Naming the activity rather than the question is not expressible at all: an
	// ActivityID is a distinct type and cannot be passed here.

	// Neither stale answer consumed the outstanding question.
	EXPECT_EQ(gameHandler.activities->topActivity(player), activity);
}

TEST_F(LevelUpActivityTest, nothingIsAskedBeforeThePlayersInterfaceIsReady)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	// Deliberately no onAdvInterfaceReady().
	gameHandler.giveExperience(hero, 10);

	auto activity = gameHandler.activities->topActivity(player);
	ASSERT_NE(activity, nullptr);
	EXPECT_FALSE(activity->hasOutstandingQuestion()) << "asked before the client could show it";

	// The level is applied by the dialog pack, so it is still pending too.
	EXPECT_TRUE(hero->gainsLevel());

	gameHandler.onAdvInterfaceReady(player);

	EXPECT_TRUE(activity->hasOutstandingQuestion());
	EXPECT_FALSE(hero->gainsLevel());
}

TEST_F(LevelUpActivityTest, everyQuestionSentToTheClientIsReportedResolved)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	gameHandler.onAdvInterfaceReady(player);

	gameHandler.giveExperience(hero, 100000); // worth several levels at once

	while(auto pending = gameHandler.activities->topActivity(player))
	{
		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
	}

	// The client keeps an activity-backed dialog open until the server reports that
	// question resolved, so every prompt sent has to come back resolved - by the id
	// the client was given, not by the id of the activity behind it.
	ASSERT_GT(server.levelUpPromptIDs.size(), 1u) << "expected several levels";
	EXPECT_EQ(server.resolvedQuestionIDs, server.levelUpPromptIDs);
}

TEST_F(TwoPlayerBattleTest, battleActivityIsStillFoundWhenThePlayerPausesMidBattle)
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

	auto * expected = gameHandler.battles->findBattleActivity(*battle);
	ASSERT_NE(expected, nullptr);

	// BattleActivity lets GamePause through, so a player may pause mid-battle, which
	// puts a TimerPauseActivity on top of the battle activity. That is legal, and the
	// battle activity must still be found - looking only at the top of the stack lost
	// it, and the battle then ended with "Cannot find battle activity!".
	auto pause = std::make_shared<TimerPauseActivity>(&gameHandler, PlayerColor(0));
	gameHandler.activities->addActivity(pause);
	ASSERT_EQ(gameHandler.activities->topActivity(PlayerColor(0)), pause);

	EXPECT_EQ(gameHandler.battles->findBattleActivity(*battle), expected);
}

TEST_F(ActivityProcessorTest, settle_completesALongRunOfQueuedWork)
{
	const PlayerColor player(1);

	// A turn start can queue a lot in one go: one visit per town, each running a
	// routine that visits several buildings. All of it lands in a single settle(),
	// so the round limit must not act as a budget for legitimate work.
	constexpr int queuedItems = 60;
	std::vector<std::shared_ptr<TestRoutine>> queued;

	auto seeder = std::make_shared<TestActivity>(&gh, player, ActivityType::HeroMovement);
	seeder->onRemovalAction = [&]()
	{
		for(int i = 0; i < queuedItems; ++i)
		{
			auto routine = std::make_shared<TestRoutine>(&gh, player, 2);
			queued.push_back(routine);
			activities.addActivityWhenIdle(routine);
		}
	};

	activities.addActivity(seeder);
	activities.popIfTop(seeder); // everything above is queued within this one mutation

	EXPECT_EQ(activities.topActivity(player), nullptr) << "work was left unfinished";

	int unfinished = 0;
	for(const auto & routine : queued)
		if(routine->stepsTaken != 2)
			unfinished++;

	EXPECT_EQ(unfinished, 0) << unfinished << " of " << queuedItems << " queued routines never ran";
}

TEST_F(ActivityProcessorTest, replyIsAcceptedWhileAVisitSitsOnTop)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto visit = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(dialog);
	activities.addActivity(visit);

	// A visit blocks every action, but answering a question is not an action - the
	// reply may well be for an activity the visit is sitting on top of.
	EXPECT_FALSE(visit->blocksPack(&replyFromPlayer(player)));

	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);
	EXPECT_TRUE(dialog->isAnswered());
}

TEST_F(ActivityProcessorTest, aVisitStillBlocksOrdinaryActions)
{
	const PlayerColor player(1);
	auto visit = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);
	activities.addActivity(visit);

	SaveGame save;
	save.player = player;
	EXPECT_TRUE(visit->blocksPack(&save)) << "saving mid-visit must still be refused";
}

TEST_F(ActivityProcessorTest, submitReply_rejectsAnAnswerWithNoValueWhereOneIsNeeded)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	activities.addActivity(dialog);

	// Only an activity that offers a way out may be answered with nothing. Accepting it
	// here would resolve the dialog with no answer for the object to act on.
	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, std::nullopt),
		ReplyOutcome::RejectedMissingAnswer);
	EXPECT_FALSE(dialog->isAnswered());
	EXPECT_EQ(activities.topActivity(player), dialog);
}
