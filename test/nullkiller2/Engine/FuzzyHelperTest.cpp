/*
 * FuzzyHelperTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/AIMemory.h"
#include "AI/Nullkiller2/Engine/FuzzyHelper.h"

#include "lib/CPlayerState.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CRewardableObject.h"
#include "lib/mapping/CMap.h"

#include "nullkiller2/NullkillerTest.h"

namespace
{

const PlayerColor PLAYER(0);
const int3 OBJECT_POSITION(5, 5, 0);

class Nullkiller2_Engine_FuzzyHelper : public NullkillerTest
{
protected:
	void SetUp() override
	{
		NullkillerTest::SetUp();
		startWithMap(TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
			.size(36, false)
			.name("NK2GuardedRewardable")
			.playerActive(PLAYER)
			.hero(OBJECT_POSITION, HeroTypeID(HeroTypeID::decode("pyre")), PLAYER));

		hero = findHeroAt(OBJECT_POSITION);
		ASSERT_NE(hero, nullptr);

		callback = makeCallback(PLAYER);
		bank = std::make_shared<CRewardableObject>(callback.get());
		bank->ID = Obj::CREATURE_BANK;
		bank->subID = MapObjectSubID(0);
		bank->appearance = hero->appearance;
		bank->instanceName = "guardedRewardableForTest";
		bank->setAnchorPos(OBJECT_POSITION);

		auto & visitInfo = bank->configuration.info.emplace_back();
		visitInfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		visitInfo.reward.guards.emplace_back(CreatureID(CreatureID::decode("imp")), 100);
		bank->initializeGuards();
		map()->addNewObject(bank);

		auto & team = gameState()->teams.at(gameState()->players.at(PLAYER).team);
		team.fogOfWarMap[OBJECT_POSITION] = 1;
		team.scoutedObjects.insert(bank->id);
		gateway = makeGateway(callback);
	}

	CGHeroInstance * hero = nullptr;
	std::shared_ptr<CCallback> callback;
	std::shared_ptr<CRewardableObject> bank;
	std::unique_ptr<NK2AI::AIGateway> gateway;
};

}

TEST_F(Nullkiller2_Engine_FuzzyHelper, guardedRewardableIsNotRememberedAsFinishedAfterScouting)
{
	const auto rewardableInfo = callback->getRewardableObjectInfo(bank.get());
	ASSERT_TRUE(rewardableInfo);
	ASSERT_TRUE(rewardableInfo->scouted);
	ASSERT_TRUE(rewardableInfo->guarded);

	auto & events = static_cast<IGameEventsReceiver &>(*gateway);
	events.heroVisit(hero, bank.get(), true);

	EXPECT_FALSE(gateway->nullkiller->memory->wasVisited(bank.get()));
	events.heroVisit(hero, bank.get(), false);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, unscoutedRewardableIsNotRememberedAsFinished)
{
	bank->clearSlots();
	bank->configuration.info.front().reward.guards.clear();

	const auto rewardableInfo = callback->getRewardableObjectInfo(bank.get());
	ASSERT_TRUE(rewardableInfo);
	ASSERT_TRUE(rewardableInfo->scouted);
	ASSERT_FALSE(rewardableInfo->guarded);
	ASSERT_EQ(rewardableInfo->rewardAvailable, true);

	auto & events = static_cast<IGameEventsReceiver &>(*gateway);
	events.heroVisit(hero, bank.get(), true);
	ASSERT_TRUE(gateway->nullkiller->memory->wasVisited(bank.get()));
	events.heroVisit(hero, bank.get(), false);

	const TeamID team = gameState()->players.at(PLAYER).team;
	gameState()->teams.at(team).scoutedObjects.erase(bank->id);
	gateway->nullkiller->memory->markObjectUnvisited(bank.get());

	events.heroVisit(hero, bank.get(), true);

	EXPECT_FALSE(gateway->nullkiller->memory->wasVisited(bank.get()));
	events.heroVisit(hero, bank.get(), false);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, visitorDoesNotHideScoutedRewardableDanger)
{
	EXPECT_GT(gateway->nullkiller->dangerEvaluator->evaluateDanger(hero->visitablePos(), hero), 0);
}
