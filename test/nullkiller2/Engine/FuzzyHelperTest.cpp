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
#include "lib/json/JsonRandom.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGPandoraBox.h"
#include "lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "lib/mapping/CMap.h"
#include "lib/modding/LibraryLoadContext.h"
#include "lib/modding/ModScope.h"
#include "lib/rewardable/Info.h"

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
			.hero(OBJECT_POSITION, HeroTypeID(HeroTypeID::decode("pyre")), PLAYER), EMapDifficulty::EXPERT);
		setGuardsVisible(true);

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

	void setGuardsVisible(bool visible)
	{
		JsonNode value;
		value.Bool() = visible;
		map()->overrideGameSetting(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION, value);
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
	const auto bankDanger = gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get());
	ASSERT_GT(bankDanger, 0);
	ASSERT_NE(bankDanger, gateway->nullkiller->dangerEvaluator->evaluateDanger(hero));
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(hero->visitablePos(), hero), bankDanger);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, unscoutedPandoraGuardsPreventUnsafeVisit)
{
	const CreatureID imp(CreatureID::decode("imp"));
	hero->clearSlots();
	ASSERT_TRUE(hero->setCreature(SlotID(0), imp, 1));

	auto pandora = std::make_shared<CGPandoraBox>(callback.get());
	pandora->ID = Obj::PANDORAS_BOX;
	pandora->subID = MapObjectSubID(0);
	pandora->instanceName = "unscoutedPandoraForTest";
	pandora->appearance = hero->appearance;
	pandora->setAnchorPos(OBJECT_POSITION);
	ASSERT_TRUE(pandora->setCreature(SlotID(0), imp, 1000));
	map()->addNewObject(pandora);
	ASSERT_FALSE(pandora->wasScouted(PLAYER));

	const auto safeAttackRatio = gateway->nullkiller->settings->getSafeAttackRatio();
	const auto danger = gateway->nullkiller->dangerEvaluator->evaluateDanger(pandora.get());
	EXPECT_EQ(danger, pandora->estimateCombatValue());
	EXPECT_FALSE(NK2AI::isSafeToVisit(hero, danger, safeAttackRatio));

	pandora->clearSlots();
	const auto unguardedDanger = gateway->nullkiller->dangerEvaluator->evaluateDanger(pandora.get());
	EXPECT_EQ(unguardedDanger, 0);
	EXPECT_TRUE(NK2AI::isSafeToVisit(hero, unguardedDanger, safeAttackRatio));
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, hiddenGuardCompositionUsesMaximumBankDanger)
{
	setGuardsVisible(false);
	const auto rewardableInfo = callback->getRewardableObjectInfo(bank.get());
	ASSERT_TRUE(rewardableInfo);
	EXPECT_TRUE(rewardableInfo->guarded);
	EXPECT_EQ(rewardableInfo->rewardAvailable, std::nullopt);
	EXPECT_EQ(rewardableInfo->guardStrength, 0);
	EXPECT_TRUE(rewardableInfo->rewards.empty());

	auto & team = gameState()->teams.at(gameState()->players.at(PLAYER).team);
	for(const auto type : {Obj::CREATURE_BANK, Obj::CRYPT, Obj::DRAGON_UTOPIA, Obj::SHIPWRECK, Obj::DERELICT_SHIP})
	{
		SCOPED_TRACE(static_cast<int>(type));
		bank->ID = type;
		const auto objectInfo = bank->getObjectHandler()->getObjectInfo();
		const auto * bankInfo = dynamic_cast<const Rewardable::Info *>(objectInfo.get());
		ASSERT_NE(bankInfo, nullptr);
		const auto guardRange = bankInfo->getCreatureBankGuardRange();
		ASSERT_TRUE(guardRange.has_value());
		ASSERT_GT(guardRange->second, 0);
		EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get()), guardRange->second);

		team.scoutedObjects.erase(bank->id);
		EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get()), guardRange->second);
		team.scoutedObjects.insert(bank->id);
	}
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, hiddenGuardsUseConfigurationForCustomObjectType)
{
	class CustomBank : public CRewardableObject
	{
	public:
		using CRewardableObject::CRewardableObject;
		MOCK_METHOD(TObjectTypeHandler, getObjectHandler, (), (const, override));
	};

	setGuardsVisible(false);
	CustomBank customBank(callback.get());
	customBank.ID = MapObjectID(1000);
	const auto handler = bank->getObjectHandler();
	EXPECT_CALL(customBank, getObjectHandler()).WillRepeatedly(::testing::Return(handler));
	const auto objectInfo = handler->getObjectInfo();
	const auto * bankInfo = dynamic_cast<const Rewardable::Info *>(objectInfo.get());
	ASSERT_NE(bankInfo, nullptr);
	const auto guardRange = bankInfo->getCreatureBankGuardRange();
	ASSERT_TRUE(guardRange.has_value());
	ASSERT_GT(guardRange->second, 0);
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(&customBank), guardRange->second);

	customBank.id = bank->id;
	customBank.configuration = bank->configuration;
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(&customBank), guardRange->second);

	customBank.configuration.visitMode = Rewardable::VISIT_ONCE;
	customBank.setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	gameState()->players.at(PLAYER).visitedObjects.insert(customBank.id);
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(&customBank), 0);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, hiddenGuardCompositionDoesNotMakeUnguardedObjectsDangerous)
{
	setGuardsVisible(false);
	bank->ID = Obj::WITCH_HUT;
	bank->clearSlots();
	bank->configuration.info.front().reward.guards.clear();
	gameState()->teams.at(gameState()->players.at(PLAYER).team).scoutedObjects.erase(bank->id);
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get()), 0);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, visibleUnscoutedBankUsesMinimumDangerAtExpertDifficulty)
{
	gameState()->teams.at(gameState()->players.at(PLAYER).team).scoutedObjects.erase(bank->id);

	const auto objectInfo = bank->getObjectHandler()->getObjectInfo();
	const auto * bankInfo = dynamic_cast<const Rewardable::Info *>(objectInfo.get());
	ASSERT_NE(bankInfo, nullptr);
	const auto guardRange = bankInfo->getCreatureBankGuardRange();
	ASSERT_TRUE(guardRange.has_value());
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get()), guardRange->first);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, visibleForceCombatBankUsesMaximumDangerBelowExpertDifficulty)
{
	bank->configuration.forceCombat = true;
	gameState()->getStartInfo()->difficulty = static_cast<ui8>(EMapDifficulty::NORMAL);
	gameState()->teams.at(gameState()->players.at(PLAYER).team).scoutedObjects.erase(bank->id);

	const auto objectInfo = bank->getObjectHandler()->getObjectInfo();
	const auto * bankInfo = dynamic_cast<const Rewardable::Info *>(objectInfo.get());
	ASSERT_NE(bankInfo, nullptr);
	const auto guardRange = bankInfo->getCreatureBankGuardRange();
	ASSERT_TRUE(guardRange.has_value());
	EXPECT_EQ(gateway->nullkiller->dangerEvaluator->evaluateDanger(bank.get()), guardRange->second);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, guaranteedGuardUpgradeExcludesBaseCreature)
{
	JsonNode guards;
	auto & guard = guards.Vector().emplace_back();
	guard["type"].String() = "archer";
	guard["amount"].Integer() = 1;
	guard["upgradeChance"].Integer() = 100;
	guards.setModScope(ModScope::scopeBuiltin());

	const auto evaluatedGuards = JsonRandom::evaluateCreatures(guards);
	ASSERT_TRUE(evaluatedGuards);
	ASSERT_EQ(evaluatedGuards->size(), 1);
	EXPECT_FALSE(vstd::contains(evaluatedGuards->front().allowedCreatures, CreatureID(CreatureID::decode("archer")).toCreature()));
	EXPECT_TRUE(vstd::contains(evaluatedGuards->front().allowedCreatures, CreatureID(CreatureID::decode("marksman")).toCreature()));
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, unsupportedGuardConfigurationRemainsUnknown)
{
	JsonNode supportedConfiguration;
	auto & supportedGuard = supportedConfiguration["rewards"].Vector().emplace_back()["guards"].Vector().emplace_back();
	supportedGuard["type"].String() = "archer";
	supportedGuard["amount"].Integer() = 10;
	supportedConfiguration.setModScope(ModScope::scopeBuiltin());
	Rewardable::Info supportedInfo;
	LibraryLoadContext context(*LIBRARY);
	supportedInfo.init(context, supportedConfiguration, "testCreatureBank");
	const auto guardRange = supportedInfo.getCreatureBankGuardRange();
	ASSERT_TRUE(guardRange);
	EXPECT_GT(guardRange->first, 0);
	EXPECT_EQ(guardRange->first, guardRange->second);

	JsonNode configuration;
	configuration["rewards"].Vector().emplace_back()["guards"]["archer"].Integer() = 10;
	configuration.setModScope(ModScope::scopeBuiltin());
	Rewardable::Info info;
	info.init(context, configuration, "testCreatureBank");

	EXPECT_EQ(info.getCreatureBankGuardRange(), std::nullopt);
}

TEST_F(Nullkiller2_Engine_FuzzyHelper, enemyClearDoesNotChangeRememberedGuardComposition)
{
	bank->configuration.visitMode = Rewardable::VISIT_ONCE;
	const auto rememberedGuards = bank->getPopupComponents(PLAYER);
	ASSERT_FALSE(rememberedGuards.empty());

	bank->clearSlots();
	bank->setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	const auto guardsAfterEnemyClear = bank->getPopupComponents(PLAYER);
	ASSERT_EQ(guardsAfterEnemyClear.size(), rememberedGuards.size());
	EXPECT_EQ(guardsAfterEnemyClear.front().type, rememberedGuards.front().type);
	EXPECT_EQ(guardsAfterEnemyClear.front().subType, rememberedGuards.front().subType);
	EXPECT_EQ(guardsAfterEnemyClear.front().value, rememberedGuards.front().value);
	const auto rewardableInfo = callback->getRewardableObjectInfo(bank.get());
	ASSERT_TRUE(rewardableInfo);
	EXPECT_TRUE(rewardableInfo->guarded);
	EXPECT_GT(rewardableInfo->guardStrength, 0);

	gameState()->players.at(PLAYER).visitedObjects.insert(bank->id);
	EXPECT_TRUE(bank->getPopupComponents(PLAYER).empty());
}
