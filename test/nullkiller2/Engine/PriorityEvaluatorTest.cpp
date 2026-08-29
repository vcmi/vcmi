/*
 * PriorityEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/ArmyManager.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"
#include "AI/Nullkiller2/Goals/Composition.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Markers/ExplorationPoint.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CRewardableObject.h"

#include "mock/TinyH3MBuilder.h"
#include "mock/TinyMapGameTest.h"
#include "nullkiller2/NullkillerTest.h"

namespace
{
const PlayerColor PLAYER(0);

TinyH3M::TinyH3MBuilder makePlannedArmyEvaluationMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2PlannedArmyEvaluation")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 10}})
		.hero({6, 5, 0}, HeroTypeID(1), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 50}})
		.hero({7, 5, 0}, HeroTypeID(2), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 40}})
		.town({10, 10, 0}, FactionID::CASTLE, PLAYER);

	return builder;
}

using Nullkiller2_Engine_PriorityEvaluatorMap = NullkillerTest;
}

TEST_F(Nullkiller2_Engine_PriorityEvaluatorMap, lossBudgetScalesWithCommittedArmyShare)
{
	startWithMap(makePlannedArmyEvaluationMap());

	const auto gateway = makeGateway(PLAYER);
	const auto * targetHero = findHeroAt({5, 5, 0});
	const auto * plannedArmy = findHeroAt({6, 5, 0});
	ASSERT_NE(targetHero, nullptr);
	ASSERT_NE(plannedArmy, nullptr);
	gateway->nullkiller->armyManager->update();

	NK2AI::AIPath path;
	path.targetHero = targetHero;
	path.heroArmy = plannedArmy;
	path.exchangeCount = 1;
	path.targetObjectDanger = 0;
	// the whole player-wide budget, lost by an army that is half of the matching forces
	path.armyLoss = static_cast<uint64_t>(plannedArmy->estimateCombatValue() * gateway->nullkiller->settings->getMaxArmyLossTarget());
	path.targetObjectArmyLoss = 0;
	path.chainMask = 1;
	auto node = pathNode(*targetHero, {8, 5, 0});
	node.cost = 0.5f;
	path.nodes.push_back(node);

	const auto task = sptr(NK2AI::Goals::Composition()
		.addNext(NK2AI::Goals::ExplorationPoint({8, 5, 0}, 80))
		.addNext(NK2AI::Goals::ExecuteHeroChain(path, nullptr)));
	gateway->nullkiller->dangerHitMap->updateHitMap();
	auto context = gateway->nullkiller->priorityEvaluator->buildEvaluationContext(task);

	EXPECT_NEAR(context.powerRatio, 0.5f, 0.001f);
	EXPECT_GT(gateway->nullkiller->priorityEvaluator->evaluate(
		task,
		NK2AI::PriorityEvaluator::EXPLORE_AND_GATHER,
		context), 0.0f);
}

namespace
{

const int3 HERO_POSITION(5, 5, 0);

TinyH3M::TinyH3MBuilder makeCreatureBankRewardMap()
{
	return TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false)
		.name("NK2CreatureBankReward")
		.playerActive(PLAYER)
		.hero(HERO_POSITION, HeroTypeID(HeroTypeID::decode("isra")), PLAYER)
		.heroGarrison({
			{CreatureID(CreatureID::decode("skeleton")), 1000},
			{CreatureID(CreatureID::decode("walkingDead")), 500},
			{CreatureID(CreatureID::decode("wight")), 300},
			{CreatureID(CreatureID::decode("vampire")), 100},
			{CreatureID(CreatureID::decode("lich")), 50},
			{CreatureID(CreatureID::decode("blackKnight")), 25},
			{CreatureID(CreatureID::decode("boneDragon")), 10}
		});
}

class Nullkiller2_Engine_CreatureBankReward : public TinyMapGameTest
{
protected:
	void SetUp() override
	{
		TinyMapGameTest::SetUp();
		startWithMap(makeCreatureBankRewardMap());
		hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);

		callback = makeCallback(PLAYER);
		nullkiller.cc = callback;
		nullkiller.armyManager = std::make_unique<NK2AI::ArmyManager>(callback.get(), &nullkiller);
		bank = std::make_unique<CRewardableObject>(callback.get());
		bank->ID = Obj::CREATURE_BANK;
		bank->subID = MapObjectSubID(2); // Griffin Conservatory
		bank->id = ObjectInstanceID(42);
		bank->appearance = hero->appearance;
		bank->setAnchorPos(hero->anchorPos());

		auto & visitInfo = bank->configuration.info.emplace_back();
		visitInfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		visitInfo.reward.guards.emplace_back(CreatureID(CreatureID::decode("griffin")), 50);
		visitInfo.reward.creatures.emplace_back(CreatureID(CreatureID::decode("angel")), 1);
	}

	NK2AI::CreatureRewardValueInfo evaluateCreatureReward() const
	{
		NK2AI::RewardEvaluator evaluator(&nullkiller);
		return evaluator.getCreatureReward({bank.get(), hero, hero, NK2AI::HeroRole::SCOUT, false});
	}

	void markBankScouted() const
	{
		auto teamID = gameState()->players.at(PLAYER).team;
		gameState()->teams.at(teamID).scoutedObjects.insert(bank->id);
	}

	CGHeroInstance * hero = nullptr;
	std::shared_ptr<CCallback> callback;
	NK2AI::Nullkiller nullkiller;
	std::unique_ptr<CRewardableObject> bank;
};

}

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestBeatsOrdinaryHeroHunting)
{
	EXPECT_GT(NK2AI::evaluateEnemyTownConquestValue(0.8f, 3), 2.0f * 1.5f)
		<< "visible enemy towns should clearly outrank the capped strategic value of a loose enemy hero";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, lastVisibleEnemyTownGetsEliminationPressure)
{
	EXPECT_GT(
		NK2AI::evaluateEnemyTownConquestValue(1.0f, 1),
		NK2AI::evaluateEnemyTownConquestValue(1.5f, 3))
		<< "taking the last visible enemy town should outrank a normal enemy capital capture";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, nonEnemyTownKeepsBaseConquestValue)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateEnemyTownConquestValue(1.2f, 0),
		1.2f)
		<< "only visible enemy towns should get extra conquest pressure";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestCanRelaxArmyLossLimit)
{
	EXPECT_GT(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, true),
		0.39f)
		<< "enemy town conquest should not be rejected by the ordinary creeping loss cap";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, ordinaryTargetsKeepArmyLossLimit)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, false),
		0.35f)
		<< "the relaxed loss cap is only for enemy-town conquest";
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, fullNecropolisArmyCannotUseAngelReward)
{
	ASSERT_EQ(hero->stacksCount(), GameConstants::ARMY_SIZE);
	markBankScouted();

	const auto reward = evaluateCreatureReward();
	EXPECT_EQ(reward.state, NK2AI::CreatureRewardValueInfo::State::UNUSABLE);
	EXPECT_EQ(reward.minMaxValue.first, 0);
	EXPECT_EQ(reward.minMaxValue.second, 0);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, fullArmyCanUseOnlyLargerCreatureBankRewardTiers)
{
	const CreatureID angel(CreatureID::decode("angel"));
	const CreatureID pikeman(CreatureID::decode("pikeman"));
	const int pikemanCount = angel.toCreature()->getAIValue() / pikeman.toCreature()->getAIValue() + 1;
	const std::array<CreatureID, 7> castleCreatures = {
		pikeman,
		CreatureID(CreatureID::decode("archer")),
		CreatureID(CreatureID::decode("griffin")),
		CreatureID(CreatureID::decode("swordsman")),
		CreatureID(CreatureID::decode("monk")),
		CreatureID(CreatureID::decode("cavalier")),
		CreatureID(CreatureID::decode("archangel"))
	};

	hero->clearSlots();
	for(size_t i = 0; i < castleCreatures.size(); ++i)
		ASSERT_TRUE(hero->setCreature(SlotID(static_cast<int>(i)), castleCreatures[i], i == 0 ? pikemanCount : 1000));
	markBankScouted();

	const auto reward = evaluateCreatureReward();
	EXPECT_EQ(reward.state, NK2AI::CreatureRewardValueInfo::State::DEPENDS_ON_TIER);
	EXPECT_EQ(reward.minMaxValue.first, 0);
	EXPECT_GT(reward.minMaxValue.second, 0);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, armyWithFreeSlotCanUseAngelReward)
{
	hero->eraseStack(SlotID(0));
	markBankScouted();

	const auto reward = evaluateCreatureReward();
	EXPECT_EQ(reward.state, NK2AI::CreatureRewardValueInfo::State::USABLE);
	EXPECT_GT(reward.minMaxValue.first, 0);
	EXPECT_GT(reward.minMaxValue.second, reward.minMaxValue.first);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, moraleImprovementMakesWeakerRawRewardUsable)
{
	const std::array<CreatureID, 6> castleCreatures = {
		CreatureID(CreatureID::decode("pikeman")),
		CreatureID(CreatureID::decode("archer")),
		CreatureID(CreatureID::decode("griffin")),
		CreatureID(CreatureID::decode("swordsman")),
		CreatureID(CreatureID::decode("monk")),
		CreatureID(CreatureID::decode("cavalier"))
	};
	const CreatureID angel(CreatureID::decode("angel"));
	const CreatureID centaur(CreatureID::decode("centaur"));
	const int centaurCount = static_cast<int>(angel.toCreature()->getAIValue() / centaur.toCreature()->getAIValue() + 1);

	hero->clearSlots();
	for(size_t i = 0; i < castleCreatures.size(); ++i)
		ASSERT_TRUE(hero->setCreature(SlotID(static_cast<int>(i)), castleCreatures[i], 100));
	ASSERT_TRUE(hero->setCreature(SlotID(6), centaur, centaurCount));

	CCreatureSet rewardArmy;
	rewardArmy.addToSlot(SlotID(0), angel, 1);
	const auto terrain = callback->getTile(bank->visitablePos())->getTerrainID();
	const auto bestArmyInfo = nullkiller.armyManager->getBestArmyInfo(hero, hero, &rewardArmy, terrain);
	uint64_t selectedRawStrength = 0;
	for(const auto & slot : bestArmyInfo.army)
		selectedRawStrength += slot.creature->getAIValue() * slot.count;

	ASSERT_LT(selectedRawStrength, hero->getArmyStrength());
	ASSERT_GT(bestArmyInfo.sourceStrength, 0);
	ASSERT_GT(bestArmyInfo.strengthGain, 0);

	markBankScouted();
	const auto reward = evaluateCreatureReward();
	EXPECT_EQ(reward.state, NK2AI::CreatureRewardValueInfo::State::USABLE);
	EXPECT_EQ(reward.minMaxValue.first, bestArmyInfo.strengthGain);
	EXPECT_GT(reward.minMaxValue.second, bestArmyInfo.strengthGain);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, unscoutedCreatureRewardUsesPublicBankPossibilities)
{
	const auto reward = evaluateCreatureReward();
	EXPECT_EQ(reward.state, NK2AI::CreatureRewardValueInfo::State::UNUSABLE);
	EXPECT_EQ(reward.minMaxValue.first, 0);
	EXPECT_EQ(reward.minMaxValue.second, 0);
}
