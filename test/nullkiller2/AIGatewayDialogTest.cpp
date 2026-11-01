/*
 * AIGatewayDialogTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/AsyncRunner.h"
#include "lib/callback/IClient.h"
#include "lib/mapObjects/CRewardableObject.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

/// Records answers that AI sends to the server
class AnswerRecordingClient : public IClient
{
public:
	std::vector<QuestionAnswer> answers;

	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(const auto * answer = dynamic_cast<const QuestionAnswer *>(&request))
			answers.push_back(*answer);
		return static_cast<int>(answers.size());
	}
};

class Nullkiller2_AIGatewayDialog : public NullkillerTest
{
};
}

/// Dialogs such as commander resurrection are not caused by a visit, so AI has no target tile for them
TEST_F(Nullkiller2_AIGatewayDialog, answersDialogWithoutTargetTile)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);

	gateway->showBlockingDialog("resurrect commander?", {}, QuestionID(55), 0, false, true, false);
	gateway->asyncTasks->wait();

	ASSERT_EQ(client.answers.size(), 1u);
	EXPECT_EQ(client.answers.front().questionID, QuestionID(55));
	EXPECT_EQ(client.answers.front().reply, 1);

	// server would resolve the question once it applies the answer
	gateway->status.removeQuestion(QuestionID(55));
}

TEST_F(Nullkiller2_AIGatewayDialog, rejectsKnownBankFightWithExcessivePredictedLoss)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER).heroGarrison({{CreatureID::ARCHER, 100}})
		.hero({10, 5, 0}, HeroTypeID(1), PLAYER).heroGarrison({{CreatureID::ARCHER, 100}})
		.town({15, 15, 0}, FactionID::CASTLE, PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	const auto callback = makeCallback(PLAYER, &client);
	auto * hero = findHeroAt({5, 5, 0});
	ASSERT_NE(hero, nullptr);
	JsonNode guardsVisible;
	guardsVisible.Bool() = true;
	map()->overrideGameSetting(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION, guardsVisible);

	auto bank = std::make_shared<CRewardableObject>(callback.get());
	bank->ID = Obj::CREATURE_BANK;
	bank->subID = MapObjectSubID(1);
	bank->instanceName = "knownBankDialog";
	bank->appearance = hero->appearance;
	bank->setAnchorPos({6, 5, 0});
	auto & visit = bank->configuration.info.emplace_back();
	visit.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	visit.reward.guards.emplace_back(CreatureID(CreatureID::decode("dwarf")), 1);
	bank->initializeGuards();
	const auto singleGuardStrength = bank->estimateCombatValue();
	ASSERT_GT(singleGuardStrength, 0);
	const auto guardCount = std::max<int>(1, std::lround(NK2AI::getHeroFightingStrength(hero, hero) * 0.8 / singleGuardStrength));
	bank->setStackCount(SlotID(0), guardCount);
	map()->addNewObject(bank);
	revealMap(PLAYER);
	gameState()->teams.at(gameState()->players.at(PLAYER).team).scoutedObjects.insert(bank->id);

	auto gateway = makeGateway(callback);
	gateway->nullkiller->armyManager->update();
	preparePlanning(*gateway);
	const auto danger = gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get());
	const auto visibleInfo = callback->getRewardableObjectInfo(bank.get(), hero);
	ASSERT_TRUE(visibleInfo);
	ASSERT_TRUE(visibleInfo->guarded);
	ASSERT_EQ(danger, visibleInfo->guardStrength);
	const auto armyStrength = NK2AI::getHeroArmyStrengthWithCommander(hero, hero);
	const auto predictedLoss = gateway->nullkiller->pathfinder->getStorage()->evaluateArmyLoss(hero, armyStrength, danger);
	ASSERT_TRUE(NK2AI::isSafeToVisit(hero, danger, gateway->nullkiller->settings->getSafeAttackRatio()));
	ASSERT_GT(predictedLoss, hero->estimateCombatValue() * NK2AI::CREATURE_BANK_MAX_ARMY_LOSS_RATIO);
	ASSERT_LT(static_cast<float>(predictedLoss) / hero->estimateCombatValue(),
		NK2AI::evaluateMaxArmyLossRatio(gateway->nullkiller->settings->getMaxArmyLossTarget(),
			NK2AI::evaluateArmyPowerRatio(*gateway->nullkiller->armyManager, hero),
			gameState()->players.at(PLAYER).daysWithoutCastle.has_value()));

	gateway->nullkiller->setActive(hero, bank->visitablePos());
	gateway->nullkiller->setTargetObject(bank->id.getNum());
	gateway->showBlockingDialog("Fight guards?", {}, QuestionID(56), 0, false, true, false);
	gateway->asyncTasks->wait();
	ASSERT_EQ(client.answers.size(), 1u);
	EXPECT_EQ(client.answers.front().reply, 0);
	gateway->status.removeQuestion(QuestionID(56));
}
