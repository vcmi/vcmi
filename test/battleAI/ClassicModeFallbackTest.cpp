/*
 * ClassicModeFallbackTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../server/battles/BattleTestFixture.h"

#include "AI/BattleAI/BattleAI.h"
#include "AI/BattleAI/Classic/ClassicBattleDecision.h"
#include "AI/BattleAI/Classic/ClassicBattleRng.h"
#include "lib/battle/BattleAction.h"
#include "lib/callback/CBattleCallback.h"
#include "server/CGameHandler.h"

namespace
{
class RecordingModeCallback : public CBattleCallback
{
public:
	std::vector<BattleAction> actions;

	RecordingModeCallback()
		: CBattleCallback(PlayerColor(0), nullptr)
	{
	}

	void battleMakeSpellAction(const BattleID &, const BattleAction & action) override
	{
		actions.push_back(action);
	}

	void battleMakeUnitAction(const BattleID &, const BattleAction & action) override
	{
		actions.push_back(action);
	}

	void battleMakeTacticAction(const BattleID &, const BattleAction & action) override
	{
		actions.push_back(action);
	}

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}
};

class ClassicModeFallbackTest : public BattleTestFixture
{
protected:
	std::shared_ptr<RecordingModeCallback> callback;
	std::unique_ptr<CBattleAI> ai;
	CStack * active = nullptr;

	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->tacticDistance = 0;
		battle()->stacks.clear();
		battle()->obstacles.clear();
		active = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 10);
		addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 10);
		callback = std::make_shared<RecordingModeCallback>();
		callback->onBattleStarted(battle());

		BattleAISettings settings;
		settings.mode = BattleAIMode::CLASSIC;
		ai = std::make_unique<CBattleAI>(settings);
		AutocombatPreferences preferences;
		preferences.enableSpellsUsage = false;
		preferences.enableTacticsUsage = false;
		ai->initBattleInterface(gameHandler, callback, preferences);
	}

	void TearDown() override
	{
		ai.reset();
		callback.reset();
		BattleTestFixture::TearDown();
	}

	void notifyBattleStart()
	{
		ai->battleStart(BattleID(0), nullptr, nullptr, int3(), nullptr, nullptr, BattleSide::ATTACKER, false);
	}

	CStack * addModdedStack()
	{
		return addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"),
			BattleHex(rightHex + 2), 1);
	}

	void removeStack(const CStack * stack)
	{
		BattleUnitsChanged removal;
		removal.battleID = BattleID(0);
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(removal);
	}
};

TEST_F(ClassicModeFallbackTest, UnsupportedStartingArmyUsesModernDecisions)
{
	addModdedStack();
	notifyBattleStart();
	EXPECT_FALSE(ai->isClassicMode());
	EXPECT_NO_THROW(ai->activeStack(BattleID(0), active));
	ASSERT_EQ(callback->actions.size(), 1u);
	EXPECT_NE(callback->actions.back().actionType, EActionType::NO_ACTION);
}

TEST_F(ClassicModeFallbackTest, UnsupportedSummonFallsBackUntilNextBattle)
{
	notifyBattleStart();
	ASSERT_TRUE(ai->isClassicMode());
	const CStack * summoned = addModdedStack();
	EXPECT_NO_THROW(ai->activeStack(BattleID(0), active));
	EXPECT_FALSE(ai->isClassicMode());
	ASSERT_EQ(callback->actions.size(), 1u);

	removeStack(summoned);
	ai->activeStack(BattleID(0), active);
	EXPECT_FALSE(ai->isClassicMode());
	ASSERT_EQ(callback->actions.size(), 2u);

	ai->battleEnd(BattleID(0), nullptr, QueryID());
	notifyBattleStart();
	EXPECT_TRUE(ai->isClassicMode());
	EXPECT_NO_THROW(ai->activeStack(BattleID(0), active));
	EXPECT_EQ(callback->actions.size(), 3u);
}

TEST_F(ClassicModeFallbackTest, FallbackSupportsTacticsAndActionFinished)
{
	addModdedStack();
	battle()->tacticDistance = 2;
	battle()->tacticsSide = BattleSide::ATTACKER;
	notifyBattleStart();
	ASSERT_FALSE(ai->isClassicMode());
	ai->yourTacticPhase(BattleID(0), 2);
	ASSERT_EQ(callback->actions.size(), 1u);
	EXPECT_EQ(callback->actions.back().actionType, EActionType::END_TACTIC_PHASE);
	EXPECT_NO_THROW(ai->actionFinished(BattleID(0), callback->actions.back()));
}

TEST_F(ClassicModeFallbackTest, UnsupportedCreatureInActionFinishedSwitchesMode)
{
	notifyBattleStart();
	ASSERT_TRUE(ai->isClassicMode());
	addModdedStack();
	ai->actionFinished(BattleID(0), BattleAction::makeDefend(active));
	EXPECT_FALSE(ai->isClassicMode());
	EXPECT_NO_THROW(ai->activeStack(BattleID(0), active));
	EXPECT_EQ(callback->actions.size(), 1u);
}

TEST_F(ClassicModeFallbackTest, DirectDecisionRejectsUnsupportedCreatures)
{
	addModdedStack();
	std::shared_ptr<CBattleInfoCallback> battleView(battle(), [](CBattleInfoCallback *) {});
	EXPECT_FALSE(ClassicBattleDecision::supportsBattle(*battleView));
	EXPECT_THROW(ClassicBattleDecision::decide(
		gameHandler.get(), battleView, BattleSide::ATTACKER, active, 2, AutocombatPreferences(),
		std::make_shared<ClassicBattleAIRng>(), nullptr), std::domain_error);
}
}
