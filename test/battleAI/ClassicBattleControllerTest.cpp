/*
 * ClassicBattleControllerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"

#include "../../AI/BattleAI/Classic/ClassicBattleController.h"
#include "../../AI/BattleAI/Classic/ClassicBattleRng.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/callback/IClient.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../server/CGameHandler.h"
#include "../../server/battles/BattleProcessor.h"
#include "../server/battles/BattleTestFixture.h"

namespace
{
class ControllerTestRng final : public IClassicBattleAIRng
{
public:
	int32_t nextIntInclusive(int32_t lower, int32_t upper) override
	{
		return upper;
	}
};

class ControllerTestClient final : public IClient
{
public:
	std::vector<BattleAction> actions;

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		const auto * action = dynamic_cast<const MakeAction *>(&request);
		if(action)
			actions.push_back(action->ba);
		return static_cast<int>(actions.size());
	}
};
}

class ClassicBattleControllerTest : public BattleTestFixture
{
protected:
	ControllerTestClient client;
	std::shared_ptr<CBattleCallback> callback;
	std::unique_ptr<ClassicBattleController> controller;
	AutocombatPreferences preferences;

	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();

		BattleUnitsChanged removal;
		removal.battleID = BattleID(0);
		for(const auto * stack : battle()->battleGetAllStacks(false))
			removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(removal);

		battle()->tacticDistance = 7;
		battle()->tacticsSide = BattleSide::ATTACKER;
		callback = std::make_shared<CBattleCallback>(PlayerColor(0), &client);
		callback->onBattleStarted(battle());
		controller = std::make_unique<ClassicBattleController>(
			std::static_pointer_cast<Environment>(gameHandler), callback, PlayerColor(0),
			std::make_shared<ControllerTestRng>(), nullptr);
		controller->battleStart(BattleID(0), BattleSide::ATTACKER);
		preferences.enableSpellsUsage = false;
		preferences.enableTacticsUsage = true;
	}

	void TearDown() override
	{
		controller.reset();
		callback.reset();
		BattleTestFixture::TearDown();
	}
};

TEST_F(ClassicBattleControllerTest, ShooterDeploymentUsesTheClassicDecision)
{
	auto * first = addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(18), 20);
	addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(52), 20);
	addStack(BattleSide::DEFENDER, CreatureID::ARCHER, BattleHex(19), 40);
	const BattleAction expected = controller->decideStackAction(BattleID(0), first, preferences);
	ASSERT_EQ(expected.actionType, EActionType::WALK);

	controller->yourTacticPhase(BattleID(0), 7, preferences);
	ASSERT_EQ(client.actions.size(), 1u);
	const BattleAction actual = client.actions.back();
	EXPECT_EQ(actual.actionType, expected.actionType);
	EXPECT_EQ(actual.stackNumber, expected.stackNumber);
	ASSERT_EQ(actual.target.size(), expected.target.size());
	EXPECT_EQ(actual.target.front().hexValue, expected.target.front().hexValue);
	EXPECT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), actual));
}

TEST_F(ClassicBattleControllerTest, MeleeDeploymentUsesTheClassicDecision)
{
	auto * first = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(86), 20);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(100), 40);
	const BattleAction expected = controller->decideStackAction(BattleID(0), first, preferences);
	ASSERT_EQ(expected.actionType, EActionType::WALK);

	controller->yourTacticPhase(BattleID(0), 7, preferences);
	ASSERT_EQ(client.actions.size(), 1u);
	const BattleAction actual = client.actions.back();
	EXPECT_EQ(actual.actionType, expected.actionType);
	ASSERT_EQ(actual.target.size(), expected.target.size());
	EXPECT_EQ(actual.target.front().hexValue, expected.target.front().hexValue);
	EXPECT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), actual));
}

TEST_F(ClassicBattleControllerTest, LateTacticsCompletionDoesNotIssueACombatAction)
{
	addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(18), 20);
	addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(52), 20);
	addStack(BattleSide::DEFENDER, CreatureID::ARCHER, BattleHex(19), 40);
	controller->yourTacticPhase(BattleID(0), 7, preferences);
	ASSERT_EQ(client.actions.size(), 1u);
	const BattleAction move = client.actions.back();
	ASSERT_EQ(move.actionType, EActionType::WALK);

	battle()->tacticDistance = 0;
	controller->actionFinished(BattleID(0), move);
	EXPECT_EQ(client.actions.size(), 1u);
}
