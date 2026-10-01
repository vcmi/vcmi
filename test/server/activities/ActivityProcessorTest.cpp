#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../../server/battles/BattleProcessor.h"
#include "../../../server/activities/BattleActivities.h"
#include "../../../server/activities/Activity.h"
#include "../../../server/activities/MapActivities.h"
#include "../../../server/activities/VisitActivities.h"
#include "../../../server/activities/ActivityProcessor.h"
#include "../../../server/TurnTimerHandler.h"
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
#include "lib/mapObjects/Quest.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapObjects/TownBuildingInstance.h"
#include "lib/bonuses/Bonus.h"
#include "lib/CPlayerState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"
#include "lib/mapping/CMapEvent.h"
#include "lib/GameLibrary.h"
#include "lib/CSkillHandler.h"

namespace
{

enum class ActivityEvent
{
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
			addPlayer(player);
	}

	TestActivity(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, ActivityType type)
		: Activity(gh, type)
	{
		for(auto player : affectedPlayers)
			addPlayer(player);
	}

	TestActivity(CGameHandler * gh, PlayerColor player, ActivityType type)
		: Activity(gh, type)
	{
		addPlayer(player);
	}

	std::vector<RecordedEvent> * sharedEventLog = nullptr;
	std::vector<ActivityEvent> events;
	int onAddedCalls = 0;
	int onRemovalCalls = 0;
	std::vector<ActivityPtr> exposureArgs;
	bool finishOnExposure = false;
	bool addReplacementOnRemoval = false;
	ActivityPtr replacementActivity;
	std::function<void()> onRemovalAction;

	void onAdded() override
	{
		events.push_back(ActivityEvent::OnAdded);
		onAddedCalls++;
		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnAdded});
	}

	void onRemoval() override
	{
		events.push_back(ActivityEvent::OnRemoval);
		onRemovalCalls++;

		if(addReplacementOnRemoval && replacementActivity)
			owner->addActivity(replacementActivity);

		if(onRemovalAction)
			onRemovalAction();

		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnRemoval});
	}

	bool blocksPack(const CPackForServer * pack) const override
	{
		// Mirrors VisitActivity: everything is blocked except answers
		if(getType() == ActivityType::MapObjectVisit)
			return blockAllButReply(pack);

		return Activity::blocksPack(pack);
	}

	void onChildCompleted(const ActivityPtr & child) override
	{
		events.push_back(ActivityEvent::OnExposure);
		exposureArgs.push_back(child);

		if(sharedEventLog)
			sharedEventLog->push_back({this, ActivityEvent::OnExposure});

		if(finishOnExposure)
			finish();
	}
};

/// An activity that a player reply can end, to test reply routing without a dialog
/// activity and its netpack traffic.
class TestDialogActivity : public Activity
{
public:
	TestDialogActivity(CGameHandler * gh, const std::vector<PlayerColor> & affectedPlayers, ActivityType type)
		: Activity(gh, type)
	{
		for(auto player : affectedPlayers)
			addPlayer(player);

		askQuestion(); // a dialog stands for a question already asked
	}

	TestDialogActivity(CGameHandler * gh, PlayerColor player, ActivityType type)
		: Activity(gh, type)
	{
		addPlayer(player);

		askQuestion(); // a dialog stands for a question already asked
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

	void onRemoval() override
	{
		onRemovalCalls++;
	}
};

/// A routine of a fixed number of steps that can push a child activity on a chosen one,
/// to drive suspension and resumption deterministically.
class TestRoutine : public Activity, public IRoutine
{
public:
	TestRoutine(CGameHandler * gh, PlayerColor player, int totalSteps)
		: Activity(gh, ActivityType::MapObjectVisit)
		, totalSteps(totalSteps)
	{
		addPlayer(player);
	}

	int totalSteps;
	int stepsTaken = 0;

	int pushChildOnStep = -1; ///< step index on which to push childToPush, -1 to never push
	ActivityPtr childToPush;

	std::vector<ActivityPtr> completedChildren;
	/// Step indices at which advance() was entered, to check resumption order
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

	std::function<void()> onRemovalAction;

	void onRemoval() override
	{
		if(onRemovalAction)
			onRemovalAction();
	}
};

/// A QuestionAnswer pack from a given player, to check what blocksPack() allows
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
	ASSERT_EQ(attackerActivity->getPlayers().size(), 1);
	EXPECT_EQ(attackerActivity->getPlayers().front(), PlayerColor(0));
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

	// The level-up must exist before the other player is eliminated, otherwise nothing
	// is deferred and this would test the empty case.
	gameHandler.giveExperience(hero, 10);

	auto levelUpActivity = gameHandler.activities->topActivity(levelUpPlayer);
	ASSERT_NE(levelUpActivity, nullptr);
	ASSERT_EQ(levelUpActivity->getType(), ActivityType::HeroLevelUpDialog);

	// The player is mid-level-up, so victory is not applied yet
	gameHandler.checkVictoryLossConditionsForPlayer(defeatedPlayer);
	EXPECT_EQ(gameState()->getPlayerState(defeatedPlayer)->status, EPlayerStatus::LOSER);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::INGAME);

	ASSERT_EQ(gameHandler.activities->submitReply(levelUpActivity->getActiveQuestionID(), levelUpPlayer, 0),
		ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.activities->topActivity(levelUpPlayer), nullptr);
	EXPECT_EQ(gameState()->getPlayerState(levelUpPlayer)->status, EPlayerStatus::WINNER);
}

TEST_F(ActivityProcessorTest, finish_exposesActivityBelowWithRemovedActivityAsArgument)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	topActivity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), bottomActivity);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded,
		ActivityEvent::OnExposure
	}));

	EXPECT_EQ(bottomActivity->onAddedCalls, 1);
	EXPECT_EQ(bottomActivity->onRemovalCalls, 0);

	ASSERT_EQ(bottomActivity->exposureArgs.size(), 1);
	EXPECT_EQ(bottomActivity->exposureArgs[0], topActivity);

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));
}

TEST_F(ActivityProcessorTest, finish_allowsExposedActivityToFinishItself)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	bottomActivity->finishOnExposure = true;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	topActivity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(bottomActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(topActivity.get()), 0);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded,
		ActivityEvent::OnExposure,
		ActivityEvent::OnRemoval
	}));

	ASSERT_EQ(bottomActivity->exposureArgs.size(), 1);
	EXPECT_EQ(bottomActivity->exposureArgs[0], topActivity);

	EXPECT_EQ(bottomActivity->onRemovalCalls, 1);

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));
}

TEST_F(ActivityProcessorTest, finish_removesMultiPlayerActivityOnlyWhereItIsTop)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::ScriptDialog);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	sharedActivity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), blueTopActivity);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(blueTopActivity.get()), 1);

	// Still on the stack of the other player
	EXPECT_EQ(sharedActivity->onRemovalCalls, 0);
}

TEST_F(ActivityProcessorTest, finish_removesMultiPlayerActivityOnceItBecomesTopAgain)
{
	auto sharedActivity = std::make_shared<TestActivity>(
		&gh,
		std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)},
		ActivityType::ScriptDialog);

	auto blueTopActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	activities.addActivity(sharedActivity);
	activities.addActivity(blueTopActivity);

	sharedActivity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), nullptr);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), blueTopActivity);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 1);

	blueTopActivity->finish();

	ASSERT_EQ(sharedActivity->exposureArgs.size(), 1);
	EXPECT_EQ(sharedActivity->exposureArgs[0], blueTopActivity);

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activities.countActivity(blueTopActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(sharedActivity.get()), 0);

	EXPECT_EQ(sharedActivity->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, finish_callsRemovalBeforeExposure)
{
	std::vector<RecordedEvent> eventLog;

	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);

	bottomActivity->sharedEventLog = &eventLog;
	topActivity->sharedEventLog = &eventLog;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	eventLog.clear();

	topActivity->finish();

	ASSERT_EQ(eventLog.size(), 2);
	EXPECT_EQ(eventLog[0].activity, topActivity.get());
	EXPECT_EQ(eventLog[0].event, ActivityEvent::OnRemoval);
	EXPECT_EQ(eventLog[1].activity, bottomActivity.get());
	EXPECT_EQ(eventLog[1].event, ActivityEvent::OnExposure);
}

TEST_F(ActivityProcessorTest, finish_skipsExposureWhenRemovalAddsNewTopActivity)
{
	auto bottomActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::HeroMovement);
	auto topActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::MapObjectVisit);
	auto replacementActivity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::ScriptDialog);

	topActivity->addReplacementOnRemoval = true;
	topActivity->replacementActivity = replacementActivity;

	activities.addActivity(bottomActivity);
	activities.addActivity(topActivity);

	topActivity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), replacementActivity);
	EXPECT_EQ(activities.countActivity(bottomActivity.get()), 1);
	EXPECT_EQ(activities.countActivity(topActivity.get()), 0);
	EXPECT_EQ(activities.countActivity(replacementActivity.get()), 1);

	EXPECT_EQ(bottomActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded
	}));
	EXPECT_TRUE(bottomActivity->exposureArgs.empty());

	EXPECT_EQ(topActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded,
		ActivityEvent::OnRemoval
	}));

	EXPECT_EQ(replacementActivity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded
	}));
}

TEST_F(ActivityProcessorTest, addActivity_addsSameActivityForAllAffectedPlayers)
{
	auto activity = std::make_shared<TestActivity>(&gh, std::initializer_list<PlayerColor>{PlayerColor(0), PlayerColor(1)}, ActivityType::ScriptDialog);

	activities.addActivity(activity);

	EXPECT_EQ(activities.topActivity(PlayerColor(0)), activity);
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), activity);
	EXPECT_EQ(activities.countActivity(activity.get()), 2);

	EXPECT_EQ(activity->events, std::vector<ActivityEvent>({
		ActivityEvent::OnAdded
	}));

	EXPECT_EQ(activity->onAddedCalls, 1);
	EXPECT_EQ(activity->onRemovalCalls, 0);
	EXPECT_TRUE(activity->exposureArgs.empty());
}

TEST_F(ActivityProcessorTest, finish_removesAnActivityOnTopRightAway)
{
	// Finished from outside of any other mutation, e.g. a battle that ends with nobody to
	// ask about the result, so nothing else would settle the stacks afterwards
	auto activity = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::Battle);
	activities.addActivity(activity);

	activity->finish();

	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
	EXPECT_EQ(activity->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, finish_waitsUntilTheActivityIsOnTop)
{
	auto below = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::Battle);
	auto above = std::make_shared<TestActivity>(&gh, PlayerColor(1), ActivityType::TimerPause);
	activities.addActivity(below);
	activities.addActivity(above);

	below->finish();
	EXPECT_EQ(activities.countActivity(below.get()), 1);

	above->finish();
	EXPECT_EQ(activities.topActivity(PlayerColor(1)), nullptr);
}

TEST_F(ActivityProcessorTest, submitReply_rejectsAnAnswerThatWasNotOffered)
{
	const PlayerColor player(1);
	BlockingDialog dialog(true, false);
	dialog.player = player;

	auto activity = std::make_shared<BlockingDialogActivity>(&gh, dialog);
	activities.addActivity(activity);
	const auto questionID = activity->askQuestion();

	// A yes/no question is answered with 0 or 1 only
	EXPECT_EQ(activities.submitReply(questionID, player, 2), ReplyOutcome::RejectedInvalidAnswer);
	EXPECT_EQ(activities.submitReply(questionID, player, -1), ReplyOutcome::RejectedInvalidAnswer);
	EXPECT_EQ(activities.topActivity(player), activity);
	EXPECT_EQ(activities.submitReply(questionID, player, 1), ReplyOutcome::Accepted);
}

TEST_F(ActivityProcessorTest, getActivity_findsAnActivityByTheQuestionItAsked)
{
	auto activity = std::make_shared<TestDialogActivity>(&gh, PlayerColor(1), ActivityType::BlockingDialog);
	const auto question = activity->getActiveQuestionID();

	activities.addActivity(activity);

	EXPECT_EQ(activities.getActivity(question), activity);

	activity->finish();

	EXPECT_EQ(activities.getActivity(question), nullptr);
}


// --------------------------------------------------------------------------------
// Reply routing.
//
// The client is asked about an activity and answers it, but the server may push something
// else in between. These tests drive the processor through those orderings.
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
	// Server pushes something else after the dialog was sent, but before the answer arrives
	activities.addActivity(pushedAfterPrompt);

	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 3), ReplyOutcome::Accepted);

	// The dialog is answered but still buried, so it stays put for now.
	EXPECT_EQ(activities.topActivity(player), pushedAfterPrompt);
	EXPECT_TRUE(dialog->isAnswered());
	EXPECT_EQ(dialog->onRemovalCalls, 0);

	pushedAfterPrompt->finish();

	// Exposing it must resolve it instead of leaving the player waiting forever
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

	// Answers arrive bottom-up, the opposite of the stack order
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
	activity->finish(); // removed by some other event while the reply was in flight

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
	// QuestionID::CLIENT is used by every pause activity, so two players can hold different
	// activities with the same id at the same time.
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
	// ...and still in place for the one that is busy, instead of silently skipped
	EXPECT_EQ(activities.countActivity(shared.get()), 1);
	EXPECT_EQ(activities.topActivity(second), busy);

	busy->finish();

	EXPECT_EQ(activities.topActivity(second), nullptr);
	EXPECT_EQ(activities.countActivity(shared.get()), 0);
}

// --------------------------------------------------------------------------------
// Property test: no ordering of questions and replies may leave a player holding an
// already answered activity.
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
			else if(action == 1) // client replies to some activity it was asked about
			{
				const auto & target = live[rng() % live.size()];
				processor.submitReply(target->getActiveQuestionID(), player, 0);
			}
			else // server removes the top activity for reasons of its own
			{
				if(auto top = processor.topActivity(player))
					processor.finishActivity(*top);
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
			processor.finishActivity(*top);
		}
	}
}


// --------------------------------------------------------------------------------
// Quiescence.
//
// Removing an activity runs hooks that may add or remove further activities, so the stacks
// pass through meaningless intermediate states: briefly empty, or holding an activity that
// is about to be replaced. Deferred work must run from the settled state only.
// --------------------------------------------------------------------------------

TEST_F(ActivityProcessorTest, settle_resolvesRepliesThatArrivedWhileStacksWereMoving)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto cover = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(dialog);
	activities.addActivity(cover);

	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);
	EXPECT_EQ(activities.topActivity(player), cover);

	// Removing the cover exposes an answered activity, which settle() must resolve within
	// the same quiescent point instead of leaving it to a later mutation.
	cover->finish();

	EXPECT_EQ(activities.topActivity(player), nullptr);
	EXPECT_EQ(dialog->onRemovalCalls, 1);
}

TEST_F(ActivityProcessorTest, settle_throwsInsteadOfLoopingForeverWhenDeferredWorkKeepsChanging)
{
	const PlayerColor player(1);

	// A routine that is done on its first step and queues another one as it is removed, so
	// every settle() round finds real work and only the round limit can end the chain.
	std::function<void()> queueAnother = [&]()
	{
		auto next = std::make_shared<TestRoutine>(&gh, player, 0);
		next->onRemovalAction = queueAnother;
		activities.addActivity(next);
	};

	auto first = std::make_shared<TestRoutine>(&gh, player, 0);
	first->onRemovalAction = queueAnother;

	// settle() runs on leaving this call
	EXPECT_THROW(activities.addActivity(first), std::runtime_error);
}

// --------------------------------------------------------------------------------
// Routines.
//
// A routine is a multi-step server-side activity, e.g. an object visit or a town building
// visit, that stops when a step needs the player and continues from where it stopped. The
// processor drives it and the routine keeps its own position instead of reading the stack.
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

	child->finish();

	// Continues from step 2, without restarting or skipping a step
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
	child->finish();

	// Child completion is reported through onChildCompleted only, so that a routine can
	// not implement resumption twice.
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

	grandchild->finish();
	EXPECT_EQ(routine->stepsTaken, 1); // the child is still unfinished

	child->finish();
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

	// Answering the dialog must resolve it and continue the routine within the same
	// quiescent point.
	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(routine->stepsTaken, 3);
	EXPECT_EQ(activities.topActivity(player), nullptr);
}

// --------------------------------------------------------------------------------
// Object visits driven end to end.
//
// The visit routine passes control to the object, which may finish immediately or start a
// battle. These tests use the real pipeline instead of a stand-in routine.
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

	// The object opens a dialog, so the visit suspends instead of finishing
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
		return gameHandler.activities->findVisit(pandora->id) != nullptr;
	};

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	EXPECT_TRUE(visitIsPending());
	EXPECT_EQ(gameHandler.getVisitingHero(pandora), hero);

	// Answering starts a battle one level deeper. The visit must still be below it, so that
	// the object receives the result when the battle ends.
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

	// The visit must stay registered for its whole duration - object code relies on that
	// through removeAfterVisit().
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

	// A reward granted in the after-level-up half of the pipeline, so that granting it
	// twice is visible, plus guards so that the visit goes through a battle.
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

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	ASSERT_EQ(gameHandler.activities->topActivity(player)->getType(), ActivityType::Battle);
	gameHandler.battles->cheatBattleVictory(player);

	// Accept the result instead of replaying the battle
	auto resultDialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(resultDialog, nullptr);
	ASSERT_EQ(resultDialog->getType(), ActivityType::BattleDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(resultDialog->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);

	// The level-up from battle experience is asked above the finished battle, before the
	// object learns about the battle
	ASSERT_EQ(rewardsGranted(), 0u);
	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);

	// The hero may gain several levels at once, each asked about in turn
	int levelUpsAnswered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		if(pending->getType() != ActivityType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++levelUpsAnswered, 10) << "level-up chain did not terminate";
	}

	EXPECT_GE(levelUpsAnswered, 1);

	// The object is told about the battle and not about the level-ups, otherwise
	// experienceApplied() would grant the reward once more
	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

TEST_F(MapObjectVisitTest, levelUpFromABattleWithoutAResultDialogIsAskedBeforeTheObjectLearnsOfTheBattle)
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

	// An AI gets no result dialog, so the battle ends while no reply is being processed
	gameState()->players.at(player).human = false;

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

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

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	ASSERT_EQ(gameHandler.activities->topActivity(player)->getType(), ActivityType::Battle);
	gameHandler.battles->cheatBattleVictory(player);

	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);
	EXPECT_EQ(rewardsGranted(), 0u) << "the object learned of the battle before the level-up";

	int levelUpsAnswered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		if(pending->getType() != ActivityType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
		ASSERT_LT(++levelUpsAnswered, 10) << "level-up chain did not terminate";
	}

	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

// --------------------------------------------------------------------------------
// Locating the battle activity.
//
// A battle activity is one object on both belligerents' stacks, found from either side.
// --------------------------------------------------------------------------------

namespace
{
class TwoPlayerBattleTest : public TinyMapGameTest
{
protected:
	/// Color that is played by an AI, not by a human
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

	// One object on both belligerents' stacks, and only one per player
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
		// Savage is the only disposition with a fixed aggression instead of a rolled one,
		// and a hero this weak can not avoid the fight, so the outcome is deterministic.
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
// A hero can gain several levels from one reward. One routine asks about each of them in
// turn, so the player can not act between levels and the activity below is notified once.
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

TEST_F(LevelUpActivityTest, severalLevelsAreAskedAboutByOneRoutineThatStaysOnTheStack)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	const int levelBefore = hero->level;
	gameHandler.giveExperience(hero, 100000); // worth several levels at once

	auto * routine = gameHandler.activities->findSoleActivity<LevelUpRoutine>(player);
	ASSERT_NE(routine, nullptr);

	std::set<QuestionID> questionsAsked;
	int answers = 0;

	while(auto pending = gameHandler.activities->topActivity(player))
	{
		ASSERT_EQ(pending->getType(), ActivityType::HeroLevelUpDialog);

		// Always the same routine underneath, however many prompts it puts on top
		ASSERT_EQ(gameHandler.activities->findSoleActivity<LevelUpRoutine>(player), routine)
			<< "the chain was driven by more than one routine";

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

	gameHandler.giveExperience(hero, 100000);

	auto * routine = gameHandler.activities->findSoleActivity<LevelUpRoutine>(player);
	ASSERT_NE(routine, nullptr);

	auto prompt = gameHandler.activities->topActivity(player);
	ASSERT_NE(prompt, nullptr);

	const auto firstQuestion = prompt->getActiveQuestionID();
	ASSERT_EQ(gameHandler.activities->submitReply(firstQuestion, player, 0), ReplyOutcome::Accepted);

	// The next level is now being asked about, the previous question is stale
	auto nextPrompt = gameHandler.activities->topActivity(player);
	ASSERT_NE(nextPrompt, nullptr);
	ASSERT_NE(nextPrompt, prompt);
	ASSERT_NE(nextPrompt->getActiveQuestionID(), firstQuestion);

	EXPECT_EQ(gameHandler.activities->submitReply(firstQuestion, player, 0),
		ReplyOutcome::IgnoredAlreadyCompleted);

	// The stale answer consumed neither the outstanding question nor the routine below.
	EXPECT_EQ(gameHandler.activities->topActivity(player), nextPrompt);
	EXPECT_EQ(gameHandler.activities->findSoleActivity<LevelUpRoutine>(player), routine);
}

TEST_F(LevelUpActivityTest, everyQuestionSentToTheClientIsReportedResolved)
{
	const PlayerColor player(0);
	buildHeroAboutToLevel(999);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.giveExperience(hero, 100000); // worth several levels at once

	while(auto pending = gameHandler.activities->topActivity(player))
	{
		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
	}

	// The client keeps its dialog open until the server reports that question as resolved,
	// so every question sent must come back resolved, by the id that the client was given
	// and not by the id of the activity behind it.
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

	// BattleActivity allows GamePause, so a player may pause mid-battle, which puts a
	// TimerPauseActivity on top of the battle activity. The battle activity must still be
	// found: looking only at the top of the stack ended the battle with
	// "Cannot find battle activity!".
	auto pause = std::make_shared<TimerPauseActivity>(&gameHandler, PlayerColor(0));
	gameHandler.activities->addActivity(pause);
	ASSERT_EQ(gameHandler.activities->topActivity(PlayerColor(0)), pause);

	EXPECT_EQ(gameHandler.battles->findBattleActivity(*battle), expected);
}

TEST_F(TwoPlayerBattleTest, battleResultIsAppliedEvenWhenThePlayerPausedMidBattle)
{
	buildTwoPlayerMap();

	auto * attacker = findHeroByOwner(PlayerColor(0));
	auto * defender = findHeroByOwner(PlayerColor(AI_PLAYER));
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	GameHandlerTestServer server(gameState(), PlayerColor(0));
	CGameHandler gameHandler(server, gameState());

	gameHandler.battles->startBattle(attacker, defender);
	ASSERT_NE(gameState()->getBattle(PlayerColor(0)), nullptr);

	// The pause ends up between the battle activity and the result dialog pushed on top of
	// it, so the battle activity is no longer the top one when the result is answered
	gameHandler.activities->addActivity(std::make_shared<TimerPauseActivity>(&gameHandler, PlayerColor(0)));

	gameHandler.battles->cheatBattleVictory(PlayerColor(0));

	auto resultDialog = gameHandler.activities->topActivity(PlayerColor(0));
	ASSERT_NE(gameHandler.activities->activityAs<BattleResultActivity>(resultDialog), nullptr);

	ASSERT_EQ(gameHandler.activities->submitReply(resultDialog->getActiveQuestionID(), PlayerColor(0), 0),
		ReplyOutcome::Accepted);

	EXPECT_EQ(server.battlesConfirmed, 1) << "the battle result was never applied";
}

TEST_F(MapObjectVisitTest, aHeroIsNotAttackedWhileItsOwnerChoosesSkillsAfterDefending)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PlayerColor(0))
		.playerActive(PlayerColor(1))
		.hero(int3(5, 5, 0), HeroTypeID(0), PlayerColor(0))
		.heroGarrison({{CreatureID(0), 10}})
		.hero(int3(6, 5, 0), HeroTypeID(1), PlayerColor(1))
		.heroGarrison({{CreatureID(0), 10}});
	startWithMap(std::move(builder));

	auto * attacker = findHeroByOwner(PlayerColor(0));
	auto * defender = findHeroByOwner(PlayerColor(1));
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	GameHandlerTestServer server(gameState(), PlayerColor(0));
	CGameHandler gameHandler(server, gameState());

	// As after winning a defence: the defender's owner is asked about a level while the attacker still acts
	gameHandler.giveExperience(defender, 100000);
	ASSERT_NE(gameHandler.activities->findSoleActivity<LevelUpRoutine>(PlayerColor(1)), nullptr);

	EXPECT_FALSE(gameHandler.moveHero(attacker->id, defender->anchorPos(), EMovementMode::STANDARD, false, PlayerColor(0), EPathfindingLayer::LAND));
	EXPECT_EQ(gameState()->getBattle(PlayerColor(0)), nullptr);
}

TEST_F(ActivityProcessorTest, settle_completesALongRunOfDeferredWork)
{
	const PlayerColor player(1);

	// A turn start visits every town of a player, each visit running a routine over several
	// buildings. All of it lands in a single settle(), so the round limit must not act as
	// a budget for legitimate work.
	constexpr int deferredItems = 60;
	std::vector<std::shared_ptr<TestRoutine>> queued;

	auto seeder = std::make_shared<TestActivity>(&gh, player, ActivityType::HeroMovement);
	seeder->onRemovalAction = [&]()
	{
		for(int i = 0; i < deferredItems; ++i)
		{
			auto routine = std::make_shared<TestRoutine>(&gh, player, 2);
			queued.push_back(routine);
			activities.addActivity(routine);
		}
	};

	activities.addActivity(seeder);
	seeder->finish(); // everything above is added within this one settle()

	EXPECT_EQ(activities.topActivity(player), nullptr) << "work was left unfinished";

	int unfinished = 0;
	for(const auto & routine : queued)
		if(routine->stepsTaken != 2)
			unfinished++;

	EXPECT_EQ(unfinished, 0) << unfinished << " of " << deferredItems << " routines never ran";
}

TEST_F(ActivityProcessorTest, replyIsAcceptedWhileAVisitSitsOnTop)
{
	const PlayerColor player(1);
	auto dialog = std::make_shared<TestDialogActivity>(&gh, player, ActivityType::BlockingDialog);
	auto visit = std::make_shared<TestActivity>(&gh, player, ActivityType::MapObjectVisit);

	activities.addActivity(dialog);
	activities.addActivity(visit);

	// A visit blocks every action, but an answer is not an action: the reply may be for an
	// activity below the visit.
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

	// Only an activity that the player can cancel may be answered without a value.
	// Accepting it here would resolve the dialog with no answer for the object to use.
	EXPECT_EQ(activities.submitReply(dialog->getActiveQuestionID(), player, std::nullopt),
		ReplyOutcome::RejectedMissingAnswer);
	EXPECT_FALSE(dialog->isAnswered());
	EXPECT_EQ(activities.topActivity(player), dialog);
}

// --------------------------------------------------------------------------------
// Continuation tags.
//
// A reward is granted in two halves, around any level-up caused by its experience. The
// visit carries the id of the reward in progress.
// --------------------------------------------------------------------------------

TEST_F(MapObjectVisitTest, rewardInterruptedByALevelUpIsFinishedFromTheVisitStateNotTheObject)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroExperience(999) // one experience point short of the next level
		.pandora(int3(6, 5, 0));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(pandora, nullptr);

	// Two rewards, of which only the second can be granted, so resuming from anything but
	// the recorded state grants nothing.
	ASSERT_FALSE(pandora->configuration.info.empty());
	pandora->configuration.info.push_back(pandora->configuration.info.at(0));
	pandora->configuration.info.at(0).limiter.heroLevel = 99; // out of reach

	auto & reward = pandora->configuration.info.at(1).reward;
	reward.heroExperience = 5000;
	reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));

	auto rewardsGranted = [&]()
	{
		return hero->getBonuses([](const Bonus * b)
		{
			return b->type == BonusType::MORALE && b->source == BonusSource::OBJECT_TYPE;
		})->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	// The experience in the reward opened a level-up, suspending the reward half way
	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);
	EXPECT_EQ(rewardsGranted(), 0u) << "the second half was granted before the level-up";

	int answered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		if(pending->getType() != ActivityType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
		ASSERT_LT(++answered, 20);
	}

	// Answering it resumes the right reward, exactly once.
	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

TEST_F(MapObjectVisitTest, aRewardWithTooLittleExperienceStillGrantsItsSecondHalf)
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

	// Experience far too little to gain a level, so nothing interrupts the reward
	ASSERT_FALSE(pandora->configuration.info.empty());
	auto & reward = pandora->configuration.info.at(0).reward;
	reward.heroExperience = 1;
	reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));

	auto rewardsGranted = [&]()
	{
		return hero->getBonuses([](const Bonus * b)
		{
			return b->type == BonusType::MORALE && b->source == BonusSource::OBJECT_TYPE;
		})->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	// No level was gained, so the whole reward is applied and the visit is over
	EXPECT_EQ(rewardsGranted(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

TEST_F(MapObjectVisitTest, aRewardSkillThatGrantsALevelEndsTheVisitOnceTheLevelIsChosen)
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

	const auto & skills = LIBRARY->skillh->objects;
	const auto found = std::ranges::find_if(skills, [](const auto & s){ return s->getJsonKey() == "vcmi-test:levelGranting"; });
	ASSERT_NE(found, skills.end());
	const SecondarySkill skill = (*found)->getId();

	// No experience in the reward: the level comes only from learning the skill
	ASSERT_FALSE(pandora->configuration.info.empty());
	auto & reward = pandora->configuration.info.at(0).reward;
	reward = {};
	reward.secondary[skill] = 1;

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());
	const auto levelBefore = hero->level;

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr);
	ASSERT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);

	// A level-up may offer to upgrade the same skill, which grants one more level
	int answered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		ASSERT_EQ(pending->getType(), ActivityType::HeroLevelUpDialog);
		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
		ASSERT_LT(++answered, 10);
	}

	EXPECT_GE(hero->getSecSkillLevel(skill), 1);
	EXPECT_GT(hero->level, levelBefore);
}

/// Records the object that it was notified about on completion.
class NotifyRecordingActivity : public Activity
{
public:
	NotifyRecordingActivity(CGameHandler * gh, PlayerColor player)
		: Activity(gh, ActivityType::BlockingDialog)
	{
		addPlayer(player);
	}

	mutable const IObjectInterface * reportedTo = nullptr;

	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject,
		const CGHeroInstance * visitingHero, const JsonNode & visitState) const override
	{
		reportedTo = visitedObject;
	}
};

TEST_F(MapObjectVisitTest, aTownBuildingVisitReportsToTheBuildingNotTheTown)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.town(int3(8, 8, 0), FactionID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_FALSE(town->rewardableBuildings.empty());

	const auto buildingID = town->rewardableBuildings.begin()->first;
	const auto * building = town->rewardableBuildings.begin()->second.get();
	town->addBuilding(buildingID); // a building can only be visited once it exists
	town->setVisitingHero(hero);   // and only by a hero who is actually in the town

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	auto visit = std::make_shared<TownBuildingVisitActivity>(
		&gameHandler, town, std::vector<const CGHeroInstance *>{hero}, std::vector<BuildingID>{buildingID});

	// This building finishes its visit without asking anything, so the report is driven
	// directly instead of waiting for a dialog that never appears.
	gameHandler.activities->addActivity(visit);

	auto child = std::make_shared<NotifyRecordingActivity>(&gameHandler, player);
	visit->onChildCompleted(child);

	// A dialog opened during a building's visit belongs to the building and not to the
	// town, so the answer is reported to the building.
	EXPECT_EQ(child->reportedTo, static_cast<const IObjectInterface *>(building));
	EXPECT_NE(child->reportedTo, static_cast<const IObjectInterface *>(town));
}

TEST_F(MapObjectVisitTest, aTownEntryLeavesOneVisitIdentifyingTheObjectAndTheHero)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.town(int3(8, 8, 0), FactionID(5), player)  // Dungeon
		.hero(int3(6, 8, 0), HeroTypeID(0), player); // the town entrance
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_EQ(hero->visitablePos(), town->visitablePos());

	// The Battle Scholar Academy grants experience, so the entry stops on a level-up and
	// the stack can still be inspected.
	ASSERT_TRUE(town->rewardableBuildings.count(BuildingID::SPECIAL_4));
	town->addBuilding(BuildingID::SPECIAL_4);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(town, hero);
	ASSERT_NE(gameHandler.activities->topActivity(player), nullptr) << "the entry did not stop anywhere";

	// Entering a town nests a building walk inside the visit, but only the visit says that
	// the town is busy, so the town still names one hero.
	EXPECT_NE(gameHandler.activities->findActivity<TownBuildingVisitActivity>(
		player, [](const TownBuildingVisitActivity &){ return true; }), nullptr)
		<< "no building walk was nested inside the visit";

	EXPECT_EQ(gameHandler.getVisitingHero(town), hero);

	// Answering everything must hand the town back, otherwise it would stay busy forever
	int answered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
		ASSERT_LT(++answered, 20) << "the entry did not finish";
	}

	EXPECT_EQ(gameHandler.activities->findVisit(town->id), nullptr);
	EXPECT_EQ(gameHandler.getVisitingHero(town), nullptr);
}

TEST_F(MapObjectVisitTest, turnStartVisitsWaitForThePlayerToAcceptTheTurn)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.town(int3(8, 8, 0), FactionID(5), player)  // Dungeon
		.hero(int3(6, 8, 0), HeroTypeID(0), player); // the town entrance
	startWithMap(std::move(builder));

	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	ASSERT_EQ(town->getVisitingHero(), findHeroByOwner(player));

	// The Battle Scholar Academy grants experience, so the turn-start visit stops on a level-up
	ASSERT_TRUE(town->rewardableBuildings.count(BuildingID::SPECIAL_4));
	town->addBuilding(BuildingID::SPECIAL_4);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	auto pause = std::make_shared<TimerPauseActivity>(&gameHandler, player);
	const auto questionID = pause->askQuestion();
	gameHandler.activities->addActivity(std::make_shared<TurnStartRoutine>(&gameHandler, player, pause));

	// Accepting the turn comes first - the academy level-up must not overtake it
	EXPECT_EQ(gameHandler.activities->topActivity(player), pause);

	ASSERT_EQ(gameHandler.activities->submitReply(questionID, player, 0), ReplyOutcome::Accepted);

	auto levelUp = gameHandler.activities->topActivity(player);
	ASSERT_NE(levelUp, nullptr) << "the turn-start visit never ran";
	EXPECT_EQ(levelUp->getType(), ActivityType::HeroLevelUpDialog);
}

TEST_F(MapObjectVisitTest, aSeerHutOffersItsNextQuestOnlyAfterTheRewardLevelUp)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder.hotaVersion(3)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(6), player)
		.seerHutMulti(int3(8, 8, 0), {
			{TinyH3M::TinyH3MBuilder::missionLevel(1), TinyH3M::TinyH3MBuilder::rewardExperience(5000)},
			{TinyH3M::TinyH3MBuilder::missionLevel(1), TinyH3M::TinyH3MBuilder::rewardResource(GameResID::WOOD, 7)}});
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * seer = findFirst<SeerHut>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(seer, hero);

	auto completion = gameHandler.activities->topActivity(player);
	ASSERT_NE(completion, nullptr);
	ASSERT_EQ(completion->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(completion->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	// The reward levelled the hero, so it is not fully handed over yet - the next quest
	// must wait for the level-up instead of being stated on top of it
	auto next = gameHandler.activities->topActivity(player);
	ASSERT_NE(next, nullptr);
	EXPECT_EQ(next->getType(), ActivityType::HeroLevelUpDialog) << gameHandler.activities->describeStacks();
}

TEST_F(MapObjectVisitTest, aSeerHutOffersItsNextQuestOnlyAfterTheGarrisonWindowCloses)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder.hotaVersion(3)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(6), player)
		.seerHutMulti(int3(8, 8, 0), {
			{TinyH3M::TinyH3MBuilder::missionLevel(1), TinyH3M::TinyH3MBuilder::rewardNothing()},
			{TinyH3M::TinyH3MBuilder::missionLevel(1), TinyH3M::TinyH3MBuilder::rewardResource(GameResID::WOOD, 7)}});
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * seer = findFirst<SeerHut>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	// Seven different stacks leave no room for an eighth kind, so the reward opens a garrison window
	for(int slot = 0; slot < GameConstants::ARMY_SIZE; ++slot)
		ASSERT_TRUE(hero->setCreature(SlotID(slot), CreatureID(slot), 1));
	seer->configuration.info.at(0).reward.creatures.emplace_back(CreatureID(10), 5);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(seer, hero);

	auto completion = gameHandler.activities->topActivity(player);
	ASSERT_NE(completion, nullptr);
	ASSERT_EQ(completion->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(completion->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	auto garrison = gameHandler.activities->topActivity(player);
	ASSERT_NE(garrison, nullptr);
	ASSERT_EQ(garrison->getType(), ActivityType::GarrisonDialog) << gameHandler.activities->describeStacks();
	ASSERT_EQ(gameHandler.activities->submitReply(garrison->getActiveQuestionID(), player, 0),
		ReplyOutcome::Accepted);

	// With the reward handed over, the seer states his next quest within the same visit
	auto next = gameHandler.activities->topActivity(player);
	ASSERT_NE(next, nullptr);
	EXPECT_EQ(next->getType(), ActivityType::BlockingDialog) << gameHandler.activities->describeStacks();
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

TEST_F(MapObjectVisitTest, creaturesRefusedAsRecruitsAskWhetherToLetThemFlee)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		// Compliant is the one disposition that always offers to join for free
		.monster(int3(6, 5, 0), CreatureID(0), 10,
			static_cast<int8_t>(CGCreature::Character::COMPLIANT));
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * monster = findFirst<CGCreature>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(monster, nullptr);
	const auto monsterID = monster->id;

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(monster, hero);

	auto offer = gameHandler.activities->topActivity(player);
	ASSERT_NE(offer, nullptr);
	ASSERT_EQ(offer->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(offer->getActiveQuestionID(), player, 0),
		ReplyOutcome::Accepted);

	// Refused, they would still join if asked anew - the question now is whether to pursue them
	auto flee = gameHandler.activities->topActivity(player);
	ASSERT_NE(flee, nullptr);
	ASSERT_EQ(flee->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->findVisit(monsterID)->visitState, JsonNode(static_cast<int32_t>(CGCreature::FLEE)));
	ASSERT_EQ(gameHandler.activities->submitReply(flee->getActiveQuestionID(), player, 0),
		ReplyOutcome::Accepted);

	EXPECT_EQ(gameState()->getObjInstance(monsterID), nullptr) << "the creatures were let go";
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
}

TEST_F(MapObjectVisitTest, aPandoraRewardChoiceIsNotTakenForOpeningTheBoxAgain)
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

	// Two rewards that the player has to choose between once the box is open
	ASSERT_FALSE(pandora->configuration.info.empty());
	pandora->configuration.info.resize(1);
	pandora->configuration.info.push_back(pandora->configuration.info.at(0));
	pandora->configuration.selectMode = Rewardable::SELECT_PLAYER;
	pandora->configuration.info.at(0).reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));
	pandora->configuration.info.at(1).reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));

	auto granted = [&](BonusType type)
	{
		return hero->getBonuses([type](const Bonus * b){ return b->type == type && b->source == BonusSource::OBJECT_TYPE; })->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);

	auto open = gameHandler.activities->topActivity(player);
	ASSERT_NE(open, nullptr);
	ASSERT_EQ(gameHandler.activities->submitReply(open->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	auto choice = gameHandler.activities->topActivity(player);
	ASSERT_NE(choice, nullptr);
	ASSERT_EQ(choice->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(choice->getActiveQuestionID(), player, 2), ReplyOutcome::Accepted);

	EXPECT_EQ(granted(BonusType::LUCK), 1u);
	EXPECT_EQ(granted(BonusType::MORALE), 0u);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr) << gameHandler.activities->describeStacks();
}

TEST_F(MapObjectVisitTest, aRewardChoiceGrantsWhatItOfferedNotTheFirstAvailable)
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

	// A random pick that may be refused: the question names one reward out of both available
	ASSERT_FALSE(pandora->configuration.info.empty());
	pandora->configuration.info.resize(1);
	pandora->configuration.info.push_back(pandora->configuration.info.at(0));
	pandora->configuration.selectMode = Rewardable::SELECT_RANDOM;
	pandora->configuration.canRefuse = true;
	pandora->configuration.info.at(0).reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));
	pandora->configuration.info.at(1).reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));

	auto granted = [&](BonusType type)
	{
		return hero->getBonuses([type](const Bonus * b){ return b->type == type && b->source == BonusSource::OBJECT_TYPE; })->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);

	auto open = gameHandler.activities->topActivity(player);
	ASSERT_NE(open, nullptr);
	ASSERT_EQ(gameHandler.activities->submitReply(open->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	auto offer = gameHandler.activities->topActivity(player);
	ASSERT_NE(offer, nullptr);
	ASSERT_EQ(offer->getType(), ActivityType::BlockingDialog);

	auto & offered = gameHandler.activities->findVisit(pandora->id)->visitState;
	ASSERT_TRUE(offered.isVector());
	ASSERT_EQ(offered.Vector().size(), 1u);

	// Stands in for the random pick landing on the second reward, which it does only sometimes
	offered.Vector().at(0) = JsonNode(1u);

	ASSERT_EQ(gameHandler.activities->submitReply(offer->getActiveQuestionID(), player, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(granted(BonusType::LUCK), 1u);
	EXPECT_EQ(granted(BonusType::MORALE), 0u) << "the first available reward was granted instead of the offered one";
}

TEST_F(MapObjectVisitTest, grantingAllRewardsContinuesPastALevelUpAndAGarrisonWindow)
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

	// Seven different stacks leave no room for an eighth kind, so the second reward opens a garrison window
	for(int slot = 0; slot < GameConstants::ARMY_SIZE; ++slot)
		ASSERT_TRUE(hero->setCreature(SlotID(slot), CreatureID(slot), 1));

	auto & info = pandora->configuration.info;
	ASSERT_FALSE(info.empty());
	info.resize(3, info.front());
	info[0].reward = {};
	info[0].reward.heroExperience = 5000;
	info[1].reward = {};
	info[1].reward.creatures.emplace_back(CreatureID(10), 5);
	info[2].reward = {};
	info[2].reward.resources[GameResID::WOOD] = 7;
	pandora->configuration.selectMode = Rewardable::SELECT_ALL;

	const auto woodBefore = gameState()->players.at(player).resources[GameResID::WOOD];

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(pandora, hero);

	auto dialog = gameHandler.activities->topActivity(player);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(dialog->getType(), ActivityType::BlockingDialog);
	ASSERT_EQ(gameHandler.activities->submitReply(dialog->getActiveQuestionID(), player, 1),
		ReplyOutcome::Accepted);

	auto pending = gameHandler.activities->topActivity(player);
	ASSERT_NE(pending, nullptr);
	ASSERT_EQ(pending->getType(), ActivityType::HeroLevelUpDialog) << gameHandler.activities->describeStacks();
	while(pending && pending->getType() == ActivityType::HeroLevelUpDialog)
	{
		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
		pending = gameHandler.activities->topActivity(player);
	}

	ASSERT_NE(pending, nullptr);
	ASSERT_EQ(pending->getType(), ActivityType::GarrisonDialog) << gameHandler.activities->describeStacks();
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::WOOD], woodBefore);
	ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
		ReplyOutcome::Accepted);

	// The reward after the garrison window is still granted, and the visit is over
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::WOOD], woodBefore + 7);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr) << gameHandler.activities->describeStacks();
}

TEST_F(MapObjectVisitTest, anExchangeBetweenAlliesLetsOnlyTheInitiatorCloseIt)
{
	const PlayerColor red(0);
	const PlayerColor blue(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.playerActive(blue)
		.hero(int3(5, 5, 0), HeroTypeID(0), red)
		.heroGarrison({{CreatureID(0), 10}, {CreatureID(1), 10}})
		.hero(int3(6, 5, 0), HeroTypeID(1), blue)
		.heroGarrison({{CreatureID(2), 10}});
	startWithMap(std::move(builder));

	auto * redHero = findHeroByOwner(red);
	auto * blueHero = findHeroByOwner(blue);
	ASSERT_NE(redHero, nullptr);
	ASSERT_NE(blueHero, nullptr);

	const TeamID team = gameState()->players.at(red).team;
	gameState()->players.at(blue).team = team;
	gameState()->teams.at(team).players.insert(blue);
	ASSERT_EQ(gameState()->getPlayerRelations(red, blue), PlayerRelations::ALLIES);

	// Only red acts, blue trades out of turn
	gameState()->actingPlayers = {red};

	GameHandlerTestServer server(gameState(), red);
	CGameHandler gameHandler(server, gameState());
	gameHandler.turnTimerHandler->setTimerEnabled(blue, true);
	EXPECT_FALSE(gameHandler.isAllowedExchange(blue, blueHero->id, blueHero->id));

	gameHandler.heroExchange(redHero->id, blueHero->id);

	// One exchange on both stacks, so that both stay in it until red closes it
	auto exchange = gameHandler.activities->topActivity(red);
	ASSERT_NE(exchange, nullptr);
	EXPECT_EQ(exchange->getType(), ActivityType::GarrisonDialog);
	EXPECT_EQ(gameHandler.activities->topActivity(blue), exchange);
	EXPECT_TRUE(gameHandler.isAllowedExchange(blue, redHero->id, blueHero->id));
	EXPECT_TRUE(gameHandler.isAllowedExchange(blue, blueHero->id, blueHero->id));
	EXPECT_FALSE(gameHandler.turnTimerHandler->isTimerEnabled(blue));

	// Either side may give, neither may take
	const SlotID freeSlot(5);
	EXPECT_TRUE(gameHandler.arrangeStacks(redHero->id, blueHero->id, 1, SlotID(1), freeSlot, 0, red));
	EXPECT_EQ(blueHero->getCreature(freeSlot), CreatureID(1).toCreature());
	EXPECT_FALSE(gameHandler.arrangeStacks(blueHero->id, redHero->id, 1, SlotID(0), freeSlot, 0, red));
	EXPECT_FALSE(gameHandler.arrangeStacks(redHero->id, blueHero->id, 1, SlotID(0), SlotID(6), 0, blue));
	EXPECT_EQ(blueHero->getCreature(SlotID(6)), nullptr);

	EXPECT_EQ(gameHandler.activities->submitReply(exchange->getActiveQuestionID(), blue, 0), ReplyOutcome::RejectedWrongPlayer);
	EXPECT_EQ(gameHandler.activities->topActivity(blue), exchange);

	// An artifact that blue still holds when red closes the window goes back to its hero
	ASSERT_TRUE(gameHandler.giveHeroNewArtifact(blueHero, ArtifactID(7), ArtifactPosition::TRANSITION_POS));
	ASSERT_NE(blueHero->getArt(ArtifactPosition::TRANSITION_POS), nullptr);

	ASSERT_EQ(gameHandler.activities->submitReply(exchange->getActiveQuestionID(), red, 0), ReplyOutcome::Accepted);
	EXPECT_EQ(gameHandler.activities->topActivity(red), nullptr);
	EXPECT_EQ(gameHandler.activities->topActivity(blue), nullptr);
	EXPECT_TRUE(gameHandler.turnTimerHandler->isTimerEnabled(blue));
	EXPECT_EQ(blueHero->getArt(ArtifactPosition::TRANSITION_POS), nullptr);
	EXPECT_FALSE(gameHandler.isAllowedExchange(blue, redHero->id, blueHero->id));
	EXPECT_FALSE(gameHandler.isAllowedExchange(red, redHero->id, blueHero->id));
}

TEST_F(MapObjectVisitTest, anAllyBusyWithSomethingElseIsLeftOutOfTheExchange)
{
	const PlayerColor red(0);
	const PlayerColor blue(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.playerActive(blue)
		.hero(int3(5, 5, 0), HeroTypeID(0), red)
		.hero(int3(6, 5, 0), HeroTypeID(1), blue);
	startWithMap(std::move(builder));

	auto * redHero = findHeroByOwner(red);
	auto * blueHero = findHeroByOwner(blue);
	ASSERT_NE(redHero, nullptr);
	ASSERT_NE(blueHero, nullptr);

	const TeamID team = gameState()->players.at(red).team;
	gameState()->players.at(blue).team = team;
	gameState()->teams.at(team).players.insert(blue);

	GameHandlerTestServer server(gameState(), red);
	CGameHandler gameHandler(server, gameState());

	// Blue is in the middle of their own interaction
	auto busy = std::make_shared<TestActivity>(&gameHandler, blue, ActivityType::GarrisonDialog);
	gameHandler.activities->addActivity(busy);

	gameHandler.heroExchange(redHero->id, blueHero->id);

	auto exchange = gameHandler.activities->topActivity(red);
	ASSERT_NE(exchange, nullptr);
	EXPECT_EQ(exchange->getType(), ActivityType::GarrisonDialog);
	EXPECT_EQ(exchange->getPlayers().size(), 1u);
	EXPECT_EQ(gameHandler.activities->topActivity(blue), busy);
}

TEST_F(MapObjectVisitTest, turnStartEventsRunOnceThePlayerAcceptedTheTurn)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	// A scenario event of the day. It runs from the turn start routine, so it must wait
	// for the pause just like the visits do.
	CMapEvent event;
	event.players.insert(player);
	event.humanAffected = true;
	event.nextOccurrence = 1; // recurring daily, so that it also fires on the very first day
	event.resources[GameResID::GOLD] = 1000;
	map()->events.push_back(event);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	const auto goldBefore = gameState()->getPlayerState(player)->resources[GameResID::GOLD];

	auto pause = std::make_shared<TimerPauseActivity>(&gameHandler, player);
	const auto questionID = pause->askQuestion();
	gameHandler.activities->addActivity(std::make_shared<TurnStartRoutine>(&gameHandler, player, pause));

	EXPECT_EQ(gameState()->getPlayerState(player)->resources[GameResID::GOLD], goldBefore)
		<< "the event was applied before the player accepted the turn";

	ASSERT_EQ(gameHandler.activities->submitReply(questionID, player, 0), ReplyOutcome::Accepted);

	EXPECT_EQ(gameState()->getPlayerState(player)->resources[GameResID::GOLD], goldBefore + 1000);
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr) << "the routine did not finish";
}

TEST_F(MapObjectVisitTest, aDialogOpenedBeforeTheFirstBuildingIsReportedToTheTown)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.town(int3(8, 8, 0), FactionID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_FALSE(town->rewardableBuildings.empty());

	const auto buildingID = town->rewardableBuildings.begin()->first;
	town->addBuilding(buildingID);
	town->setVisitingHero(hero);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	// Not added, so no building has been visited yet - the state the town is in while it
	// queues its building visits and then opens its own dialog, which lands above them.
	auto visit = std::make_shared<TownBuildingVisitActivity>(
		&gameHandler, town, std::vector<const CGHeroInstance *>{hero}, std::vector<BuildingID>{buildingID});

	auto child = std::make_shared<NotifyRecordingActivity>(&gameHandler, player);
	visit->onChildCompleted(child);

	// The dialog was opened by the town, so dropping the answer would silently lose it
	EXPECT_EQ(child->reportedTo, static_cast<const IObjectInterface *>(town));
}

TEST_F(MapObjectVisitTest, aBuildingRewardInterruptedByALevelUpResumesTheBuildingsOwnReward)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroExperience(999) // one experience point short of the next level
		.town(int3(8, 8, 0), FactionID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_FALSE(town->rewardableBuildings.empty());

	const auto buildingID = town->rewardableBuildings.begin()->first;
	auto * building = town->rewardableBuildings.begin()->second.get();
	town->addBuilding(buildingID);
	hero->setAnchorPos(town->visitablePos() + hero->getVisitableOffset()); // stand in the town

	// Two rewards of which only the second can be granted. The tag is written while the
	// town's own visit sits below the building's, so a search from the wrong end of the
	// stack resumes reward 0 and nothing is granted at all.
	auto & info = building->configuration.info;
	ASSERT_FALSE(info.empty());
	info.push_back(info.at(0));
	info.at(0).limiter.heroLevel = 99; // out of reach

	auto & reward = info.at(1).reward;
	reward.heroExperience = 5000;
	reward.heroBonuses.push_back(
		std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OBJECT_TYPE, 1, BonusSourceID()));

	auto rewardsGranted = [&]()
	{
		return hero->getBonuses([](const Bonus * b)
		{
			return b->type == BonusType::MORALE && b->source == BonusSource::OBJECT_TYPE;
		})->size();
	};

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.objectVisited(town, hero);

	int answered = 0;
	while(auto pending = gameHandler.activities->topActivity(player))
	{
		if(pending->getType() != ActivityType::HeroLevelUpDialog)
			break;

		ASSERT_EQ(gameHandler.activities->submitReply(pending->getActiveQuestionID(), player, 0),
			ReplyOutcome::Accepted);
		ASSERT_LT(++answered, 20);
	}

	EXPECT_GT(answered, 0) << "the reward granted no experience, so nothing interrupted it";
	EXPECT_EQ(rewardsGranted(), 1u);
}

// --------------------------------------------------------------------------------
// Casts that pause to ask something.
//
// A town portal style spell stops to ask which town to teleport to, and an activity
// holding ids completes the cast once the player answers.
// --------------------------------------------------------------------------------

namespace
{
class TownPortalTest : public TinyMapGameTest
{
};
}

TEST_F(TownPortalTest, castingPausesToAskWhichTownAndFinishesAtTheChosenOne)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroSpells({SpellID(SpellID::decode("core:townPortal"))})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID(ArtifactID::SPELLBOOK)}})
		.heroPrimary(0, 0, 20, 20) // enough spell power and knowledge for the mana
		.heroSecondarySkills({{SecondarySkill(SecondarySkill::EARTH_MAGIC), 3}})
		.town(int3(20, 20, 0), FactionID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	// Cast with no destination, which makes the spell ask
	gameHandler.castSpell(hero, SpellID(SpellID::decode("core:townPortal")), int3(-1, -1, -1));

	auto asking = gameHandler.activities->topActivity(player);
	ASSERT_NE(asking, nullptr) << "the cast did not stop to ask";
	EXPECT_EQ(asking->getType(), ActivityType::TownSelection);

	const auto startedAt = hero->visitablePos();
	ASSERT_EQ(gameHandler.activities->submitReply(asking->getActiveQuestionID(), player, town->id.getNum()),
		ReplyOutcome::Accepted);

	// The answer completed the cast, from ids alone
	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
	EXPECT_NE(hero->visitablePos(), startedAt);
	EXPECT_EQ(hero->visitablePos(), town->visitablePos());
}

TEST_F(TownPortalTest, closingTheTownWindowAbandonsTheCast)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero(int3(5, 5, 0), HeroTypeID(0), player)
		.heroSpells({SpellID(SpellID::decode("core:townPortal"))})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID(ArtifactID::SPELLBOOK)}})
		.heroPrimary(0, 0, 20, 20)
		.heroSecondarySkills({{SecondarySkill(SecondarySkill::EARTH_MAGIC), 3}})
		.town(int3(20, 20, 0), FactionID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	gameHandler.castSpell(hero, SpellID(SpellID::decode("core:townPortal")), int3(-1, -1, -1));

	auto asking = gameHandler.activities->topActivity(player);
	ASSERT_NE(asking, nullptr);

	const auto startedAt = hero->visitablePos();
	ASSERT_EQ(gameHandler.activities->submitReply(asking->getActiveQuestionID(), player, std::nullopt),
		ReplyOutcome::Accepted);

	EXPECT_EQ(gameHandler.activities->topActivity(player), nullptr);
	EXPECT_EQ(hero->visitablePos(), startedAt) << "the hero travelled without a town being chosen";
}
