/*
 * PriorityEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CRewardableObject.h"

#include "mock/TinyH3MBuilder.h"
#include "mock/TinyMapGameTest.h"

namespace
{

const PlayerColor PLAYER(0);
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
		bank->id = ObjectInstanceID(42);
		bank->appearance = hero->appearance;
		bank->setAnchorPos(hero->anchorPos());

		auto & visitInfo = bank->configuration.info.emplace_back();
		visitInfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		visitInfo.reward.guards.emplace_back(CreatureID(CreatureID::decode("griffin")), 50);
		visitInfo.reward.creatures.emplace_back(CreatureID(CreatureID::decode("angel")), 1);
	}

	std::optional<uint64_t> evaluateCreatureReward() const
	{
		NK2AI::RewardEvaluator evaluator(&nullkiller);
		return evaluator.getCreatureReward(bank.get(), hero, hero);
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
	ASSERT_TRUE(reward.has_value());
	EXPECT_EQ(*reward, 0);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, armyWithFreeSlotCanUseAngelReward)
{
	hero->eraseStack(SlotID(0));
	markBankScouted();

	const auto reward = evaluateCreatureReward();
	ASSERT_TRUE(reward.has_value());
	EXPECT_GT(*reward, 0);
}

TEST_F(Nullkiller2_Engine_CreatureBankReward, unscoutedCreatureRewardRemainsUnknown)
{
	EXPECT_EQ(evaluateCreatureReward(), std::nullopt);
}
