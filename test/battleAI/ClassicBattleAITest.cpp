/*
 * ClassicBattleAITest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"

#include "../../AI/BattleAI/BattleAI.h"
#include "../../AI/BattleAI/Classic/ClassicAttackEvaluator.h"
#include "../../AI/BattleAI/Classic/ClassicBattleController.h"
#include "../../AI/BattleAI/Classic/ClassicBattleDecision.h"
#include "../../AI/BattleAI/Classic/ClassicBattleRng.h"
#include "../../AI/BattleAI/Classic/ClassicBattleStateView.h"
#include "../../AI/BattleAI/Classic/ClassicCombatValue.h"
#include "../../AI/BattleAI/Classic/ClassicDecisionTrace.h"
#include "../../AI/BattleAI/Classic/ClassicRetreatEvaluator.h"
#include "../../AI/BattleAI/Classic/ClassicRulesAdapter.h"
#include "../../AI/BattleAI/Classic/ClassicSpellEvaluator.h"
#include "../../AI/BattleAI/StackWithBonuses.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/callback/IClient.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/ObstacleHandler.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/CPlayerState.h"
#include "../../server/CGameHandler.h"
#include "../../server/battles/BattleProcessor.h"
#include "../mock/ReplayClassicBattleAIRng.h"
#include "../mock/TinyH3MBuilder.h"
#include "../server/battles/BattleTestFixture.h"

namespace
{
class UpperBoundClassicBattleAIRng final : public IClassicBattleAIRng
{
public:
	int32_t nextIntInclusive(int32_t lower, int32_t upper) override
	{
		return std::max(lower, upper);
	}
};

class RecordingBattleAIClient final : public IClient
{
public:
	int requests = 0;

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor player,
		const BattleID & battleID,
		const BattleStateInfoForRetreat & battleState) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override
	{
		return ++requests;
	}
};

class ClassicBattleAIIntegrationTest : public BattleTestFixture
{
protected:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->tacticDistance = 0;
	}

	std::shared_ptr<CBattleInfoCallback> battleCallback()
	{
		return std::shared_ptr<CBattleInfoCallback>(
			battle(),
			[](CBattleInfoCallback *)
			{
			}
		);
	}

	void removeAllStacks()
	{
		BattleUnitsChanged removal;
		removal.battleID = BattleID(0);
		for(const CStack * stack : battle()->battleGetAllStacks(false))
			removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(removal);
	}
};

class ClassicBattleAISiegeIntegrationTest : public TinyMapGameTest
{

protected:
	std::shared_ptr<CGameHandler> gameHandler;
	RecordingGameServer server;
	Services * gameServices() override
	{
		return LIBRARY;
	}

	void configurePlayer(PlayerSettings & settings) const override
	{
		settings.bonus = PlayerStartingBonus::GOLD;
	}

	void SetUp() override
	{
		TinyMapGameTest::SetUp();
		const CreatureID token(0);
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("ClassicBattleAISiege")
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0)).heroGarrison({{token, 1}})
			.hero({12, 12, 0}, HeroTypeID(1), PlayerColor(1)).heroGarrison({{token, 1}})
			.town({10, 10, 0}, FactionID::CASTLE, PlayerColor(1))
			.townFortification(3);
		startWithMap(std::move(builder));

		server.gameState = gameState();
		gameHandler = std::make_shared<CGameHandler>(server, gameState());
		gameHandler->randomizer->setSeed(BattleTestFixture::seed);
		CGHeroInstance * attacker = findHeroByOwner(PlayerColor(0));
		CGHeroInstance * defender = findHeroByOwner(PlayerColor(1));
		CGTownInstance * town = findFirst<CGTownInstance>();
		ASSERT_NE(attacker, nullptr);
		ASSERT_NE(defender, nullptr);
		ASSERT_NE(town, nullptr);

		BattleSideArray<const CGHeroInstance *> heroes = {attacker, defender};
		BattleSideArray<const CArmedInstance *> armies = {attacker, defender};
		const int3 tile(10, 10, 0);
		const BattleLayout layout = BattleLayout::createDefaultLayout(*gameState(), attacker, defender);
		const std::string battlefieldName = "core:sand_shore";
		const BattleField battlefield(
			*LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "battlefield", battlefieldName));
		BattleStart start;
		start.info = BattleInfo::setupBattle(
			gameState().get(),
			tile,
			gameState()->getTile(tile)->getTerrainID(),
			battlefield,
			armies,
			heroes,
			layout,
			nullptr
		);
		start.battleID = BattleID(0);
		gameHandler->sendAndApply(start);
		battle()->townID = town->id;
		battle()->tacticDistance = 0;
		battle()->obstacles.clear();
	}

	void TearDown() override
	{
		gameHandler.reset();
		TinyMapGameTest::TearDown();
	}

	BattleInfo * battle() const
	{
		return gameState()->currentBattles.front().get();
	}

	CStack * addStack(BattleSide side, CreatureID creature, BattleHex position, int32_t count)
	{
		battle::UnitInfo info;
		info.id = battle()->battleNextUnitId();
		info.count = count;
		info.type = creature;
		info.side = side;
		info.position = position;
		BattleUnitsChanged change;
		change.battleID = BattleID(0);
		change.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(change.changedStacks.back().data);
		gameHandler->sendAndApply(change);
		return battle()->getStack(info.id);
	}

	std::shared_ptr<CBattleInfoCallback> battleCallback()
	{
		return std::shared_ptr<CBattleInfoCallback>(battle(), [](CBattleInfoCallback *) {});
	}
};


}

TEST(ClassicBattleAIRngTest, ReplaysInclusiveValuesInOrder)
{
	ReplayClassicBattleAIRng rng({75, 100, 1});
	EXPECT_EQ(rng.nextIntInclusive(75, 100), 75);
	EXPECT_EQ(rng.nextIntInclusive(75, 100), 100);
	EXPECT_EQ(rng.nextIntInclusive(1, 100), 1);
	EXPECT_EQ(rng.consumed(), 3);
	EXPECT_TRUE(rng.exhausted());
}

TEST(ClassicBattleAIRngTest, RejectsExhaustedTape)
{
	ReplayClassicBattleAIRng rng({});
	EXPECT_THROW(rng.nextIntInclusive(1, 100), std::runtime_error);
}

TEST(ClassicBattleAIRngTest, RejectsValueOutsideRequestedRange)
{
	ReplayClassicBattleAIRng rng({74});
	EXPECT_THROW(rng.nextIntInclusive(75, 100), std::runtime_error);
}

TEST(ClassicBattleAIRngTest, ProductionGeneratorHonorsInclusiveBounds)
{
	ClassicBattleAIRng rng;
	for(int iteration = 0; iteration < 32; ++iteration)
	{
		const int32_t value = rng.nextIntInclusive(-3, 7);
		EXPECT_GE(value, -3);
		EXPECT_LE(value, 7);
	}
}

TEST(ClassicBattleAIRngTest, VstdAdapterUsesTheInjectedInclusiveStream)
{
	UpperBoundClassicBattleAIRng source;
	ClassicVstdRngAdapter rng(source);
	EXPECT_EQ(rng.nextInt(-2, 7), 7);
	EXPECT_EQ(rng.nextInt64(-2, 9), 9);
	EXPECT_DOUBLE_EQ(rng.nextDouble(-2.0, 6.0), 6.0);
	EXPECT_EQ(rng.nextInt(11), 11);
	EXPECT_EQ(rng.nextInt64(12), 12);
	EXPECT_DOUBLE_EQ(rng.nextDouble(13.0), 13.0);
	EXPECT_EQ(rng.nextInt(), std::numeric_limits<int32_t>::max());
	EXPECT_EQ(rng.nextBinomialInt(3, 1.0), 3);
	EXPECT_THROW(
		rng.nextInt64(static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1, 0),
		std::out_of_range
	);
}

TEST(ClassicBattleAIRngTest, DestroysGeneratorThroughInterface)
{
	std::unique_ptr<IClassicBattleAIRng> rng =
		std::make_unique<ReplayClassicBattleAIRng>(std::vector<int32_t>{7});
	EXPECT_EQ(rng->nextIntInclusive(0, 7), 7);
	rng.reset();
	EXPECT_EQ(rng, nullptr);
}

TEST(ClassicCombatValueTest, UsesExecutableStyleTruncationTowardZero)
{
	EXPECT_EQ(ClassicCombatValue::truncateTowardZero(2.49), 2);
	EXPECT_EQ(ClassicCombatValue::truncateTowardZero(2.51), 2);
	EXPECT_EQ(ClassicCombatValue::truncateTowardZero(-2.49), -2);
}

TEST(ClassicAttackEvaluatorTest, ReproducesShooterDisabledTargetAndEqualityOrdering)
{
	EXPECT_FALSE(ClassicAttackEvaluator::shouldReplaceShooterTarget(100, true, 1, true));
	EXPECT_TRUE(ClassicAttackEvaluator::shouldReplaceShooterTarget(1, false, 100, true));
	EXPECT_TRUE(ClassicAttackEvaluator::shouldReplaceShooterTarget(100, true, 100, false));
	EXPECT_FALSE(ClassicAttackEvaluator::shouldReplaceShooterTarget(99, true, 100, false));
	EXPECT_TRUE(ClassicAttackEvaluator::shouldReplaceShooterTarget(100, false, 100, false));
}

TEST(ClassicSpellEvaluatorTest, AppliesManaConservationBoundaries)
{
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 0), 0);
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 1), 100);
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 4), 200);
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 6), 245);
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 7), 250);
	EXPECT_EQ(ClassicSpellEvaluator::applyManaConservation(100, 20), 250);
}

TEST(ClassicSpellEvaluatorTest, AppliesSlowToCurrentEffectiveSpeedWithIntegerTruncation)
{
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(5, 0), 3);
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(5, 1), 3);
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(5, 2), 2);
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(11, 3), 5);
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(1, 3), 1);
	// Haste is deliberately already present in the input speed. The SoD AI
	// scores expert Slow as 50% of ten, not 50% of the un-Hasted base five.
	EXPECT_EQ(ClassicSpellEvaluator::classicSlowSpeed(10, 3), 5);
}

TEST(ClassicRetreatEvaluatorTest, AppliesBiasWithConversionAfterEachStep)
{
	EXPECT_EQ(ClassicRetreatEvaluator::applyTenPercentBias(10), 11);
	EXPECT_EQ(ClassicRetreatEvaluator::applyTenPercentBias(15), 17);
	EXPECT_EQ(ClassicRetreatEvaluator::applyTenPercentBias(ClassicRetreatEvaluator::applyTenPercentBias(15)), 19);
}

TEST(ClassicDecisionTraceTest, PreservesInsertionOrder)
{
	ClassicDecisionTrace trace;
	trace.record("one", "a", 1);
	trace.record("two", "b", 2);
	ASSERT_EQ(trace.getEntries().size(), 2);
	EXPECT_EQ(trace.getEntries()[0].stage, "one");
	EXPECT_EQ(trace.getEntries()[1].stage, "two");
	trace.clear();
	EXPECT_TRUE(trace.getEntries().empty());
}

TEST(ClassicBattleAIModeTest, DefaultsToModernAndCanBeEnabledExplicitly)
{
	CBattleAI modern;
	EXPECT_FALSE(modern.isClassicMode());

	BattleAISettings settings;
	settings.mode = BattleAIMode::CLASSIC;
	CBattleAI classic(settings);
	EXPECT_TRUE(classic.isClassicMode());
}

TEST_F(ClassicBattleAIIntegrationTest, BuildsStableSidesAndCombatParameters)
{
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const auto stacks = view.orderedStacks();
	ASSERT_GE(stacks.size(), 2);
	EXPECT_EQ(stacks.front()->unitSide(), BattleSide::ATTACKER);
	EXPECT_EQ(stacks.back()->unitSide(), BattleSide::DEFENDER);
	EXPECT_TRUE(view.isEnemy(stacks.front(), stacks.back()));
	EXPECT_FALSE(view.isEnemy(stacks.front(), stacks.front()));

	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	int32_t expectedLowestAttack = std::numeric_limits<int32_t>::max();
	int32_t expectedLowestDefense = std::numeric_limits<int32_t>::max();
	for(const CStack * stack : callback->battleGetAllStacks(false))
	{
		if(!stack->alive() || stack->isTurret())
			continue;
		expectedLowestAttack = std::min(
			expectedLowestAttack,
			stack->getAttack(stack->isShooter()) - stack->unitType()->getBaseAttack());
		expectedLowestDefense = std::min(
			expectedLowestDefense,
			stack->getDefense(false) - stack->unitType()->getBaseDefense());
	}
	EXPECT_EQ(parameters.lowestAttack, expectedLowestAttack);
	EXPECT_EQ(parameters.lowestDefense, expectedLowestDefense);
	EXPECT_GT(parameters.friendlyCombatValue, 0);
	EXPECT_GT(parameters.enemyCombatValue, 0);
	EXPECT_GE(parameters.roundsLeft, 1);
	EXPECT_LE(parameters.roundsLeft, 7);
}

TEST_F(ClassicBattleAIIntegrationTest, ReadsEnemyHeroBonusesThroughPlayerCallback)
{
	removeAllStacks();
	CStack * defender = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	CStack * ballista = addStack(BattleSide::DEFENDER, CreatureID(146), BattleHex(149), 1);
	ClassicCombatParameters parameters;
	parameters.lowestAttack = defender->getAttack(false) - defender->unitType()->getBaseAttack();
	parameters.lowestDefense = defender->getDefense(false) - defender->unitType()->getBaseDefense();
	const auto unrestricted = battleCallback();
	ClassicCombatValue unrestrictedValues(unrestricted);
	const int64_t withoutArmorer = unrestrictedValues.unitValue(defender, parameters);

	defenderSideHero->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::GENERAL_DAMAGE_REDUCTION,
		BonusSource::SECONDARY_SKILL,
		50,
		BonusSourceID(SecondarySkill(SecondarySkill::ARMORER)),
		BonusSubtypeID(BonusCustomSubtype::damageTypeAll)));
	const int64_t unrestrictedValue = unrestrictedValues.unitValue(defender, parameters);
	ASSERT_LT(unrestrictedValue, withoutArmorer);
	const int64_t withoutArtillery = unrestrictedValues.unitValue(ballista, parameters);
	defenderSideHero->setSecSkillLevel(
		SecondarySkill::ARTILLERY, 3, ChangeValueMode::ABSOLUTE);
	const int64_t withArtillery = unrestrictedValues.unitValue(ballista, parameters);
	ASSERT_GT(withArtillery, withoutArtillery);

	auto restricted = std::make_shared<CPlayerBattleCallback>(
		battle(), attackerSideHero->getOwner());
	ClassicCombatValue restrictedValues(restricted);
	EXPECT_EQ(restrictedValues.unitValue(defender, parameters), unrestrictedValue);
	EXPECT_EQ(restrictedValues.unitValue(ballista, parameters), withArtillery);
}

TEST_F(ClassicBattleAIIntegrationTest, ReproducesExecutableMoveOrderBandsAndSideAlternation)
{
	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(74), 1);
	CStack * defender = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(112), 1);
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);

	auto entryFor = [](const std::vector<ClassicMoveOrderEntry> & order, const CStack * stack)
		-> const ClassicMoveOrderEntry &
	{
		const auto found = std::ranges::find_if(order, [stack](const auto & entry)
		{
			return entry.stack == stack;
		});
		if(found == order.end())
			throw std::runtime_error("test stack missing from classic move order");
		return *found;
	};
	auto firstSideForKey = [](const std::vector<ClassicMoveOrderEntry> & order, int32_t key)
	{
		const auto found = std::ranges::find_if(order, [key](const auto & entry)
		{
			return entry.key == key;
		});
		return found == order.end() ? BattleSide::NONE : found->stack->unitSide();
	};

	const int32_t speed = attacker->getInitiative(0);
	ASSERT_EQ(speed, defender->getInitiative(0));
	auto order = view.moveOrder(BattleSide::ATTACKER, false);
	EXPECT_EQ(entryFor(order, attacker).key, speed);
	EXPECT_EQ(firstSideForKey(order, speed), BattleSide::ATTACKER);
	order = view.moveOrder(BattleSide::DEFENDER, false);
	EXPECT_EQ(firstSideForKey(order, speed), BattleSide::DEFENDER);

	order = view.moveOrder(BattleSide::ATTACKER, true);
	EXPECT_EQ(entryFor(order, attacker).key, -speed);

	JsonNode state = attacker->save();
	state["state"]["waiting"].Bool() = false;
	state["state"]["waitedThisTurn"].Bool() = false;
	state["state"]["moved"].Bool() = true;
	attacker->load(state);
	order = view.moveOrder(BattleSide::ATTACKER, false);
	EXPECT_EQ(entryFor(order, attacker).key, speed - 1000);

	auto blind = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::NOT_ACTIVE,
		BonusSource::SPELL_EFFECT,
		0,
		BonusSourceID(SpellID(SpellID::BLIND)));
	blind->turnsRemain = 2;
	attacker->addNewBonus(blind);
	order = view.moveOrder(BattleSide::ATTACKER, false);
	EXPECT_EQ(entryFor(order, attacker).key, -10000);

	CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(72), 1);
	order = view.moveOrder(BattleSide::ATTACKER, false);
	EXPECT_EQ(entryFor(order, tent).key, -100000);
	EXPECT_GT(order.front().order, order.back().order);
}

TEST_F(ClassicBattleAIIntegrationTest, LossValueConvertsTheDirectHitPointProductOnce)
{
	auto callback = battleCallback();
	const CStack * candidate = nullptr;
	for(const CStack * stack : callback->battleGetAllStacks(false))
	{
		if(stack->alive() && !stack->isTurret() && !stack->isShooter()
		   && stack->canMove() && stack->getTotalAttacks(false) == 1)
		{
			candidate = stack;
			break;
		}
	}
	ASSERT_NE(candidate, nullptr);

	ClassicCombatParameters parameters;
	parameters.lowestAttack = candidate->getAttack(false) - candidate->unitType()->getBaseAttack();
	parameters.lowestDefense = candidate->getDefense(false) - candidate->unitType()->getBaseDefense();
	const int64_t hitPoints = candidate->getMaxHealth();
	const int64_t lostHealth = std::max<int64_t>(1, hitPoints / 3);
	const int64_t before = candidate->getAvailableHealth();
	const int64_t expected = ClassicCombatValue::truncateTowardZero(
		static_cast<double>(candidate->unitType()->getFightValue()) * lostHealth / hitPoints);
	ClassicCombatValue values(callback);
	EXPECT_EQ(values.lossValue(candidate, before, before - lostHealth, parameters), expected);
}

TEST_F(ClassicBattleAIIntegrationTest, BlockedShooterPenaltyAndRangedExtraStrikeStayInsideSquareRoot)
{
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(3), BattleHex(77), 1);
	auto callback = battleCallback();
	ClassicCombatParameters parameters;
	parameters.lowestAttack = shooter->getAttack(true) - shooter->unitType()->getBaseAttack();
	parameters.lowestDefense = shooter->getDefense(false) - shooter->unitType()->getBaseDefense();
	ClassicCombatValue values(callback);
	const int64_t fightValue = shooter->unitType()->getFightValue();
	ASSERT_TRUE(callback->battleCanShoot(shooter));
	EXPECT_EQ(values.unitValue(shooter, parameters), ClassicCombatValue::truncateTowardZero(fightValue * std::sqrt(2.0)));

	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(78), 1);
	ASSERT_FALSE(callback->battleCanShoot(shooter));
	parameters.lowestAttack = shooter->getAttack(false) - shooter->unitType()->getBaseAttack();
	EXPECT_EQ(values.unitValue(shooter, parameters), ClassicCombatValue::truncateTowardZero(fightValue * std::sqrt(0.5)));
}

TEST_F(ClassicBattleAIIntegrationTest, TacticsShooterPlacementAvoidsOnlyEnemyShootersAndKeepsCurrentTies)
{
	removeAllStacks();
	battle()->obstacles.clear();
	battle()->tacticDistance = 7;
	battle()->tacticsSide = BattleSide::ATTACKER;
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(86), 20);
	addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(87), 20);
	ClassicAttackEvaluator evaluator(
		battleCallback(), std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr);
	EXPECT_EQ(evaluator.chooseTacticsShooterPlacement(shooter, true).actionType, EActionType::WAIT);

	removeAllStacks();
	shooter = addStack(BattleSide::ATTACKER, CreatureID::ARCHER, BattleHex(86), 20);
	addStack(BattleSide::DEFENDER, CreatureID::ARCHER, BattleHex(87), 20);
	ClassicAttackEvaluator enemyEvaluator(
		battleCallback(), std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr);
	const BattleAction action = enemyEvaluator.chooseTacticsShooterPlacement(shooter, true);
	ASSERT_EQ(action.actionType, EActionType::WALK);
	ASSERT_FALSE(action.target.empty());
	EXPECT_NE(action.target.front().hexValue, shooter->getPosition());
}

TEST_F(ClassicBattleAIIntegrationTest, BlessAndCurseScaleCombatValueUsingEffectiveDamage)
{
	struct SpellCase
	{
		SpellID spell;
		BonusType bonus;
		int32_t shift;
		double effectiveDamage;
	};
	const std::array<SpellCase, 4> cases = {{
		{SpellID(SpellID::BLESS), BonusType::ALWAYS_MAXIMUM_DAMAGE, 0, 3.0},
		{SpellID(SpellID::BLESS), BonusType::ALWAYS_MAXIMUM_DAMAGE, 1, 4.0},
		{SpellID(SpellID::CURSE), BonusType::ALWAYS_MINIMUM_DAMAGE, 0, 2.0},
		{SpellID(SpellID::CURSE), BonusType::ALWAYS_MINIMUM_DAMAGE, 1, 1.0}
	}};
	for(const SpellCase & test : cases)
	{
		SCOPED_TRACE(test.spell.getNum());
		SCOPED_TRACE(test.shift);
		removeAllStacks();
		CStack * archer = addStack(BattleSide::ATTACKER, CreatureID(2), BattleHex(35), 10);
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 10);
		ClassicCombatValue values(battleCallback());
		const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
		const double baseValue = values.unitValueExact(archer, parameters, true);
		ASSERT_EQ(archer->getMinDamage(true), 2);
		ASSERT_EQ(archer->getMaxDamage(true), 3);
		auto bonus = std::make_shared<Bonus>(
			BonusDuration::N_TURNS, test.bonus, BonusSource::SPELL_EFFECT,
			test.shift, BonusSourceID(test.spell));
		bonus->turnsRemain = 2;
		archer->addNewBonus(bonus);

		EXPECT_NEAR(values.unitValueExact(archer, parameters, true),
			baseValue * std::sqrt(test.effectiveDamage / 2.5), 1e-9);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, CloneAndSummonUseOriginalSpecialStackScaling)
{
	CStack * clone = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(74), 10);
	JsonNode cloneState = clone->save();
	cloneState["state"]["cloned"].Bool() = true;
	clone->load(cloneState);

	ClassicCombatParameters cloneParameters;
	cloneParameters.lowestAttack = clone->getAttack(false) - clone->unitType()->getBaseAttack();
	cloneParameters.lowestDefense = clone->getDefense(false) - clone->unitType()->getBaseDefense();
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	EXPECT_EQ(
		values.stackValue(clone, cloneParameters),
		ClassicCombatValue::truncateTowardZero(clone->unitType()->getFightValue() * clone->getCount() / 5.0));

	CStack * summoned = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(75), 1);
	JsonNode summonState = summoned->save();
	summonState["state"]["summoned"].Bool() = true;
	summoned->load(summonState);
	ClassicCombatParameters summonParameters;
	summonParameters.lowestAttack = summoned->getAttack(false) - summoned->unitType()->getBaseAttack();
	summonParameters.lowestDefense = summoned->getDefense(false) - summoned->unitType()->getBaseDefense();
	int64_t ordinaryHealth = 0;
	for(const CStack * stack : callback->battleGetAllStacks(false))
	{
		if(stack->unitSide() == summoned->unitSide()
		   && !stack->hasBonusOfType(BonusType::SIEGE_WEAPON)
		   && !stack->summoned
		   && !stack->isClone())
			ordinaryHealth += stack->getAvailableHealth();
	}
	const double expected = summoned->unitType()->getFightValue()
		* static_cast<double>(ordinaryHealth) / (summoned->getAvailableHealth() + ordinaryHealth);
	EXPECT_EQ(values.stackValue(summoned, summonParameters), ClassicCombatValue::truncateTowardZero(expected));
}

TEST_F(ClassicBattleAIIntegrationTest, ProducesDeterministicLegalStackAction)
{
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const CStack * attacker = nullptr;
	for(const CStack * stack : view.orderedStacks())
	{
		if(stack->unitSide() == BattleSide::ATTACKER)
		{
			attacker = stack;
			break;
		}
	}
	ASSERT_NE(attacker, nullptr);

	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>(32, 100));
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	const ClassicScoredAction result = evaluator.chooseAction(attacker, parameters);

	EXPECT_TRUE(result.valid);
	EXPECT_EQ(result.action.stackNumber, attacker->unitId());
	EXPECT_NE(result.action.actionType, EActionType::NO_ACTION);
	EXPECT_FALSE(trace->getEntries().empty());
}

TEST_F(ClassicBattleAIIntegrationTest, MultiTurnMeleeTargetAdvancesBeforeConsideringWait)
{
	removeAllStacks();
	battle()->obstacles.clear();
	battle()->terrainType = TerrainId(2); // Grass: native Castle units gain one speed.
	battle()->nodeHasChanged();
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(86), 95);
	pikemen->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::STACKS_SPEED,
		BonusSource::TERRAIN_NATIVE,
		1,
		BonusSourceID()));
	addStack(BattleSide::DEFENDER, CreatureID(41), BattleHex(100), 1);
	auto callback = battleCallback();
	ASSERT_EQ(pikemen->getMovementRange(), 5);
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{92});
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);

	const ClassicScoredAction result = evaluator.chooseAction(pikemen, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().hexValue, BattleHex(91));
	EXPECT_TRUE(rng->exhausted());
}

TEST_F(ClassicBattleAIIntegrationTest, DangerProjectionMarksVisitedCellsThroughSpeedPlusOne)
{
	removeAllStacks();
	battle()->obstacles.clear();
	battle()->terrainType = TerrainId(2);

	auto addStaticObstacle = [&](int32_t obstacleID, int32_t hex)
	{
		const ObstacleInfo * info = LIBRARY->obstacleHandler->getByName(std::to_string(obstacleID));
		ASSERT_NE(info, nullptr);
		auto obstacle = std::make_shared<CObstacleInstance>();
		obstacle->ID = info->obstacle.getNum();
		obstacle->pos = BattleHex(hex);
		obstacle->obstacleType = CObstacleInstance::USUAL;
		obstacle->uniqueID = static_cast<int32_t>(battle()->obstacles.size());
		battle()->obstacles.push_back(std::move(obstacle));
	};
	addStaticObstacle(23, 41);
	addStaticObstacle(21, 131);
	addStaticObstacle(19, 26);
	addStaticObstacle(20, 62);

	CStack * masterGenie = addStack(BattleSide::ATTACKER, CreatureID(37), BattleHex(35), 20);
	CStack * archMage = addStack(BattleSide::ATTACKER, CreatureID(35), BattleHex(137), 20);
	const std::array<CStack *, 6> enemies = {
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(10), 167),
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(44), 167),
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(113), 167),
		addStack(BattleSide::DEFENDER, CreatureID(1), BattleHex(129), 167),
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(146), 166),
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(180), 154),
	};
	auto markDone = [](CStack * stack)
	{
		JsonNode state = stack->save();
		state["state"]["moved"].Bool() = true;
		stack->load(state);
	};
	markDone(archMage);
	for(CStack * enemy : enemies)
		markDone(enemy);
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{
		82, 95, 92, 98, 89, 83, 97, 89, 75, 79, 91, 77});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace, true, true);

	const ClassicScoredAction result = evaluator.chooseAction(masterGenie, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK_AND_ATTACK);
	auto firstDanger = [&](int32_t hex) -> std::optional<int64_t>
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[hex](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == "attack_hex.danger"
					&& entry.key == std::to_string(hex);
			});
		return found == trace->getEntries().end()
			? std::nullopt
			: std::optional<int64_t>(found->value);
	};
	const auto danger9 = firstDanger(9);
	const auto danger11 = firstDanger(11);
	const auto danger28 = firstDanger(28);
	const auto danger45 = firstDanger(45);
	ASSERT_TRUE(danger9.has_value());
	ASSERT_TRUE(danger11.has_value());
	ASSERT_TRUE(danger28.has_value());
	ASSERT_TRUE(danger45.has_value());
	EXPECT_LT(*danger11, *danger9);
	EXPECT_EQ(*danger28, *danger11);
	EXPECT_LT(*danger45, *danger11);
}

TEST_F(ClassicBattleAIIntegrationTest, DangerProjectionFiltersObstaclesAndValuesHostileFireWall)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * actor = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);

	auto addSpellObstacle = [&](SpellID trigger, BattleSide caster, int32_t damage, BattleHex hex)
	{
		auto obstacle = std::make_shared<SpellCreatedObstacle>();
		obstacle->ID = trigger.getNum();
		obstacle->pos = hex;
		obstacle->uniqueID = static_cast<int32_t>(battle()->obstacles.size());
		obstacle->turnsRemaining = 2;
		obstacle->casterSide = caster;
		obstacle->minimalDamage = damage;
		obstacle->passable = true;
		obstacle->trigger = trigger;
		obstacle->customSize.insert(hex);
		battle()->obstacles.push_back(std::move(obstacle));
	};
	// Exercise every original Fire Wall filter before the one contributing
	// hostile, positive damage record.
	const ObstacleInfo * ordinaryInfo = LIBRARY->obstacleHandler->getByName("23");
	ASSERT_NE(ordinaryInfo, nullptr);
	auto ordinary = std::make_shared<CObstacleInstance>();
	ordinary->ID = ordinaryInfo->obstacle.getNum();
	ordinary->obstacleType = CObstacleInstance::USUAL;
	ordinary->pos = BattleHex(54);
	ordinary->uniqueID = static_cast<int32_t>(battle()->obstacles.size());
	battle()->obstacles.push_back(std::move(ordinary));
	addSpellObstacle(SpellID(SpellID::LAND_MINE), BattleSide::DEFENDER, 100, BattleHex(55));
	addSpellObstacle(SpellID(SpellID::FIRE_WALL), BattleSide::ATTACKER, 100, BattleHex(56));
	addSpellObstacle(SpellID(SpellID::FIRE_WALL), BattleSide::DEFENDER, 0, BattleHex(57));
	addSpellObstacle(SpellID(SpellID::FIRE_WALL), BattleSide::DEFENDER, 1000, BattleHex(150));
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	const ClassicScoredAction result = evaluator.chooseAction(actor, parameters);

	ASSERT_TRUE(result.valid);
	const auto danger = std::ranges::find_if(
		trace->getEntries(),
		[](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "attack_hex.danger" && entry.key == "150";
		});
	ASSERT_NE(danger, trace->getEntries().end());
	EXPECT_LT(danger->value, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, RunSelectorEscapesNegativeDangerWithOriginalTieOrder)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * actor = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(56), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);

	auto fireWall = std::make_shared<SpellCreatedObstacle>();
	fireWall->ID = SpellID(SpellID::FIRE_WALL).getNum();
	fireWall->pos = actor->getPosition();
	fireWall->uniqueID = 0;
	fireWall->turnsRemaining = 2;
	fireWall->casterSide = BattleSide::DEFENDER;
	fireWall->minimalDamage = 1000;
	fireWall->passable = true;
	fireWall->trigger = SpellID(SpellID::FIRE_WALL);
	fireWall->customSize.insert(actor->getPosition());
	battle()->obstacles.push_back(std::move(fireWall));
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace, true);
	const ClassicScoredAction result = evaluator.chooseRunAction(actor, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK);
	ASSERT_FALSE(result.action.target.empty());
	// All six adjacent safe cells have danger zero and distance one. The
	// executable's ascending scan replaces on a complete tie, retaining 73.
	EXPECT_EQ(result.action.target.front().hexValue, BattleHex(73));
	EXPECT_EQ(result.score, 0);
	EXPECT_EQ(result.attackTime, 1);
	const auto current = std::ranges::find_if(
		trace->getEntries(),
		[](const auto & entry)
		{
			return entry.stage == "run.current";
		}
	);
	ASSERT_NE(current, trace->getEntries().end());
	EXPECT_LT(current->value, 0);
	const auto selected = std::ranges::find_if(
		trace->getEntries(),
		[](const auto & entry)
		{
			return entry.stage == "run.final_hex";
		}
	);
	ASSERT_NE(selected, trace->getEntries().end());
	EXPECT_EQ(selected->value, 73);
}

TEST_F(ClassicBattleAIIntegrationTest, RunSelectorRequiresClassicHardGateAndNegativeCurrentDanger)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * actor = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(56), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);

	ClassicAttackEvaluator hardEvaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr, true);
	EXPECT_FALSE(hardEvaluator.chooseRunAction(actor, parameters).valid);

	auto fireWall = std::make_shared<SpellCreatedObstacle>();
	fireWall->ID = SpellID(SpellID::FIRE_WALL).getNum();
	fireWall->pos = actor->getPosition();
	fireWall->uniqueID = 0;
	fireWall->turnsRemaining = 2;
	fireWall->casterSide = BattleSide::DEFENDER;
	fireWall->minimalDamage = 1000;
	fireWall->passable = true;
	fireWall->trigger = SpellID(SpellID::FIRE_WALL);
	fireWall->customSize.insert(actor->getPosition());
	battle()->obstacles.push_back(std::move(fireWall));
	battle()->nodeHasChanged();

	ClassicAttackEvaluator normalEvaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr, false);
	EXPECT_FALSE(normalEvaluator.chooseRunAction(actor, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, RunSelectorRejectsEveryOriginalIncapacitatingEffect)
{
	const std::array<SpellID, 3> disablingSpells = {
		SpellID(SpellID::BLIND),
		SpellID(SpellID::STONE_GAZE),
		SpellID(SpellID::PARALYZE),
	};
	for(const SpellID spell : disablingSpells)
	{
		SCOPED_TRACE(spell.getNum());
		removeAllStacks();
		battle()->obstacles.clear();
		CStack * actor = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(56), 100);
		addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
		auto fireWall = std::make_shared<SpellCreatedObstacle>();
		fireWall->ID = SpellID(SpellID::FIRE_WALL).getNum();
		fireWall->pos = actor->getPosition();
		fireWall->uniqueID = 0;
		fireWall->turnsRemaining = 2;
		fireWall->casterSide = BattleSide::DEFENDER;
		fireWall->minimalDamage = 1000;
		fireWall->passable = true;
		fireWall->trigger = SpellID(SpellID::FIRE_WALL);
		fireWall->customSize.insert(actor->getPosition());
		battle()->obstacles.push_back(std::move(fireWall));
		auto disabled = std::make_shared<Bonus>(BonusDuration::N_TURNS, BonusType::NOT_ACTIVE, BonusSource::SPELL_EFFECT, 0, BonusSourceID(spell));
		disabled->turnsRemain = 1;
		actor->addNewBonus(disabled);
		battle()->nodeHasChanged();

		auto callback = battleCallback();
		ClassicCombatValue values(callback);
		const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
		ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr, true);
		EXPECT_FALSE(evaluator.chooseRunAction(actor, parameters).valid);
	}

	removeAllStacks();
	battle()->obstacles.clear();
	CStack * bound = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(56), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	auto fireWall = std::make_shared<SpellCreatedObstacle>();
	fireWall->ID = SpellID(SpellID::FIRE_WALL).getNum();
	fireWall->pos = bound->getPosition();
	fireWall->uniqueID = 0;
	fireWall->turnsRemaining = 2;
	fireWall->casterSide = BattleSide::DEFENDER;
	fireWall->minimalDamage = 1000;
	fireWall->passable = true;
	fireWall->trigger = SpellID(SpellID::FIRE_WALL);
	fireWall->customSize.insert(bound->getPosition());
	battle()->obstacles.push_back(std::move(fireWall));
	bound->addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::BIND_EFFECT, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));
	battle()->nodeHasChanged();
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 2);
	ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr, true);
	EXPECT_FALSE(evaluator.chooseRunAction(bound, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, NormalDifficultyMoveStillUsesDeduplicatedDangerProjection)
{
	removeAllStacks();
	battle()->obstacles.clear();
	battle()->terrainType = TerrainId(2);

	auto addStaticObstacle = [&](int32_t obstacleID, int32_t hex)
	{
		const ObstacleInfo * info = LIBRARY->obstacleHandler->getByName(std::to_string(obstacleID));
		ASSERT_NE(info, nullptr);
		auto obstacle = std::make_shared<CObstacleInstance>();
		obstacle->ID = info->obstacle.getNum();
		obstacle->pos = BattleHex(hex);
		obstacle->obstacleType = CObstacleInstance::USUAL;
		obstacle->uniqueID = static_cast<int32_t>(battle()->obstacles.size());
		battle()->obstacles.push_back(std::move(obstacle));
	};
	addStaticObstacle(23, 41);
	addStaticObstacle(21, 131);
	addStaticObstacle(19, 26);
	addStaticObstacle(20, 62);

	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(1), 10);
	addStack(BattleSide::ATTACKER, CreatureID(6), BattleHex(35), 10);
	addStack(BattleSide::ATTACKER, CreatureID(24), BattleHex(70), 10);
	addStack(BattleSide::ATTACKER, CreatureID(39), BattleHex(104), 10);
	addStack(BattleSide::ATTACKER, CreatureID(90), BattleHex(137), 10);
	addStack(BattleSide::ATTACKER, CreatureID(96), BattleHex(172), 10);
	addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(14), 167);
	addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(48), 167);
	addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(82), 167);
	CStack * actor = addStack(BattleSide::DEFENDER, CreatureID(97), BattleHex(116), 167);
	addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(150), 166);
	addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(184), 166);
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::DEFENDER, 1);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(
		std::vector<int32_t>{76, 78, 88, 90, 92, 86});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	// false models the Normal-difficulty computer-side long-wait policy. It
	// must not disable danger-map construction.
	ClassicAttackEvaluator evaluator(callback, rng, trace, false, false);

	const ClassicScoredAction result = evaluator.chooseAction(actor, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().hexValue, BattleHex(163));
	EXPECT_TRUE(rng->exhausted());

	auto traceValue = [&](std::string_view stage, int32_t hex) -> std::optional<int64_t>
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[&](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == stage && entry.key == std::to_string(hex);
			});
		return found == trace->getEntries().end()
			? std::nullopt
			: std::optional<int64_t>(found->value);
	};
	const auto danger162 = traceValue("move_toward.candidate_head", 162);
	const auto danger161 = traceValue("move_toward.candidate_head", 161);
	const auto danger160 = traceValue("move_toward.candidate_head", 160);
	ASSERT_TRUE(danger162.has_value());
	ASSERT_TRUE(danger161.has_value());
	ASSERT_TRUE(danger160.has_value());
	EXPECT_LT(*danger162, 0);
	EXPECT_EQ(*danger161, *danger162);
	EXPECT_EQ(*danger160, *danger162);
	EXPECT_EQ(traceValue("move_toward.result", 2), 163);
}

TEST_F(ClassicBattleAIIntegrationTest, MeleeTargetSelectionPrefersEnabledStackOverHigherScoringBlindStack)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * swordsmen = addStack(BattleSide::ATTACKER, CreatureID(6), BattleHex(86), 1000);
	CStack * enabled = addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(87), 167);
	CStack * blinded = addStack(BattleSide::DEFENDER, CreatureID(96), BattleHex(70), 165);
	auto blind = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::NOT_ACTIVE,
		BonusSource::SPELL_EFFECT,
		0,
		BonusSourceID(SpellID(SpellID::BLIND)));
	blind->turnsRemain = 3;
	blinded->addNewBonus(blind);
	auto blindRetaliation = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::GENERAL_ATTACK_REDUCTION,
		BonusSource::SPELL_EFFECT,
		50,
		BonusSourceID(SpellID(SpellID::BLIND)));
	blindRetaliation->turnsRemain = 3;
	blinded->addNewBonus(blindRetaliation);
	JsonNode blindedState = blinded->save();
	blindedState["counterAttacks"]["used"].Integer() = blinded->counterAttacks.total();
	blinded->load(blindedState);
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 1);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{100, 100});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);

	const ClassicScoredAction result = evaluator.chooseAction(swordsmen, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_FALSE(result.action.target.empty());
	ASSERT_GT(result.action.target.size(), 1);
	EXPECT_EQ(result.action.target[1].unitValue, enabled->unitId());
	EXPECT_TRUE(rng->exhausted());

	auto randomizedScore = [&](const CStack * target) -> std::optional<int64_t>
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[target](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == "melee.randomized"
					&& entry.key == std::to_string(target->getPosition().toInt());
			});
		return found == trace->getEntries().end()
			? std::nullopt
			: std::optional<int64_t>(found->value);
	};
	const auto enabledScore = randomizedScore(enabled);
	const auto blindedScore = randomizedScore(blinded);
	ASSERT_TRUE(enabledScore.has_value());
	ASSERT_TRUE(blindedScore.has_value());
	EXPECT_GT(*blindedScore, *enabledScore);
	for(const CStack * target : {enabled, blinded})
	{
		const auto time = std::ranges::find_if(
			trace->getEntries(),
			[target](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == "melee.time"
					&& entry.key == std::to_string(target->unitId());
			});
		ASSERT_NE(time, trace->getEntries().end());
		EXPECT_EQ(time->value, 1);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, StackEntryPointConsumesOnlyDoCompAIRandomness)
{
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const CStack * attacker = view.orderedStacks().front();
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{75});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	AutocombatPreferences preferences;
	preferences.enableSpellsUsage = true;
	const BattleAction action = ClassicBattleDecision::decide(
		gameHandler.get(), callback, attacker->unitSide(), attacker, 2, preferences, rng, trace,
		ClassicDecisionEntryPoint::DO_COMP_AI);
	EXPECT_NE(action.actionType, EActionType::NO_ACTION);
	EXPECT_EQ(rng->consumed(), 1);
	EXPECT_TRUE(rng->exhausted());
}

TEST_F(ClassicBattleAIIntegrationTest, RejectsModdedCreaturesOutsideConformanceDomain)
{
	CStack * modded = addStack(
		BattleSide::ATTACKER,
		creatureByName("vcmi-test:testSoulStealer"),
		BattleHex(leftHex),
		1
	);
	AutocombatPreferences preferences;
	preferences.enableSpellsUsage = false;
	preferences.enableTacticsUsage = false;
	EXPECT_THROW(
		ClassicBattleDecision::decide(
			gameHandler.get(),
			battleCallback(),
			BattleSide::ATTACKER,
			modded,
			4,
			preferences,
			std::make_shared<UpperBoundClassicBattleAIRng>(),
			nullptr
		),
		std::domain_error
	);
}

TEST_F(ClassicBattleAIIntegrationTest, EvaluatesHeroSpellAndRetreatWithRealHeroState)
{
	auto callback = battleCallback();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::MAGIC_ARROW);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;
	attackerSideHero->exp = 5000;

	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, trace);
	EXPECT_TRUE(spells.canChooseHeroSpell(BattleSide::ATTACKER));
	ClassicCombatParameters parameters;
	const ClassicScoredSpell spell = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);
	EXPECT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.actionType, EActionType::HERO_SPELL);

	parameters.friendlyCombatValue = 1;
	parameters.enemyCombatValue = 100000;
	addStack(BattleSide::DEFENDER, CreatureID(13), BattleHex(120), 10000);
	ClassicRetreatEvaluator retreat(callback, rng, trace);
	EXPECT_TRUE(retreat.shouldRetreat(BattleSide::ATTACKER, 4, parameters));
}

TEST_F(ClassicBattleAIIntegrationTest, CastsHeroSpellWithMageManaDiscount)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(35), BattleHex(35), 10); // Arch Mages
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::MAGIC_ARROW);
	attackerSideHero->mana = 3;

	auto callback = battleCallback();
	const CSpell * magicArrow = SpellID(SpellID::MAGIC_ARROW).toSpell();
	ASSERT_EQ(callback->battleGetSpellCost(magicArrow, attackerSideHero), 3);
	ASSERT_GT(attackerSideHero->getSpellCost(magicArrow), attackerSideHero->mana);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr);
	ClassicCombatValue values(callback);
	const ClassicScoredSpell spell = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, values.buildParameters(BattleSide::ATTACKER, 4));

	ASSERT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.spell, SpellID::MAGIC_ARROW);
	EXPECT_EQ(spell.score, spell.rawValue);
}

TEST_F(ClassicBattleAIIntegrationTest, ConservesManaUsingPegasusSpellSurcharge)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, CreatureID(20), BattleHex(151), 10); // Pegasi
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::MAGIC_ARROW);
	attackerSideHero->mana = 10;

	auto callback = battleCallback();
	const CSpell * magicArrow = SpellID(SpellID::MAGIC_ARROW).toSpell();
	ASSERT_EQ(callback->battleGetSpellCost(magicArrow, attackerSideHero), 7);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr);
	ClassicCombatValue values(callback);
	const ClassicScoredSpell spell = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, values.buildParameters(BattleSide::ATTACKER, 4));

	ASSERT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.spell, SpellID::MAGIC_ARROW);
	EXPECT_EQ(spell.score, spell.rawValue);
}

TEST_F(ClassicBattleAIIntegrationTest, ValuesProjectedDamageEnchantmentsAgainstLikelyAttacks)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(35), 100);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(151), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	for(SpellID spellID : {
		SpellID::BLESS,
		SpellID::CURSE,
		SpellID::SHIELD,
		SpellID::AIR_SHIELD,
		SpellID::FORGETFULNESS})
	{
		SCOPED_TRACE(spellID.getNum());
		attackerSideHero->addSpellToSpellbook(spellID);
		ClassicAttackEvaluator attacks(callback, rng, nullptr);
		ClassicSpellEvaluator spells(
			gameHandler.get(), callback, rng, nullptr, &attacks);
		const ClassicScoredSpell choice = spells.chooseHeroSpell(
			BattleSide::ATTACKER, false, parameters);
		ASSERT_TRUE(choice.valid);
		EXPECT_EQ(choice.action.spell, spellID);
		EXPECT_GT(choice.rawValue, 0);
		attackerSideHero->removeSpellFromSpellbook(spellID);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, FireShieldValuesReflectedMeleeDamage)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FIRE_SHIELD);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::FIRE_SHIELD);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, friendly->unitId());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeValuesTheNextIncomingMeleeAttack)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(87), 100);
	CStack * firstEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(86), 100);
	CStack * secondEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_TRUE(callback->isMeleeAttackPossible(firstEnemy, friendly, firstEnemy->getPosition()));
	ASSERT_TRUE(callback->isMeleeAttackPossible(secondEnemy, friendly, secondEnemy->getPosition()));
	ASSERT_EQ(friendly->counterAttacks.available(), 1);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::COUNTERSTRIKE);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, friendly->unitId());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeRestoresAnExhaustedRetaliation)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	JsonNode state = friendly->save();
	state["state"]["counterAttacks"]["used"].Integer() = friendly->counterAttacks.total();
	friendly->load(state);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_EQ(friendly->counterAttacks.available(), 0);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::COUNTERSTRIKE);
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeDoesNotValueUnlimitedRetaliations)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:royalGriffin"), BattleHex(87), 100);
	CStack * firstEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	CStack * secondEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(70), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_FALSE(friendly->counterAttacks.isLimited());
	ASSERT_TRUE(callback->isMeleeAttackPossible(firstEnemy, friendly, firstEnemy->getPosition()));
	ASSERT_TRUE(callback->isMeleeAttackPossible(secondEnemy, friendly, secondEnemy->getPosition()));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeValuesAnAttackItDeters)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:archangel"), BattleHex(87), 100);
	CStack * enemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	JsonNode state = friendly->save();
	state["state"]["counterAttacks"]["used"].Integer() = friendly->counterAttacks.total();
	friendly->load(state);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_EQ(friendly->counterAttacks.available(), 0);
	ASSERT_TRUE(callback->isMeleeAttackPossible(enemy, friendly, enemy->getPosition()));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::COUNTERSTRIKE);
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeUsesResetRetaliationsInLaterRounds)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	CStack * enemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	JsonNode friendlyState = friendly->save();
	friendlyState["state"]["counterAttacks"]["used"].Integer() = friendly->counterAttacks.total();
	friendly->load(friendlyState);
	JsonNode enemyState = enemy->save();
	enemyState["state"]["moved"].Bool() = true;
	enemy->load(enemyState);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_EQ(friendly->counterAttacks.available(), 0);
	ASSERT_TRUE(enemy->moved());
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeValuesRetaliationsAfterBlindExpires)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(87), 100);
	CStack * firstEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(86), 100);
	CStack * secondEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	for(BonusType type : {BonusType::NOT_ACTIVE, BonusType::NO_RETALIATION})
	{
		auto blind = std::make_shared<Bonus>(
			BonusDuration::N_TURNS,
			type,
			BonusSource::SPELL_EFFECT,
			0,
			BonusSourceID(SpellID(SpellID::BLIND)));
		blind->turnsRemain = 1;
		friendly->addNewBonus(blind);
	}
	for(CStack * enemy : {firstEnemy, secondEnemy})
	{
		JsonNode state = enemy->save();
		state["state"]["moved"].Bool() = true;
		enemy->load(state);
	}
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_EQ(friendly->counterAttacks.available(), 0);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 2;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::COUNTERSTRIKE);
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CounterstrikeValuesRefreshingAnExpiringEffect)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(87), 100);
	CStack * firstEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(86), 100);
	CStack * secondEnemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	auto expiringCounterstrike = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::ADDITIONAL_RETALIATION,
		BonusSource::SPELL_EFFECT,
		1,
		BonusSourceID(SpellID(SpellID::COUNTERSTRIKE)));
	expiringCounterstrike->turnsRemain = 1;
	friendly->addNewBonus(expiringCounterstrike);
	for(CStack * enemy : {firstEnemy, secondEnemy})
	{
		JsonNode state = enemy->save();
		state["state"]["moved"].Bool() = true;
		enemy->load(state);
	}
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::COUNTERSTRIKE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 2;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::COUNTERSTRIKE);
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkValuesAForcedAttackAgainstTheEnemyArmy)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	CStack * berserker = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(87), 100);
	CStack * redirectedTarget = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(88), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BERSERK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_TRUE(callback->isMeleeAttackPossible(
		berserker, redirectedTarget, berserker->getPosition()));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::BERSERK);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().hexValue, berserker->getPosition());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkAvoidsAreaCollateralOnFriendlyStacks)
{
	removeAllStacks();
	CStack * firstFriendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:crusader"), BattleHex(52), 1000);
	CStack * secondFriendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:crusader"), BattleHex(53), 1000);
	CStack * berserker = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(87), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:skeleton"), BattleHex(88), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BERSERK);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::FIRE_MAGIC, MasteryLevel::ADVANCED, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::BERSERK);
	ASSERT_EQ(choice.action.target.size(), 1);
	const BattleHex center = choice.action.target.front().hexValue;
	EXPECT_LE(BattleHex::getDistance(center, berserker->getPosition()), 1);
	EXPECT_GT(BattleHex::getDistance(center, firstFriendly->getPosition()), 1);
	EXPECT_GT(BattleHex::getDistance(center, secondFriendly->getPosition()), 1);
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkDoesNotProjectAnAttackFromABlindedStack)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:skeleton"), BattleHex(35), 100);
	CStack * berserker = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(87), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:skeleton"), BattleHex(88), 100);
	auto blind = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::NOT_ACTIVE,
		BonusSource::SPELL_EFFECT,
		0,
		BonusSourceID(SpellID(SpellID::BLIND)));
	blind->turnsRemain = 3;
	berserker->addNewBonus(blind);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BERSERK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_FALSE(berserker->canMove());
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 1);
	parameters.roundsLeft = 1;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkDoesNotReceivePreventionValueFromOneTurnBlind)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:skeleton"), BattleHex(35), 100);
	CStack * berserker = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(87), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:skeleton"), BattleHex(88), 100);
	auto blind = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::NOT_ACTIVE,
		BonusSource::SPELL_EFFECT,
		0,
		BonusSourceID(SpellID(SpellID::BLIND)));
	blind->turnsRemain = 1;
	berserker->addNewBonus(blind);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BERSERK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 1);
	parameters.roundsLeft = 1;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkSimulationPreservesLiveTargetTieOrder)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(86), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(87), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:skeleton"), BattleHex(88), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BERSERK);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::FIRE_MAGIC, MasteryLevel::ADVANCED, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, CureAndDispelValueRemovingBerserk)
{
	removeAllStacks();
	CStack * berserker = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(87), 100);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(88), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(35), 100);
	auto berserk = std::make_shared<Bonus>(
		BonusDuration::UNTIL_OWN_ATTACK,
		BonusType::ATTACKS_NEAREST_CREATURE,
		BonusSource::SPELL_EFFECT,
		1,
		BonusSourceID(SpellID(SpellID::BERSERK)));
	berserker->addNewBonus(berserk);
	battle()->nodeHasChanged();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	for(SpellID spellID : {SpellID::CURE, SpellID::DISPEL})
	{
		SCOPED_TRACE(spellID.getNum());
		attackerSideHero->addSpellToSpellbook(spellID);
		const ClassicScoredSpell choice = spells.chooseHeroSpell(
			BattleSide::ATTACKER, false, parameters);
		ASSERT_TRUE(choice.valid);
		EXPECT_EQ(choice.action.spell, spellID);
		EXPECT_GT(choice.rawValue, 0);
		attackerSideHero->removeSpellFromSpellbook(spellID);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, BlessDoesNotValueDamageBeyondTheTargetHealth)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 1);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BLESS);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, EnchantmentMustSurviveUntilTheProjectedAttack)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BLESS);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 3;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, ForgetfulnessDoesNotValueAShooterWithoutAmmunition)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	CStack * marksmen = addStack(
		BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(151), 100);
	JsonNode state = marksmen->save();
	state["state"]["shots"]["used"].Integer() = marksmen->shots.total();
	marksmen->load(state);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FORGETFULNESS);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_FALSE(callback->battleCanShoot(marksmen));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, FullForgetfulnessRemovesTheImmediateRangedAttack)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	CStack * marksmen = addStack(
		BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FORGETFULNESS);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::WATER_MAGIC, 3, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	ASSERT_TRUE(callback->battleCanShoot(marksmen));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::FORGETFULNESS);
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, BlessCanImproveABreakEvenExchange)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 1);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BLESS);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::BLESS);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, friendly->unitId());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, OneTurnEnchantmentsIgnoreCompletedActions)
{
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->setPrimarySkill(
		PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;
	for(SpellID spellID : {SpellID::BLESS, SpellID::SHIELD})
	{
		SCOPED_TRACE(spellID.getNum());
		removeAllStacks();
		CStack * friendly = addStack(
			BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(35), 100);
		CStack * enemy = addStack(
			BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(151), 100);
		CStack * completed = spellID == SpellID::BLESS ? friendly : enemy;
		JsonNode state = completed->save();
		state["state"]["waiting"].Bool() = false;
		state["state"]["moved"].Bool() = true;
		completed->load(state);
		attackerSideHero->addSpellToSpellbook(spellID);

		auto callback = battleCallback();
		auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
		ClassicCombatValue values(callback);
		const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
		ClassicAttackEvaluator attacks(callback, rng, nullptr);
		ClassicSpellEvaluator spells(
			gameHandler.get(), callback, rng, nullptr, &attacks);

		EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
		attackerSideHero->removeSpellFromSpellbook(spellID);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, OneTurnShieldProtectsTheActiveAttackerFromRetaliation)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	CStack * enemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	JsonNode enemyState = enemy->save();
	enemyState["state"]["waiting"].Bool() = false;
	enemyState["state"]["moved"].Bool() = true;
	enemy->load(enemyState);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::SHIELD);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::SHIELD);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, friendly->unitId());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, OneTurnBlessImprovesTheSpentDefendersRetaliation)
{
	removeAllStacks();
	CStack * friendly = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 100);
	JsonNode friendlyState = friendly->save();
	friendlyState["state"]["waiting"].Bool() = false;
	friendlyState["state"]["moved"].Bool() = true;
	friendly->load(friendlyState);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::BLESS);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::BLESS);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, friendly->unitId());
	EXPECT_GT(choice.rawValue, 0);
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportDoesNotGrantASpentStackAnotherAction)
{
	removeAllStacks();
	CStack * pikemen = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 10);
	JsonNode state = pikemen->save();
	state["state"]["waiting"].Bool() = false;
	state["state"]["moved"].Bool() = true;
	pikemen->load(state);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::TELEPORT);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 1;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);

	EXPECT_FALSE(spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportProjectionReleasesDendroidBind)
{
	removeAllStacks();
	CStack * binder = addStack(
		BattleSide::ATTACKER, creatureByName("core:dendroidGuard"), BattleHex(86), 10);
	CStack * bound = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(87), 100);
	auto bind = std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::BIND_EFFECT,
		BonusSource::CREATURE_ABILITY,
		0,
		BonusSourceID());
	bind->parameters = std::make_shared<BonusParameters>(static_cast<int32_t>(binder->unitId()));
	bound->addNewBonus(bind);
	auto callback = battleCallback();
	ASSERT_EQ(ClassicRulesAdapter::movementRangeAfterBindCleanup(*callback, bound), 0);

	HypotheticBattle after(gameHandler.get(), callback);
	after.moveUnit(binder->unitId(), BattleHex(35));
	const battle::Unit * changedBound = after.battleGetUnitByID(bound->unitId());
	ASSERT_NE(changedBound, nullptr);
	EXPECT_EQ(
		ClassicRulesAdapter::movementRangeAfterBindCleanup(after, changedBound),
		static_cast<uint32_t>(changedBound->getInitiative(0)));
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportProjectionPreservesBindForTheActiveStack)
{
	removeAllStacks();
	CStack * binder = addStack(
		BattleSide::ATTACKER, creatureByName("core:dendroidGuard"), BattleHex(86), 10);
	CStack * bound = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(87), 100);
	auto bind = std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::BIND_EFFECT,
		BonusSource::CREATURE_ABILITY,
		0,
		BonusSourceID());
	bind->parameters = std::make_shared<BonusParameters>(static_cast<int32_t>(binder->unitId()));
	bound->addNewBonus(bind);
	battle()->activeStack = bound->unitId();

	HypotheticBattle after(gameHandler.get(), battleCallback());
	after.moveUnit(bound->unitId(), BattleHex(35));
	const battle::Unit * changedBound = after.battleGetUnitByID(bound->unitId());
	ASSERT_NE(changedBound, nullptr);
	ASSERT_FALSE(vstd::contains(after.battleAdjacentUnits(changedBound),
		after.battleGetUnitByID(binder->unitId())));
	EXPECT_EQ(ClassicRulesAdapter::movementRangeAfterBindCleanup(after, changedBound), 0);
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportPlacesAnActiveBoundStackInImmediateAttackRange)
{
	removeAllStacks();
	CStack * bound = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(87), 100);
	CStack * binder = addStack(
		BattleSide::DEFENDER, creatureByName("core:dendroidGuard"), BattleHex(86), 10000);
	CStack * target = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 1);
	auto bind = std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::BIND_EFFECT,
		BonusSource::CREATURE_ABILITY,
		0,
		BonusSourceID());
	bind->parameters = std::make_shared<BonusParameters>(static_cast<int32_t>(binder->unitId()));
	bound->addNewBonus(bind);
	battle()->activeStack = bound->unitId();
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::TELEPORT);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 1;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::TELEPORT);
	ASSERT_EQ(choice.action.target.size(), 2);
	EXPECT_EQ(choice.action.target.front().unitValue, bound->unitId());
	EXPECT_TRUE(vstd::contains(
		target->getAttackableHexes(bound), choice.action.target.back().hexValue));
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportValuesAShorterRouteToTheProjectedTarget)
{
	removeAllStacks();
	CStack * pikemen = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(35), 100);
	CStack * enemy = addStack(
		BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 10);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::TELEPORT);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::TELEPORT);
	EXPECT_GT(choice.rawValue, 0);
	ASSERT_EQ(choice.action.target.size(), 2);
	EXPECT_EQ(choice.action.target.front().unitValue, pikemen->unitId());
	uint32_t distanceToAttack = ReachabilityInfo::INFINITE_DIST;
	for(const BattleHex & landing : enemy->getAttackableHexes(pikemen))
	{
		distanceToAttack = std::min<uint32_t>(distanceToAttack,
			BattleHex::getDistance(choice.action.target.back().hexValue, landing));
	}
	EXPECT_LE(distanceToAttack, pikemen->getMovementRange());
}

TEST_F(ClassicBattleAIIntegrationTest, TeleportValuesAnAttackForATrappedMeleeStack)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * pikemen = addStack(
		BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(86), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(151), 10);
	auto enclosure = std::make_shared<SpellCreatedObstacle>();
	enclosure->uniqueID = 0;
	enclosure->pos = pikemen->getPosition();
	enclosure->customSize = pikemen->getPosition().getNeighbouringTiles();
	battle()->obstacles.push_back(std::move(enclosure));
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::TELEPORT);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	const ReachabilityInfo trapped = callback->getReachability(pikemen);
	EXPECT_TRUE(std::ranges::none_of(
		pikemen->getPosition().getNeighbouringTiles(),
		[&](const BattleHex & hex)
		{
			return trapped.isReachable(hex);
		}));
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 3;
	ClassicAttackEvaluator attacks(callback, rng, nullptr);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, nullptr, &attacks);
	const ClassicScoredSpell choice = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.spell, SpellID::TELEPORT);
	EXPECT_GT(choice.rawValue, 0);
	ASSERT_EQ(choice.action.target.size(), 2);
	EXPECT_EQ(choice.action.target.front().unitValue, pikemen->unitId());
}

TEST_F(ClassicBattleAIIntegrationTest, EvaluatesSingleTargetBasicHaste)
{
	removeAllStacks();
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::HASTE);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::AIR_MAGIC, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(
		PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, trace);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, trace, &attacks);
	const ClassicScoredSpell spell = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.actionType, EActionType::HERO_SPELL);
	EXPECT_EQ(spell.action.spell, SpellID::HASTE);
	ASSERT_FALSE(spell.action.target.empty());
	EXPECT_EQ(spell.action.target.front().unitValue, pikemen->unitId());
	EXPECT_TRUE(std::ranges::any_of(
		trace->getEntries(),
		[pikemen](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "spell.haste.stack"
				&& entry.key == std::to_string(pikemen->unitId());
		}));
}

TEST_F(ClassicBattleAIIntegrationTest, EvaluatesSingleTargetBasicSlow)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 100);
	CStack * pikemen = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::SLOW);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::EARTH_MAGIC, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(
		PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator attacks(callback, rng, trace);
	ClassicSpellEvaluator spells(
		gameHandler.get(), callback, rng, trace, &attacks);
	const ClassicScoredSpell spell = spells.chooseHeroSpell(
		BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.actionType, EActionType::HERO_SPELL);
	EXPECT_EQ(spell.action.spell, SpellID::SLOW);
	ASSERT_FALSE(spell.action.target.empty());
	EXPECT_EQ(spell.action.target.front().unitValue, pikemen->unitId());
	EXPECT_TRUE(std::ranges::any_of(
		trace->getEntries(),
		[pikemen](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "spell.slow.stack"
				&& entry.key == std::to_string(pikemen->unitId());
		}));
}

TEST_F(ClassicBattleAIIntegrationTest, SpeedSpellsValueBoundAdjacentAttacksButDoNotEnableMovement)
{
	removeAllStacks();
	const uint32_t friendlyId = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(leftHex), 100)->unitId();
	const uint32_t enemyId = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(rightHex), 100)->unitId();
	CStack * friendly = battle()->getStack(friendlyId);
	CStack * enemy = battle()->getStack(enemyId);
	for(CStack * stack : {friendly, enemy})
	{
		stack->addNewBonus(std::make_shared<Bonus>(
			BonusDuration::ONE_BATTLE, BonusType::BIND_EFFECT, BonusSource::CREATURE_ABILITY, 0,
			BonusSourceID()));
		ASSERT_EQ(stack->getMovementRange(), 0);
		ASSERT_GT(stack->getInitiative(0), 0);
	}
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 5, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;
	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	for(SpellID spellID : {SpellID::HASTE, SpellID::SLOW})
	{
		SCOPED_TRACE(spellID.getNum());
		attackerSideHero->addSpellToSpellbook(spellID);
		JsonNode enemyState = enemy->save();
		enemyState["state"]["position"].Integer() = rightHex;
		enemy->load(enemyState);
		ClassicAttackEvaluator attacks(callback, rng, nullptr);
		ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr, &attacks);
		const auto adjacentSpell = spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters);
		ASSERT_TRUE(adjacentSpell.valid);
		EXPECT_EQ(adjacentSpell.action.spell, spellID);
		ASSERT_EQ(adjacentSpell.action.target.size(), 1);
		EXPECT_EQ(adjacentSpell.action.target.front().unitValue,
			spellID == SpellID::HASTE ? friendlyId : enemyId);

		enemyState["state"]["position"].Integer() = 151;
		enemy->load(enemyState);
		ClassicAttackEvaluator distantAttacks(callback, rng, nullptr);
		ClassicSpellEvaluator distantSpells(
			gameHandler.get(), callback, rng, nullptr, &distantAttacks);
		EXPECT_FALSE(distantSpells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).valid);
		attackerSideHero->removeSpellFromSpellbook(spellID);
	}
}

TEST_F(ClassicBattleAIIntegrationTest, SlowSubtractsTheWaitedTurnFromUsableDuration)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 100);
	CStack * pikemen = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::SLOW);
	attackerSideHero->setSecSkillLevel(
		SecondarySkill::EARTH_MAGIC, 1, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto evaluate = [&]()
	{
		auto callback = battleCallback();
		auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
		ClassicCombatValue values(callback);
		ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
		parameters.roundsLeft = 7;
		ClassicAttackEvaluator attacks(callback, rng, nullptr);
		ClassicSpellEvaluator spells(
			gameHandler.get(), callback, rng, nullptr, &attacks);
		return spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters).rawValue;
	};

	attackerSideHero->setPrimarySkill(
		PrimarySkill::SPELL_POWER, 1, ChangeValueMode::ABSOLUTE);
	pikemen->waiting = false;
	const int64_t oneUsableTurn = evaluate();
	ASSERT_GT(oneUsableTurn, 0);

	attackerSideHero->setPrimarySkill(
		PrimarySkill::SPELL_POWER, 2, ChangeValueMode::ABSOLUTE);
	pikemen->waiting = true;
	EXPECT_EQ(evaluate(), oneUsableTurn);
}

TEST_F(ClassicBattleAIIntegrationTest, MassSlowDoesNotValueImmuneStacks)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 200);
	CStack * immune = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 100);
	CStack * susceptible = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(149), 100);
	immune->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::PERMANENT, BonusType::SPELL_IMMUNITY, BonusSource::OTHER, 0,
		BonusSourceID(), BonusSubtypeID(SpellID(SpellID::SLOW))));
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::SLOW);
	attackerSideHero->setSecSkillLevel(SecondarySkill::EARTH_MAGIC, 3, ChangeValueMode::ABSOLUTE);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;

	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	parameters.roundsLeft = 7;
	ClassicAttackEvaluator attacks(callback, rng, trace);
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, trace, &attacks);
	const ClassicScoredSpell spell = spells.chooseHeroSpell(BattleSide::ATTACKER, false, parameters);

	ASSERT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.spell, SpellID::SLOW);
	EXPECT_TRUE(spell.action.target.empty());
	bool scoredSusceptible = false;
	for(const ClassicDecisionTraceEntry & entry : trace->getEntries())
	{
		if(entry.stage != "spell.slow.stack")
			continue;
		EXPECT_NE(entry.key, std::to_string(immune->unitId()));
		if(entry.key == std::to_string(susceptible->unitId()) && entry.value > 0)
			scoredSusceptible = true;
	}
	EXPECT_TRUE(scoredSusceptible);
}

TEST_F(ClassicBattleAIIntegrationTest, HeroSpellPipelineProjectsBeforeRetreatEligibilityGate)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(86), 100);
	addStack(BattleSide::DEFENDER, CreatureID(94), BattleHex(15), 2);
	addStack(BattleSide::DEFENDER, CreatureID(94), BattleHex(83), 2);
	addStack(BattleSide::DEFENDER, CreatureID(95), BattleHex(117), 2);
	addStack(BattleSide::DEFENDER, CreatureID(94), BattleHex(185), 1);
	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::MAGIC_ARROW);
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 100;
	attackerSideHero->exp = 0;

	auto rng = std::make_shared<ReplayClassicBattleAIRng>(
		std::vector<int32_t>{92});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	AutocombatPreferences preferences;
	preferences.enableSpellsUsage = true;
	const BattleAction action = ClassicBattleDecision::decide(
		gameHandler.get(), battleCallback(), BattleSide::ATTACKER, pikemen, 1,
		preferences, rng, trace, ClassicDecisionEntryPoint::DO_SPELL_AI);

	EXPECT_EQ(action.actionType, EActionType::HERO_SPELL);
	EXPECT_EQ(action.spell, SpellID::MAGIC_ARROW);
	ASSERT_GE(trace->getEntries().size(), 2);
	EXPECT_TRUE(std::ranges::any_of(trace->getEntries(), [](const auto & entry)
	{
		return entry.stage == "spell.prepass" && entry.key == "begin";
	}));
	EXPECT_TRUE(std::ranges::any_of(trace->getEntries(), [](const auto & entry)
	{
		return entry.stage == "spell.prepass" && entry.key == "end";
	}));
	EXPECT_EQ(rng->consumed(), 1);
	EXPECT_TRUE(rng->exhausted());
}

TEST_F(ClassicBattleAIIntegrationTest, RetreatRequiresSurrenderCostPlusTwentyFiveHundredGold)
{
	attackerSideHero->exp = 5000;
	// Keep the side alive through the one-action projection while retaining a
	// clearly losing raw fight-value share. Otherwise projected extinction is
	// an earlier unconditional retreat gate and the treasury boundary is never
	// reached.
	addStack(BattleSide::ATTACKER, CreatureID(56), BattleHex(52), 100000);
	addStack(BattleSide::DEFENDER, CreatureID(13), BattleHex(120), 10000);
	auto callback = battleCallback();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicRetreatEvaluator retreat(callback, rng, nullptr, gameHandler.get());
	ClassicCombatParameters parameters;

	auto setGold = [&](int32_t amount)
	{
		SetResources resources;
		resources.player = attackerSideHero->getOwner();
		resources.mode = ChangeValueMode::ABSOLUTE;
		resources.res[GameResID::GOLD] = amount;
		gameHandler->sendAndApply(resources);
	};
	const int32_t required = callback->battleGetSurrenderCost(attackerSideHero->getOwner()) + 2500;
	ASSERT_GT(required, 0);
	setGold(required - 1);
	EXPECT_FALSE(retreat.shouldRetreat(BattleSide::ATTACKER, 4, parameters));
	setGold(required);
	EXPECT_TRUE(retreat.shouldRetreat(BattleSide::ATTACKER, 4, parameters));
}

TEST_F(ClassicBattleAIIntegrationTest, LocalHumanAutocombatSkipsTheLowDifficultyRoll)
{
	attackerSideHero->exp = 0;
	while(!attackerSideHero->artifactsWorn.empty())
		attackerSideHero->removeArtifact(attackerSideHero->artifactsWorn.begin()->first);
	while(!attackerSideHero->artifactsInBackpack.empty())
		attackerSideHero->removeArtifact(ArtifactPosition::BACKPACK_START);
	PlayerState * player = gameState()->getPlayerState(attackerSideHero->getOwner());
	player->human = true;
	auto callback = battleCallback();
	auto humanRng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	ClassicRetreatEvaluator humanRetreat(callback, humanRng, nullptr, gameHandler.get());
	ClassicCombatParameters parameters;
	EXPECT_FALSE(humanRetreat.shouldRetreat(BattleSide::ATTACKER, 1, parameters));
	EXPECT_EQ(humanRng->consumed(), 0);

	player->human = false;
	auto aiRng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{50});
	ClassicRetreatEvaluator aiRetreat(callback, aiRng, nullptr, gameHandler.get());
	EXPECT_FALSE(aiRetreat.shouldRetreat(BattleSide::ATTACKER, 1, parameters));
	EXPECT_EQ(aiRng->consumed(), 1);
}

TEST_F(ClassicBattleAIIntegrationTest, NormalDifficultyRollFiftyStopsAndFiftyOneContinues)
{
	removeAllStacks();
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 1);
	addStack(BattleSide::DEFENDER, CreatureID(13), BattleHex(151), 100);
	attackerSideHero->exp = 5000;
	gameState()->getPlayerState(attackerSideHero->getOwner())->human = false;
	const std::map<uint32_t, int64_t> projectedDamage = {
		{pikemen->unitId(), pikemen->getAvailableHealth()}
	};
	ClassicCombatParameters parameters;

	auto stoppedRng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{50});
	ClassicRetreatEvaluator stopped(battleCallback(), stoppedRng, nullptr, gameHandler.get());
	EXPECT_FALSE(stopped.shouldRetreat(
		BattleSide::ATTACKER, 1, parameters, &projectedDamage));
	EXPECT_TRUE(stoppedRng->exhausted());

	auto continuedRng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{51});
	ClassicRetreatEvaluator continued(battleCallback(), continuedRng, nullptr, gameHandler.get());
	EXPECT_TRUE(continued.shouldRetreat(
		BattleSide::ATTACKER, 1, parameters, &projectedDamage));
	EXPECT_TRUE(continuedRng->exhausted());
}

TEST_F(ClassicBattleAIIntegrationTest, PatrollingHeroNeverRetreats)
{
	removeAllStacks();
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(35), 1);
	addStack(BattleSide::DEFENDER, CreatureID(13), BattleHex(151), 100);
	attackerSideHero->exp = 5000;
	const ClassicExpectedDamage projectedDamage = {
		{pikemen->unitId(), pikemen->getAvailableHealth()}
	};
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicRetreatEvaluator retreat(battleCallback(), rng, nullptr, gameHandler.get());
	ClassicCombatParameters parameters;

	attackerSideHero->patrol.patrolling = false;
	ASSERT_TRUE(retreat.shouldRetreat(
		BattleSide::ATTACKER, 4, parameters, &projectedDamage));
	attackerSideHero->patrol.patrolling = true;
	EXPECT_FALSE(retreat.shouldRetreat(
		BattleSide::ATTACKER, 4, parameters, &projectedDamage));
}

TEST_F(ClassicBattleAIIntegrationTest, RetreatUsesProjectedExtinctionBeforeTheGoldGate)
{
	removeAllStacks();
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(41), BattleHex(35), 100);
	CStack * victim = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 1);
	attackerSideHero->exp = 0;
	defenderSideHero->exp = 5000;
	auto callback = battleCallback();
	ASSERT_TRUE(callback->battleCanShoot(shooter, victim->getPosition()));

	SetResources resources;
	resources.player = defenderSideHero->getOwner();
	resources.mode = ChangeValueMode::ABSOLUTE;
	resources.res[GameResID::GOLD] = 0;
	gameHandler->sendAndApply(resources);

	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::DEFENDER, 4);
	ClassicAttackEvaluator projection(callback, rng, trace);
	const ClassicExpectedDamage expected = projection.projectExpectedDamage(
		BattleSide::DEFENDER, true, parameters);
	ASSERT_TRUE(expected.contains(victim->unitId()));
	EXPECT_EQ(expected.at(victim->unitId()), victim->getAvailableHealth());

	ClassicRetreatEvaluator retreat(callback, rng, trace, gameHandler.get());
	EXPECT_TRUE(retreat.shouldRetreat(BattleSide::DEFENDER, 4, parameters));
}

TEST_F(ClassicBattleAIIntegrationTest, ProjectionRetargetsAfterAnEarlierActorKillsThePreferredEnemy)
{
	removeAllStacks();
	addStack(BattleSide::ATTACKER, CreatureID(41), BattleHex(35), 100);
	addStack(BattleSide::ATTACKER, CreatureID(41), BattleHex(69), 100);
	CStack * first = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(117), 1);
	CStack * second = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(151), 1);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr);

	const auto expected = evaluator.projectExpectedDamage(BattleSide::ATTACKER, false, parameters);
	ASSERT_TRUE(expected.contains(first->unitId()));
	ASSERT_TRUE(expected.contains(second->unitId()));
	EXPECT_EQ(expected.at(first->unitId()), first->getAvailableHealth());
	EXPECT_EQ(expected.at(second->unitId()), second->getAvailableHealth());
	EXPECT_TRUE(first->alive());
	EXPECT_TRUE(second->alive());
}

TEST_F(ClassicBattleAIIntegrationTest, MultiheadedCollateralIsValuedAndProjected)
{
	removeAllStacks();
	battle()->obstacles.clear();
	const uint32_t hydraId = addStack(
		BattleSide::ATTACKER, creatureByName("core:hydra"), BattleHex(87), 100)->unitId();
	const uint32_t firstId = addStack(
		BattleSide::DEFENDER, CreatureID(0), BattleHex(88), 100)->unitId();
	const uint32_t secondId = addStack(
		BattleSide::DEFENDER, CreatureID(0), BattleHex(70), 100)->unitId();
	CStack * hydra = battle()->getStack(hydraId);
	CStack * first = battle()->getStack(firstId);
	CStack * second = battle()->getStack(secondId);
	auto callback = battleCallback();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ASSERT_TRUE(callback->isMeleeAttackPossible(hydra, first, hydra->getPosition()));
	const ReachabilityInfo hydraReachability = callback->getReachability(hydra);
	EXPECT_TRUE(hydraReachability.isReachable(hydra->getPosition()));
	EXPECT_EQ(hydraReachability.distances[hydra->getPosition().toInt()], 0);
	EXPECT_TRUE(first->getAttackableHexes(hydra).contains(hydra->getPosition()));
	ClassicAttackEvaluator evaluator(
		callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace);

	const auto action = evaluator.chooseAction(hydra, parameters);
	ASSERT_TRUE(action.valid);
	EXPECT_EQ(action.action.actionType, EActionType::WALK_AND_ATTACK);
	EXPECT_TRUE(std::ranges::any_of(trace->getEntries(), [](const ClassicDecisionTraceEntry & entry)
	{
		return entry.stage == "attack_hex.adjacent_final" && entry.value > 0;
	}));
	const auto damage = evaluator.projectExpectedDamage(BattleSide::ATTACKER, false, parameters);
	ASSERT_TRUE(damage.contains(first->unitId()));
	ASSERT_TRUE(damage.contains(second->unitId()));
	EXPECT_GT(damage.at(first->unitId()), 0);
	EXPECT_GT(damage.at(second->unitId()), 0);
}

TEST_F(ClassicBattleAIIntegrationTest, DragonBreathAvoidsFriendlyFire)
{
	removeAllStacks();
	battle()->obstacles.clear();
	const uint32_t dragonId = addStack(
		BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(87), 10)->unitId();
	const uint32_t targetId = addStack(
		BattleSide::DEFENDER, CreatureID(0), BattleHex(88), 100)->unitId();
	const uint32_t friendlyId = addStack(
		BattleSide::ATTACKER, CreatureID(0), BattleHex(89), 100)->unitId();
	CStack * dragon = battle()->getStack(dragonId);
	CStack * target = battle()->getStack(targetId);
	CStack * friendly = battle()->getStack(friendlyId);
	auto callback = battleCallback();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ASSERT_TRUE(callback->isMeleeAttackPossible(dragon, target, dragon->getPosition()));
	ClassicAttackEvaluator evaluator(
		callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace);

	const auto action = evaluator.chooseAction(dragon, parameters);
	ASSERT_TRUE(action.valid);
	EXPECT_EQ(action.action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_GE(action.action.target.size(), 2u);
	EXPECT_NE(action.action.target.front().hexValue, dragon->getPosition());
	EXPECT_TRUE(std::ranges::any_of(trace->getEntries(), [](const ClassicDecisionTraceEntry & entry)
	{
		return entry.stage == "attack_hex.adjacent_final" && entry.value < 0;
	}));
	const auto damage = evaluator.projectExpectedDamage(BattleSide::ATTACKER, false, parameters);
	ASSERT_TRUE(damage.contains(target->unitId()));
	ASSERT_TRUE(damage.contains(friendly->unitId()));
	EXPECT_GT(damage.at(target->unitId()), 0);
	EXPECT_EQ(damage.at(friendly->unitId()), 0);
}

TEST_F(ClassicBattleAIIntegrationTest, BoundSurvivorDoesNotForceRetreat)
{
	removeAllStacks();
	CStack * bound = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(leftHex), 100);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(rightHex), 100);
	bound->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE, BonusType::BIND_EFFECT, BonusSource::CREATURE_ABILITY, 0,
		BonusSourceID()));
	attackerSideHero->exp = 5000;

	auto callback = battleCallback();
	ASSERT_TRUE(callback->battleCanFlee(PlayerColor(0)));
	ClassicBattleStateView view(callback);
	const auto order = view.moveOrder(BattleSide::ATTACKER, false);
	EXPECT_TRUE(std::ranges::any_of(order, [bound](const ClassicMoveOrderEntry & entry)
	{
		return entry.stack == bound;
	}));
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicRetreatEvaluator retreat(callback, rng, nullptr);
	const ClassicExpectedDamage noProjectedDamage;
	EXPECT_FALSE(retreat.shouldRetreat(BattleSide::ATTACKER, 4, parameters, &noProjectedDamage));
}

TEST_F(ClassicBattleAIIntegrationTest, BoundStackCanAttackAdjacentEnemyAndContributesToProjection)
{
	removeAllStacks();
	battle()->obstacles.clear();
	CStack * bound = addStack(BattleSide::ATTACKER, CreatureID(6), BattleHex(86), 100);
	CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(87), 1);
	bound->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE, BonusType::BIND_EFFECT,
		BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));
	ASSERT_EQ(bound->getMovementRange(), 0);
	auto callback = battleCallback();
	ASSERT_TRUE(callback->isMeleeAttackPossible(bound, enemy, bound->getPosition()));
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr);

	const auto action = evaluator.chooseAction(bound, parameters);
	ASSERT_TRUE(action.valid);
	EXPECT_EQ(action.action.actionType, EActionType::WALK_AND_ATTACK);
	const auto targets = evaluator.projectSpellTargets(BattleSide::ATTACKER, parameters);
	ASSERT_TRUE(targets.contains(bound->unitId()));
	EXPECT_EQ(targets.at(bound->unitId()).target, enemy);
	const auto damage = evaluator.projectExpectedDamage(BattleSide::ATTACKER, false, parameters);
	ASSERT_TRUE(damage.contains(enemy->unitId()));
	EXPECT_EQ(damage.at(enemy->unitId()), enemy->getAvailableHealth());
}

TEST_F(ClassicBattleAIIntegrationTest, BerserkStackUsesForcedActionPipeline)
{
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const CStack * attacker = view.orderedStacks().front();
	auto berserk = std::make_shared<Bonus>(
		BonusDuration::N_TURNS,
		BonusType::ATTACKS_NEAREST_CREATURE,
		BonusSource::SPELL_EFFECT,
		1,
		BonusSourceID(SpellID(SpellID::BERSERK))
	);
	berserk->turnsRemain = 1;
	const_cast<CStack *>(attacker)->addNewBonus(berserk);

	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);
	const ClassicScoredAction result = evaluator.chooseAction(attacker, parameters);
	EXPECT_TRUE(result.valid);
	EXPECT_NE(result.action.actionType, EActionType::NO_ACTION);
}

TEST_F(ClassicBattleAIIntegrationTest, MasterGenieSelectsBuffTargetWithoutDrawingTheServerSpell)
{
	const uint32_t casterId = addStack(
		BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(leftHex), 1)->unitId();
	const uint32_t subjectId = addStack(
		BattleSide::ATTACKER, creatureByName("core:nagaQueen"), BattleHex(rightHex + 2), 100)->unitId();
	CStack * caster = battle()->getStack(casterId);
	CStack * subject = battle()->getStack(subjectId);
	beginCombat();
	auto callback = battleCallback();
	ASSERT_TRUE(caster->hasBonusOfType(BonusType::RANDOM_SPELLCASTER));
	ASSERT_FALSE(caster->hasBonusOfType(BonusType::SPELLCASTER));
	const auto candidates = callback->getPossibleBeneficialSpells(caster, subject);
	ASSERT_FALSE(candidates.empty());
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr);
	const auto choice = spells.chooseCreatureSpell(caster, 0, ClassicCombatParameters());

	ASSERT_TRUE(choice.valid);
	EXPECT_EQ(choice.action.actionType, EActionType::MONSTER_SPELL);
	EXPECT_EQ(choice.action.spell, SpellID::NONE);
	ASSERT_EQ(choice.action.target.size(), 1);
	EXPECT_EQ(choice.action.target.front().unitValue, subjectId);
	EXPECT_EQ(rng->consumed(), 0);
	ASSERT_NE(battle()->battleActiveUnit(), nullptr);
	ASSERT_EQ(battle()->battleActiveUnit()->unitId(), casterId);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), choice.action));
	ASSERT_EQ(server.casts.size(), 1);
	EXPECT_TRUE(vstd::contains(candidates, server.casts.front().announcement.spellID));
	EXPECT_EQ(server.casts.front().announcement.casterStack, casterId);
	EXPECT_FALSE(server.casts.front().announcement.castByHero);
}

TEST_F(ClassicBattleAIIntegrationTest, MasterGeniePreservesDeclineRollAndChecksSpellBlockade)
{
	const uint32_t casterId = addStack(
		BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(leftHex), 1)->unitId();
	addStack(BattleSide::ATTACKER, creatureByName("core:nagaQueen"), BattleHex(rightHex + 2), 100);
	CStack * caster = battle()->getStack(casterId);
	auto callback = battleCallback();
	for(int32_t roll : {30, 31})
	{
		SCOPED_TRACE(roll);
		auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{roll});
		ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr);
		EXPECT_EQ(spells.chooseCreatureSpell(caster, 1, ClassicCombatParameters()).valid, roll > 30);
		ASSERT_EQ(rng->getRequests().size(), 1);
		EXPECT_EQ(rng->getRequests().front().lower, 1);
		EXPECT_EQ(rng->getRequests().front().upper, 100);
	}
	caster->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE, BonusType::BLOCK_ALL_MAGIC, BonusSource::OTHER, 1, BonusSourceID()));
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	ClassicSpellEvaluator spells(gameHandler.get(), callback, rng, nullptr);
	EXPECT_FALSE(spells.chooseCreatureSpell(caster, 0, ClassicCombatParameters()).valid);
	EXPECT_EQ(rng->consumed(), 0);
}

TEST_F(ClassicBattleAIIntegrationTest, CreatureCasterEvaluatesLegalSpellTargets)
{
	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(13), BattleHex(leftHex), 10);
	CStack * wounded = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(rightHex), 10);
	JsonNode state = wounded->save();
	state["state"]["health"]["firstHPleft"].Integer() = 1;
	state["state"]["health"]["fullUnits"].Integer() = 8;
	wounded->load(state);

	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicSpellEvaluator spells(gameHandler.get(), battleCallback(), rng, trace);
	ClassicCombatParameters parameters;
	const ClassicScoredSpell spell = spells.chooseCreatureSpell(caster, 0, parameters);
	EXPECT_TRUE(spell.valid);
	EXPECT_EQ(spell.action.actionType, EActionType::MONSTER_SPELL);
}

TEST_F(ClassicBattleAIIntegrationTest, ChoosesRangedAttackWhenShooterHasALegalTarget)
{
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(2), BattleHex(leftHex), 20);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(rightHex + 17), 20);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);

	const ClassicScoredAction result = evaluator.chooseAction(shooter, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::SHOOT);
}

TEST_F(ClassicBattleAIIntegrationTest, SimulatedStrikeIsCappedAtDefenderHealth)
{
	removeAllStacks();
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(2), BattleHex(35), 100);
	CStack * victim = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(rightHex + 17), 1);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(
		callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace);

	const ClassicScoredAction result = evaluator.chooseAction(shooter, parameters);
	ASSERT_TRUE(result.valid);
	ASSERT_EQ(result.action.actionType, EActionType::SHOOT);
	const std::string key = std::to_string(shooter->unitId()) + ":" + std::to_string(victim->unitId());
	const auto strike = std::ranges::find_if(
		trace->getEntries(),
		[&](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "attack_sim.first" && entry.key == key;
		});
	ASSERT_NE(strike, trace->getEntries().end());
	EXPECT_EQ(strike->value, victim->getAvailableHealth());
}

TEST_F(ClassicBattleAIIntegrationTest, DamageMidpointIsTakenAcrossTheWholeStack)
{
	removeAllStacks();
	// Archers deal 2..3 damage. For three creatures the executable takes
	// (2 + 3) * 3 / 2 = 7 before modifiers, rather than (2 + 3) / 2 * 3 = 6.
	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(2), BattleHex(35), 3);
	CStack * victim = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(43), 100);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(
		callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace);

	const ClassicScoredAction result = evaluator.chooseAction(shooter, parameters);
	ASSERT_TRUE(result.valid);
	ASSERT_EQ(result.action.actionType, EActionType::SHOOT);
	const std::string key = std::to_string(shooter->unitId()) + ":" + std::to_string(victim->unitId());
	const auto strike = std::ranges::find_if(
		trace->getEntries(),
		[&](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "attack_sim.first" && entry.key == key;
		});
	ASSERT_NE(strike, trace->getEntries().end());
	EXPECT_EQ(strike->value, 7);
}

TEST_F(ClassicBattleAIIntegrationTest, AreaShooterEvaluatesBothCellsOfDoubleWideTarget)
{
	removeAllStacks();
	CStack * magog = addStack(BattleSide::ATTACKER, CreatureID(45), BattleHex(35), 100);
	CStack * griffin = addStack(BattleSide::DEFENDER, CreatureID(4), BattleHex(112), 20);
	ASSERT_TRUE(griffin->doubleWide());

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	const ClassicScoredAction result = evaluator.chooseAction(magog, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::SHOOT);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().unitValue, griffin->unitId());
	EXPECT_TRUE(griffin->getHexes().contains(result.action.target.front().hexValue));
	std::ostringstream projectionTrace;
	for(const auto & entry : trace->getEntries())
		projectionTrace << entry.stage << ':' << entry.key << '=' << entry.value << ' ';
	EXPECT_TRUE(std::ranges::any_of(
		trace->getEntries(),
		[&](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "shoot.center"
				&& entry.key == std::to_string(griffin->unitId())
				&& griffin->getHexes().contains(BattleHex(entry.value));
		}));
	EXPECT_TRUE(std::ranges::any_of(
		trace->getEntries(),
		[&](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "shoot.target_time"
				&& entry.key == std::to_string(griffin->unitId())
				&& entry.value > 0;
		})) << projectionTrace.str();
}

TEST_F(ClassicBattleAIIntegrationTest, AreaShooterSubtractsFriendlySplashDamage)
{
	removeAllStacks();
	CStack * magog = addStack(BattleSide::ATTACKER, CreatureID(45), BattleHex(35), 100);
	CStack * friendly = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(111), 1);
	// Keep the defender's projected attack time identical after removing the
	// splash victim, so the score delta isolates friendly-fire valuation.
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(116), 1);
	CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(112), 100);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto scoreFor = [&](const std::shared_ptr<ClassicDecisionTrace> & trace)
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[enemy](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == "shoot"
					&& entry.key == std::to_string(enemy->unitId());
			});
		EXPECT_NE(found, trace->getEntries().end());
		return found == trace->getEntries().end() ? int64_t(0) : found->value;
	};

	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto withFriendlyTrace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator withFriendly(callback, rng, withFriendlyTrace);
	const ClassicScoredAction withFriendlyResult = withFriendly.chooseAction(magog, parameters);
	ASSERT_TRUE(withFriendlyResult.valid);
	ASSERT_EQ(withFriendlyResult.action.actionType, EActionType::SHOOT);
	const int64_t withFriendlyScore = scoreFor(withFriendlyTrace);

	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	removal.changedStacks.emplace_back(friendly->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);
	auto withoutFriendlyTrace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator withoutFriendly(callback, rng, withoutFriendlyTrace);
	const ClassicScoredAction withoutFriendlyResult = withoutFriendly.chooseAction(magog, parameters);
	ASSERT_TRUE(withoutFriendlyResult.valid);
	ASSERT_EQ(withoutFriendlyResult.action.actionType, EActionType::SHOOT);
	EXPECT_LT(withFriendlyScore, scoreFor(withoutFriendlyTrace));
}

TEST_F(ClassicBattleAIIntegrationTest, AreaShooterUsesRangedDamageForFriendlySplash)
{
	removeAllStacks();
	CStack * magog = addStack(BattleSide::ATTACKER, CreatureID(45), BattleHex(35), 100);
	CStack * friendly = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(111), 30);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(112), 100);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(
		callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace);
	ASSERT_TRUE(evaluator.chooseAction(magog, parameters).valid);

	const std::string friendlyKey =
		std::to_string(magog->unitId()) + ":" + std::to_string(friendly->unitId());
	const auto friendlyStrike = std::ranges::find_if(
		trace->getEntries(),
		[&](const ClassicDecisionTraceEntry & entry)
		{
			return entry.stage == "attack_sim.first" && entry.key == friendlyKey;
		});
	ASSERT_NE(friendlyStrike, trace->getEntries().end());
	BattleAttackInfo rangedAttack(magog, friendly, 0, true);
	const DamageEstimation rangedEstimate = callback->battleEstimateDamage(rangedAttack);
	const int64_t expectedRangedDamage = std::min<int64_t>(
		friendly->getAvailableHealth(),
		(rangedEstimate.damage.min + rangedEstimate.damage.max) / 2);
	BattleAttackInfo meleeAttack(magog, friendly, 0, false);
	const DamageEstimation meleeEstimate = callback->battleEstimateDamage(meleeAttack);
	const int64_t expectedMeleeDamage = std::min<int64_t>(
		friendly->getAvailableHealth(),
		(meleeEstimate.damage.min + meleeEstimate.damage.max) / 2);
	ASSERT_NE(expectedRangedDamage, expectedMeleeDamage);
	EXPECT_EQ(friendlyStrike->value, expectedRangedDamage);
}

TEST_F(ClassicBattleAIIntegrationTest, DeathCloudSkipsUndeadSplashTargets)
{
	removeAllStacks();
	CStack * lich = addStack(BattleSide::ATTACKER, CreatureID(64), BattleHex(35), 100);
	CStack * primary = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(112), 100);
	CStack * secondary = addStack(BattleSide::DEFENDER, CreatureID(56), BattleHex(111), 100);
	// Preserve the defender side's projection when the adjacent stack is
	// replaced, isolating Death Cloud receptiveness from target-time scoring.
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(116), 1);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const ClassicCombatParameters parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto scoreForPrimary = [&](const std::shared_ptr<ClassicDecisionTrace> & trace)
	{
		ClassicAttackEvaluator evaluator(callback, rng, trace);
		const ClassicScoredAction result = evaluator.chooseAction(lich, parameters);
		EXPECT_TRUE(result.valid);
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[primary](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == "shoot"
					&& entry.key == std::to_string(primary->unitId());
			});
		EXPECT_NE(found, trace->getEntries().end());
		return found == trace->getEntries().end() ? int64_t(0) : found->value;
	};

	auto undeadTrace = std::make_shared<ClassicDecisionTrace>();
	const int64_t undeadScore = scoreForPrimary(undeadTrace);
	BattleUnitsChanged replacement;
	replacement.battleID = BattleID(0);
	replacement.changedStacks.emplace_back(secondary->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(replacement);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(111), 100);
	auto livingTrace = std::make_shared<ClassicDecisionTrace>();
	EXPECT_GT(scoreForPrimary(livingTrace), undeadScore);
}

TEST_F(ClassicBattleAIIntegrationTest, MeleeSimulationRecalculatesSecondStrikeAfterRetaliation)
{
	CStack * crusaders = addStack(BattleSide::ATTACKER, CreatureID(7), BattleHex(77), 40);
	CStack * pikemen = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(78), 120);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>(64, 100));
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	evaluator.chooseAction(crusaders, parameters);

	const std::string key = std::to_string(crusaders->unitId()) + ":" + std::to_string(pikemen->unitId());
	auto traceValue = [&](std::string_view stage) -> int64_t
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[&](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == stage && entry.key == key;
			});
		EXPECT_NE(found, trace->getEntries().end()) << stage;
		return found == trace->getEntries().end() ? 0 : found->value;
	};

	const int64_t first = traceValue("attack_sim.first");
	const int64_t retaliation = traceValue("attack_sim.retaliation");
	const int64_t second = traceValue("attack_sim.second");
	EXPECT_GT(first, 0);
	EXPECT_GT(retaliation, 0);
	EXPECT_GT(second, 0);
	EXPECT_LT(second, first);
	EXPECT_EQ(traceValue("attack_sim.fire_first"), 0);
	EXPECT_EQ(traceValue("attack_sim.fire_retaliation"), 0);
	EXPECT_EQ(traceValue("attack_sim.fire_second"), 0);
}

TEST_F(ClassicBattleAIIntegrationTest, MeleeSimulationAppliesInnateFireShieldBeforeEveryStrike)
{
	CStack * crusaders = addStack(BattleSide::ATTACKER, CreatureID(7), BattleHex(77), 1000);
	CStack * sultans = addStack(BattleSide::DEFENDER, CreatureID(53), BattleHex(78), 1000);
	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>(64, 100));
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	evaluator.chooseAction(crusaders, parameters);

	const std::string key = std::to_string(crusaders->unitId()) + ":" + std::to_string(sultans->unitId());
	auto traceValue = [&](std::string_view stage) -> int64_t
	{
		const auto found = std::ranges::find_if(
			trace->getEntries(),
			[&](const ClassicDecisionTraceEntry & entry)
			{
				return entry.stage == stage && entry.key == key;
			});
		EXPECT_NE(found, trace->getEntries().end()) << stage;
		return found == trace->getEntries().end() ? 0 : found->value;
	};

	EXPECT_GT(traceValue("attack_sim.fire_first"), 0);
	EXPECT_GT(traceValue("attack_sim.fire_second"), 0);
	EXPECT_LT(traceValue("attack_sim.attacker_after"), crusaders->getAvailableHealth());
}

TEST_F(ClassicBattleAISiegeIntegrationTest, CatapultTargetsGateThenFallsBackToWalls)
{
	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const CStack * actor = view.orderedStacks().front();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);

	battle()->si.gateState = EGateState::CLOSED;
	BattleAction action = evaluator.chooseCatapultAction(actor);
	EXPECT_EQ(action.actionType, EActionType::CATAPULT);
	ASSERT_FALSE(action.target.empty());
	EXPECT_EQ(action.target.front().hexValue, callback->wallPartToBattleHex(EWallPart::GATE));

	battle()->si.gateState = EGateState::DESTROYED;
	for(auto & [part, state] : battle()->si.wallState)
		state = EWallState::NONE;
	battle()->si.wallState[EWallPart::KEEP] = EWallState::INTACT;
	action = evaluator.chooseCatapultAction(actor);
	EXPECT_EQ(action.actionType, EActionType::CATAPULT);
	EXPECT_EQ(action.target.front().hexValue, callback->wallPartToBattleHex(EWallPart::KEEP));

	battle()->si.wallState[EWallPart::KEEP] = EWallState::DESTROYED;
	EXPECT_EQ(evaluator.chooseCatapultAction(actor).actionType, EActionType::DEFEND);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, GroundAttackerAdvancesToOriginalOutsideGateContour)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	for(auto & [part, state] : battle()->si.wallState)
		state = EWallState::REINFORCED;
	battle()->si.gateState = EGateState::CLOSED;
	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(86), 97);
	attacker->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE,
		BonusType::STACKS_SPEED,
		BonusSource::TERRAIN_NATIVE,
		1,
		BonusSourceID()));
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(100), 100);

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, rng, trace);
	const ClassicScoredAction result = evaluator.chooseAction(attacker, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().hexValue, BattleHex(91));
	EXPECT_EQ(rng->consumed(), 0u);
	const auto advance = std::ranges::find_if(trace->getEntries(), [](const ClassicDecisionTraceEntry & entry)
	{
		return entry.stage == "siege.advance";
	});
	ASSERT_NE(advance, trace->getEntries().end());
	EXPECT_EQ(advance->value, BattleHex::GATE_BRIDGE);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, MoatDangerChargesDefendingStack)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	addStack(BattleSide::ATTACKER, CreatureID(94), BattleHex(86), 100);
	CStack * defender = addStack(BattleSide::DEFENDER, CreatureID(58), BattleHex(97), 409);
	auto moat = std::make_shared<SpellCreatedObstacle>();
	moat->obstacleType = CObstacleInstance::MOAT;
	moat->uniqueID = 0;
	moat->casterSide = BattleSide::DEFENDER;
	moat->minimalDamage = 70;
	moat->passable = true;
	moat->customSize.insert(defender->getPosition());
	battle()->obstacles.push_back(std::move(moat));
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::DEFENDER, 4);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(
		callback,
		std::make_shared<UpperBoundClassicBattleAIRng>(),
		trace,
		true,
		false);
	evaluator.chooseRunAction(defender, parameters);
	const auto moatDanger = std::ranges::find_if(trace->getEntries(), [](const auto & entry)
	{
		return entry.stage == "run.current";
	});
	ASSERT_NE(moatDanger, trace->getEntries().end());
	const int64_t health = defender->getAvailableHealth();
	const int64_t loss = values.lossValue(
		defender,
		health,
		std::max<int64_t>(0, health - 70),
		parameters,
		false,
		parameters.killsOnly);
	EXPECT_EQ(moatDanger->value, -loss);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, ClosedGateRejectsDefenderPathBeforeMeleeRng)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	for(auto & [part, state] : battle()->si.wallState)
		state = EWallState::REINFORCED;
	battle()->si.gateState = EGateState::CLOSED;
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex::GATE_BRIDGE, 94);
	CStack * defender = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(100), 100);

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::DEFENDER, 1);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	ClassicAttackEvaluator evaluator(callback, rng, nullptr, false, false);
	const ClassicScoredAction result = evaluator.chooseAction(defender, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::DEFEND);
	EXPECT_TRUE(result.action.target.empty());
	EXPECT_EQ(rng->consumed(), 0u);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, RunSelectorPrecedesWaitFallbackForThreatenedGarrison)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	for(auto & [part, state] : battle()->si.wallState)
		state = EWallState::REINFORCED;
	battle()->si.gateState = EGateState::CLOSED;
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex::GATE_BRIDGE, 94);
	CStack * defender = addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(100), 100);
	auto fireWall = std::make_shared<SpellCreatedObstacle>();
	fireWall->ID = SpellID(SpellID::FIRE_WALL).getNum();
	fireWall->pos = defender->getPosition();
	fireWall->uniqueID = 0;
	fireWall->turnsRemaining = 2;
	fireWall->casterSide = BattleSide::ATTACKER;
	fireWall->minimalDamage = 1000;
	fireWall->passable = true;
	fireWall->trigger = SpellID(SpellID::FIRE_WALL);
	fireWall->customSize.insert(defender->getPosition());
	battle()->obstacles.push_back(std::move(fireWall));
	battle()->nodeHasChanged();

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::DEFENDER, 4);
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicAttackEvaluator evaluator(callback, std::make_shared<UpperBoundClassicBattleAIRng>(), trace, true, false);
	const ClassicScoredAction result = evaluator.chooseAction(defender, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::WALK);
	const auto current = std::ranges::find_if(
		trace->getEntries(),
		[](const auto & entry)
		{
			return entry.stage == "run.current";
		}
	);
	ASSERT_NE(current, trace->getEntries().end());
	EXPECT_LT(current->value, 0);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, FailedSiegeIgnoresCatapultAndDefendsInsteadOfWaiting)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	for(auto & [part, state] : battle()->si.wallState)
		state = EWallState::REINFORCED;
	battle()->si.gateState = EGateState::CLOSED;
	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex::GATE_BRIDGE, 94);
	addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), BattleHex(120), 1);
	addStack(BattleSide::DEFENDER, CreatureID(0), BattleHex(100), 100);

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{});
	// With the long-move policy enabled, an ordinary no-action fallback waits.
	// failed_siege is the condition that changes this exact case to DEFEND.
	ClassicAttackEvaluator evaluator(callback, rng, nullptr, true, false);
	const ClassicScoredAction result = evaluator.chooseAction(attacker, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::DEFEND);
	EXPECT_EQ(rng->consumed(), 0u);
}

TEST_F(ClassicBattleAIIntegrationTest, HealingTentSelectsMostWoundedFriendly)
{
	CStack * lightlyWounded = addStack(BattleSide::ATTACKER, CreatureID(3), BattleHex(leftHex), 5);
	CStack * heavilyWounded = addStack(BattleSide::ATTACKER, CreatureID(3), BattleHex(rightHex), 5);
	JsonNode lightState = lightlyWounded->save();
	lightState["state"]["health"]["firstHPleft"].Integer() = lightlyWounded->getMaxHealth() - 1;
	lightlyWounded->load(lightState);
	JsonNode heavyState = heavilyWounded->save();
	heavyState["state"]["health"]["firstHPleft"].Integer() = 1;
	heavilyWounded->load(heavyState);

	auto callback = battleCallback();
	ClassicBattleStateView view(callback);
	const CStack * actor = view.orderedStacks().front();
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);
	const BattleAction action = evaluator.chooseHealingTentAction(actor);
	EXPECT_EQ(action.actionType, EActionType::STACK_HEAL);
	ASSERT_FALSE(action.target.empty());
	EXPECT_EQ(action.target.front().unitValue, heavilyWounded->unitId());
}

TEST_F(ClassicBattleAIIntegrationTest, HealingTentSkipsWoundedWarMachines)
{
	removeAllStacks();
	CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(35), 1);
	CStack * ballista = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::BALLISTA), BattleHex(52), 1);
	CStack * pikemen = addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(69), 10);
	for(CStack * wounded : {tent, ballista, pikemen})
	{
		JsonNode state = wounded->save();
		state["state"]["health"]["firstHPleft"].Integer() = 1;
		wounded->load(state);
	}
	ASSERT_GT(ballista->getMaxHealth(), pikemen->getMaxHealth());
	ClassicAttackEvaluator evaluator(battleCallback(), std::make_shared<UpperBoundClassicBattleAIRng>(), nullptr);
	const BattleAction action = evaluator.chooseHealingTentAction(tent);
	ASSERT_EQ(action.actionType, EActionType::STACK_HEAL);
	ASSERT_FALSE(action.target.empty());
	EXPECT_EQ(action.target.front().unitValue, pikemen->unitId());

	JsonNode healthy = pikemen->save();
	healthy["state"]["health"]["firstHPleft"].Integer() = pikemen->getMaxHealth();
	pikemen->load(healthy);
	EXPECT_EQ(evaluator.chooseHealingTentAction(tent).actionType, EActionType::DEFEND);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, CyclopsPrefersWeakestWallForStrandedArmy)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		if(stack->unitSide() == BattleSide::DEFENDER)
			removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	CStack * cyclops = addStack(BattleSide::ATTACKER, CreatureID(94), BattleHex(35), 1);
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(52), 1000);
	battle()->si.gateState = EGateState::CLOSED;
	battle()->si.wallState[EWallPart::BELOW_GATE] = EWallState::DAMAGED;
	battle()->si.wallState[EWallPart::OVER_GATE] = EWallState::INTACT;
	battle()->si.wallState[EWallPart::BOTTOM_WALL] = EWallState::INTACT;
	battle()->si.wallState[EWallPart::UPPER_WALL] = EWallState::INTACT;

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{1});
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);
	const ClassicScoredAction result = evaluator.chooseAction(cyclops, parameters);
	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::CATAPULT);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().hexValue, callback->wallPartToBattleHex(EWallPart::BELOW_GATE));
	ASSERT_EQ(rng->getRequests().size(), 1u);
	EXPECT_EQ(rng->getRequests().front().lower, 1);
	EXPECT_EQ(rng->getRequests().front().upper, 1);
	EXPECT_EQ(rng->getRequests().front().value, 1);
}

TEST_F(ClassicBattleAISiegeIntegrationTest, CyclopsWallTieUsesOriginalOneBasedOrder)
{
	BattleUnitsChanged removal;
	removal.battleID = BattleID(0);
	for(const CStack * stack : battle()->battleGetAllStacks(false))
		if(stack->unitSide() == BattleSide::DEFENDER)
			removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(removal);

	CStack * cyclops = addStack(BattleSide::ATTACKER, CreatureID(94), BattleHex(35), 1);
	addStack(BattleSide::ATTACKER, CreatureID(0), BattleHex(52), 1000);
	for(EWallPart part : {
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::BOTTOM_WALL,
		EWallPart::UPPER_WALL})
	{
		battle()->si.wallState[part] = EWallState::INTACT;
	}

	auto callback = battleCallback();
	ClassicCombatValue values(callback);
	const auto parameters = values.buildParameters(BattleSide::ATTACKER, 4);
	auto rng = std::make_shared<ReplayClassicBattleAIRng>(std::vector<int32_t>{3});
	ClassicAttackEvaluator evaluator(callback, rng, nullptr);
	const ClassicScoredAction result = evaluator.chooseAction(cyclops, parameters);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.action.actionType, EActionType::CATAPULT);
	ASSERT_FALSE(result.action.target.empty());
	EXPECT_EQ(result.action.target.front().hexValue, callback->wallPartToBattleHex(EWallPart::BOTTOM_WALL));
	ASSERT_EQ(rng->getRequests().size(), 1u);
	EXPECT_EQ(rng->getRequests().front().lower, 1);
	EXPECT_EQ(rng->getRequests().front().upper, 4);
	EXPECT_EQ(rng->getRequests().front().value, 3);
}

TEST_F(ClassicBattleAIIntegrationTest, ControllerCoversLifecycleDecisionAndTacticsDispatch)
{
	RecordingBattleAIClient client;
	auto callback = std::make_shared<CBattleCallback>(PlayerColor(0), &client);
	callback->onBattleStarted(battle());
	auto environment = std::static_pointer_cast<Environment>(gameHandler);
	auto rng = std::make_shared<UpperBoundClassicBattleAIRng>();
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicBattleController controller(environment, callback, PlayerColor(0), rng, trace);
	controller.battleStart(BattleID(0), BattleSide::ATTACKER);

	ClassicBattleStateView view(battleCallback());
	const CStack * attacker = view.orderedStacks().front();
	AutocombatPreferences preferences;
	preferences.enableSpellsUsage = false;
	preferences.enableTacticsUsage = false;
	const BattleAction action = controller.decideStackAction(BattleID(0), attacker, preferences);
	EXPECT_NE(action.actionType, EActionType::NO_ACTION);
	controller.activeStack(BattleID(0), attacker, preferences);
	controller.yourTacticPhase(BattleID(0), 0, preferences);

	preferences.enableTacticsUsage = true;
	battle()->tacticDistance = 2;
	battle()->tacticsSide = BattleSide::ATTACKER;
	controller.yourTacticPhase(BattleID(0), 2, preferences);
	controller.actionFinished(BattleID(1), BattleAction::makeDefend(attacker));
	controller.actionFinished(BattleID(0), BattleAction::makeMove(attacker, attacker->getPosition()));
	EXPECT_GT(client.requests, 0);

	controller.battleEnd(BattleID(0));
	callback->onBattleEnded(BattleID(0));
}
