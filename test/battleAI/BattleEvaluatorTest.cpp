/*
 * BattleEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../server/battles/BattleTestFixture.h"

#include "AI/BattleAI/BattleEvaluator.h"
#include "lib/GameLibrary.h"
#include "lib/ObstacleHandler.h"
#include "lib/battle/BattleAction.h"
#include "lib/battle/BattleUnitTurnReason.h"
#include "lib/battle/CObstacleInstance.h"
#include "lib/battle/CPlayerBattleCallback.h"
#include "lib/callback/CBattleCallback.h"
#include "lib/callback/GameRandomizer.h"
#include "lib/bonuses/BonusCustomTypes.h"
#include "lib/spells/CSpell.h"
#include "lib/spells/ISpellMechanics.h"
#include "server/CGameHandler.h"
#include "server/battles/BattleProcessor.h"

namespace test
{
class BattleEvaluatorTest : public BattleTestFixture
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->stacks.clear();
	}

	static void prepareDamageComparison(std::initializer_list<CStack *> units)
	{
		for(auto * unit : units)
		{
			unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
			blockRetaliation(unit);
			forceMaximumDamage(unit);
		}
	}

	/// Snapshots the battle as BattleAI of the defending player sees it
	void evaluateAsDefender()
	{
		callback = std::make_shared<CBattleCallback>(defenderSideHero->getOwner(), nullptr);
		callback->onBattleStarted(battle());
		hypotheticalBattle = std::make_shared<HypotheticBattle>(gameHandler.get(), callback->getBattle(BattleID(0)));
		damageCache.buildDamageCache(hypotheticalBattle, BattleSide::DEFENDER);
	}

	BattleAction moveTowards(
		const CStack * stack,
		const BattleHexArray & movementTargets,
		const BattleHexArray & finalDestinationHexes)
	{
		evaluateAsDefender();
		PotentialTargets targets(stack, damageCache, hypotheticalBattle);
		BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, stack,
			defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);

		return evaluator.goTowardsNearest(stack, movementTargets, targets, finalDestinationHexes);
	}

	BattleAction moveTowards(const CStack * stack, const CStack * target)
	{
		auto primaryTargetHexes = target->getAttackableHexes(stack);
		return moveTowards(stack, primaryTargetHexes, primaryTargetHexes);
	}

	AttackPossibility evaluateMelee(const CStack * attacker, const CStack * defender, BattleHex from)
	{
		evaluateAsDefender();
		return AttackPossibility::evaluate(BattleAttackInfo(attacker, defender, 0, false), from, damageCache, hypotheticalBattle);
	}

	CStack * addPetrifyingBasilisks()
	{
		auto * basilisks = addStack(BattleSide::ATTACKER, creatureByName("core:basilisk"), BattleHex(8, 5), 400);
		basilisks->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_AFTER_ATTACK,
			BonusSource::OTHER, 80, BonusSourceID(), BonusSubtypeID(SpellID(SpellID::STONE_GAZE))));
		return basilisks;
	}


	void checkMoraleShotProtectsAlly(const std::string & creature, int enemyCount, int randomSeed)
	{
		auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 5), enemyCount);
		auto * ally = addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(2, 5), enemyCount * 2);
		auto * shooter = addStack(BattleSide::DEFENDER, creatureByName(creature), BattleHex(15, 5), 50);
		blockRetaliation(enemy);
		shooter->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OTHER, 3, BonusSourceID()));
		gameHandler->randomizer->setSeed(randomSeed);
		beginCombat();
		ASSERT_EQ(battle()->battleActiveUnit(), shooter);
		ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()),
			BattleAction::makeShotAttack(shooter, enemy)));
		ASSERT_TRUE(shooter->hadMorale);
		ASSERT_EQ(battle()->battleActiveUnit(), shooter);
		ASSERT_TRUE(battle()->battleHasShootingPenalty(shooter, enemy->getPosition()));
		const auto enemyHealthBefore = enemy->getAvailableHealth();
		const auto allyHealthBefore = ally->getAvailableHealth();

		evaluateAsDefender();
		BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, shooter,
			defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
		const auto action = evaluator.selectStackAction(shooter);
		ASSERT_EQ(action.actionType, EActionType::SHOOT) << "morale activation discarded a profitable shot";
		ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()), action));
		ASSERT_TRUE(enemy->alive());
		ASSERT_LT(enemy->getAvailableHealth(), enemyHealthBefore);
		ASSERT_EQ(battle()->battleActiveUnit(), enemy);
		ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(enemy->unitSide()),
			BattleAction::makeMeleeAttack(enemy, ally, enemy->getPosition())));
		EXPECT_LT(allyHealthBefore - ally->getAvailableHealth(), enemyHealthBefore);
	}

	void addObstacle()
	{
		const auto * obstacleInfo = LIBRARY->obstacles()->getByName("core:12");
		ASSERT_NE(obstacleInfo, nullptr);

		auto obstacle = std::make_shared<CObstacleInstance>();
		obstacle->ID = obstacleInfo->obstacle.getNum(); // ObDtS03: dead tree and rocks
		obstacle->pos = BattleHex(9, 6);
		battle()->obstacles.push_back(obstacle);
	}

	std::shared_ptr<CBattleCallback> callback;
	std::shared_ptr<HypotheticBattle> hypotheticalBattle;
	DamageCache damageCache;
};

TEST_F(BattleEvaluatorTest, HypnotizedUnitGoesForItsFormerAllies)
{
	auto * elf = addStack(BattleSide::DEFENDER, creatureByName("core:woodElf"), BattleHex(14, 8), 30);
	addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(9, 5), 5);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(10, 5), 3);
	hypnotize(behemoth);

	// BattleAI of the attacking player controls the hypnotized behemoth
	callback = std::make_shared<CBattleCallback>(attackerSideHero->getOwner(), nullptr);
	callback->onBattleStarted(battle());
	hypotheticalBattle = std::make_shared<HypotheticBattle>(gameHandler.get(), callback->getBattle(BattleID(0)));
	damageCache.buildDamageCache(hypotheticalBattle, BattleSide::ATTACKER);
	BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, behemoth,
		attackerSideHero->getOwner(), BattleID(0), BattleSide::ATTACKER, 1.0f, 2);

	BattleAction action = evaluator.selectStackAction(behemoth);

	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK);
	EXPECT_EQ(action.target[1].hexValue, elf->getPosition());
}

TEST_F(BattleEvaluatorTest, MarksmanMoraleShotProtectsAlly)
{
	checkMoraleShotProtectsAlly("core:marksman", 500, 9);
}

TEST_F(BattleEvaluatorTest, MonkMoraleShotProtectsAlly)
{
	checkMoraleShotProtectsAlly("core:monk", 2000, 24);
}

TEST_F(BattleEvaluatorTest, WaitingShooterCanShootAfterEnemyAdvances)
{
	auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 5), 500);
	auto * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(15, 5), 50);
	enemy->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	evaluateAsDefender();
	BattleEvaluator before(gameHandler, callback, hypotheticalBattle, damageCache, shooter,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto wait = before.selectStackAction(shooter);
	ASSERT_EQ(wait.actionType, EActionType::WAIT);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()), wait));
	ASSERT_EQ(battle()->battleActiveUnit(), enemy);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(enemy->unitSide()),
		BattleAction::makeMove(enemy, BattleHex(4, 5))));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_TRUE(shooter->waited());
	damageCache = DamageCache();
	evaluateAsDefender();
	BattleEvaluator after(gameHandler, callback, hypotheticalBattle, damageCache, shooter,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto shot = after.selectStackAction(shooter);
	ASSERT_EQ(shot.actionType, EActionType::SHOOT);
	const auto health = enemy->getAvailableHealth();
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()), shot));
	EXPECT_LT(enemy->getAvailableHealth(), health);
}

TEST_F(BattleEvaluatorTest, WaitedAttackAccountsForCasualties)
{
	auto * zombies = addStack(BattleSide::ATTACKER, creatureByName("core:zombie"), BattleHex(7, 5), 40);
	auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 40);
	prepareDamageComparison({zombies, swordsmen});
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), swordsmen);
	evaluateAsDefender();
	PotentialTargets targets(swordsmen, damageCache, hypotheticalBattle);
	const auto attack = AttackPossibility::evaluate(BattleAttackInfo(swordsmen, zombies, 0, false), swordsmen->getPosition(), damageCache, hypotheticalBattle);
	auto waited = std::make_shared<HypotheticBattle>(gameHandler.get(), hypotheticalBattle);
	waited->makeWait(swordsmen);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(waited);
	const auto score = evaluator.evaluateExchange(attack, 0, targets, damageCache, waited);
	const auto initialSwordsmen = swordsmen->acquireState();
	const auto initialZombies = zombies->acquireState();
	ASSERT_TRUE(act(BattleAction::makeWait(swordsmen)));
	ASSERT_EQ(battle()->battleActiveUnit(), zombies);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(zombies, swordsmen, zombies->getPosition())));
	ASSERT_TRUE(swordsmen->alive());
	ASSERT_LT(swordsmen->getCount(), initialSwordsmen->getCount());
	ASSERT_EQ(battle()->battleActiveUnit(), swordsmen);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(swordsmen, zombies, swordsmen->getPosition())));
	const auto dealt = initialZombies->getAvailableHealth() - zombies->getAvailableHealth();
	const auto received = initialSwordsmen->getAvailableHealth() - swordsmen->getAvailableHealth();
	const auto expected = AttackPossibility::calculateDamageReduce(initialZombies.get(), dealt, damageCache)
		- AttackPossibility::calculateDamageReduce(initialSwordsmen.get(), received, damageCache);
	EXPECT_NEAR(score * 2, expected, 0.01f);
}

TEST_F(BattleEvaluatorTest, WaitedShooterAttacksBlockerInsteadOfDistantTarget)
{
	auto * target = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 5), 1000);
	auto * blocker = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(10, 5), 1000);
	auto * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(14, 5), 100);
	blocker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACKS_SPEED, BonusSource::OTHER, 1, BonusSourceID()));
	for(auto * unit : {target, blocker, shooter})
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	blockRetaliation(shooter);
	forceMaximumDamage(shooter);
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	evaluateAsDefender();
	const auto shot = AttackPossibility::evaluate(BattleAttackInfo(shooter, target, 0, true), shooter->getPosition(), damageCache, hypotheticalBattle);
	ASSERT_TRUE(act(BattleAction::makeWait(shooter)));
	ASSERT_EQ(battle()->battleActiveUnit(), blocker);
	ASSERT_TRUE(act(BattleAction::makeMove(blocker, BattleHex(13, 5))));
	ASSERT_EQ(battle()->battleActiveUnit(), target);
	ASSERT_TRUE(act(BattleAction::makeDefend(target)));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	evaluateAsDefender();
	PotentialTargets targets(shooter, damageCache, hypotheticalBattle);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(hypotheticalBattle);
	const auto score = evaluator.evaluateExchange(shot, 0, targets, damageCache, hypotheticalBattle);
	const auto original = blocker->acquireState();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(shooter, blocker, shooter->getPosition())));
	const auto dealt = original->getAvailableHealth() - blocker->getAvailableHealth();
	ASSERT_GT(dealt, 0);
	EXPECT_NEAR(score * 2, AttackPossibility::calculateDamageReduce(original.get(), dealt, damageCache), 0.01f);
}

TEST_F(BattleEvaluatorTest, WaitedShooterUsesMeleeDamageAfterBlockingAttack)
{
	auto * blocker = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(11, 5), 300);
	auto * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(14, 5), 30);
	shooter->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::OTHER, 20, BonusSourceID()));
	prepareDamageComparison({blocker, shooter});
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	evaluateAsDefender();
	PotentialTargets targets(shooter, damageCache, hypotheticalBattle);
	const auto shot = AttackPossibility::evaluate(BattleAttackInfo(shooter, blocker, 0, true), shooter->getPosition(), damageCache, hypotheticalBattle);
	auto waited = std::make_shared<HypotheticBattle>(gameHandler.get(), hypotheticalBattle);
	waited->makeWait(shooter);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 2);
	evaluator.updateReachabilityMap(waited);
	const auto score = evaluator.evaluateExchange(shot, 0, targets, damageCache, waited);
	const auto originalShooter = shooter->acquireState();
	const auto originalBlocker = blocker->acquireState();
	ASSERT_TRUE(act(BattleAction::makeWait(shooter)));
	ASSERT_EQ(battle()->battleActiveUnit(), blocker);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(blocker, shooter, BattleHex(13, 5))));
	ASSERT_TRUE(shooter->alive());
	ASSERT_TRUE(blocker->alive());
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(shooter, blocker, shooter->getPosition())));
	const auto dealt = originalBlocker->getAvailableHealth() - blocker->getAvailableHealth();
	const auto received = originalShooter->getAvailableHealth() - shooter->getAvailableHealth();
	ASSERT_GT(dealt, 0);
	float expected = AttackPossibility::calculateDamageReduce(originalBlocker.get(), dealt, damageCache)
		- AttackPossibility::calculateDamageReduce(originalShooter.get(), received, damageCache);
	ASSERT_TRUE(blocker->alive());
	ASSERT_TRUE(shooter->alive());
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	const auto secondBlocker = blocker->acquireState();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(shooter, blocker, shooter->getPosition())));
	ASSERT_TRUE(blocker->alive());
	expected += AttackPossibility::calculateDamageReduce(secondBlocker.get(), secondBlocker->getAvailableHealth() - blocker->getAvailableHealth(), damageCache);
	ASSERT_EQ(battle()->battleActiveUnit(), blocker);
	const auto secondShooter = shooter->acquireState();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(blocker, shooter, blocker->getPosition())));
	ASSERT_TRUE(shooter->alive());
	expected -= AttackPossibility::calculateDamageReduce(secondShooter.get(), secondShooter->getAvailableHealth() - shooter->getAvailableHealth(), damageCache);
	EXPECT_NEAR(score, expected, 0.01f);
}

TEST_F(BattleEvaluatorTest, WaitingShooterRetargetsAfterAllyKillsOriginalTarget)
{
	auto * target = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(5, 5), 1);
	auto * survivor = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 5), 500);
	auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(4, 5), 100);
	auto * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(15, 5), 30);
	prepareDamageComparison({target, survivor, swordsmen, shooter});
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	evaluateAsDefender();
	PotentialTargets targets(shooter, damageCache, hypotheticalBattle);
	const auto shot = AttackPossibility::evaluate(BattleAttackInfo(shooter, target, 0, true), shooter->getPosition(), damageCache, hypotheticalBattle);
	auto waited = std::make_shared<HypotheticBattle>(gameHandler.get(), hypotheticalBattle);
	waited->makeWait(shooter);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(waited);
	const auto score = evaluator.evaluateExchange(shot, 0, targets, damageCache, waited);
	const auto initialTarget = target->acquireState();
	const auto initialSurvivor = survivor->acquireState();
	const auto initialSwordsmen = swordsmen->acquireState();
	ASSERT_TRUE(act(BattleAction::makeWait(shooter)));
	ASSERT_EQ(battle()->battleActiveUnit(), swordsmen);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(swordsmen, target, swordsmen->getPosition())));
	ASSERT_FALSE(target->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), survivor);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(survivor, swordsmen, BattleHex(3, 5))));
	ASSERT_TRUE(swordsmen->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_TRUE(battle()->battleCanShoot(shooter, survivor->getPosition()));
	ASSERT_GT(battle()->getReachability(shooter).distToNearestNeighbour(shooter, survivor), shooter->getMovementRange());
	ASSERT_TRUE(act(BattleAction::makeShotAttack(shooter, survivor)));
	ASSERT_TRUE(survivor->alive());
	const auto expected = AttackPossibility::calculateDamageReduce(initialTarget.get(), initialTarget->getAvailableHealth(), damageCache)
		+ AttackPossibility::calculateDamageReduce(initialSurvivor.get(), initialSurvivor->getAvailableHealth() - survivor->getAvailableHealth(), damageCache)
		- AttackPossibility::calculateDamageReduce(initialSwordsmen.get(), initialSwordsmen->getAvailableHealth() - swordsmen->getAvailableHealth(), damageCache);
	EXPECT_NEAR(score * 2, expected, 0.01f);
}

TEST_F(BattleEvaluatorTest, BlockedWaitingShooterRetargetsReachableEnemyAfterTargetDies)
{
	auto * target = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(5, 5), 1);
	auto * blocker = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(12, 5), 1);
	auto * distant = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 5), 500);
	auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(4, 5), 100);
	auto * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:marksman"), BattleHex(15, 5), 30);
	blocker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACKS_SPEED, BonusSource::OTHER, 1, BonusSourceID()));
	prepareDamageComparison({target, blocker, distant, swordsmen, shooter});
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	evaluateAsDefender();
	const auto shot = AttackPossibility::evaluate(BattleAttackInfo(shooter, target, 0, true), shooter->getPosition(), damageCache, hypotheticalBattle);
	ASSERT_TRUE(act(BattleAction::makeWait(shooter)));
	ASSERT_EQ(battle()->battleActiveUnit(), swordsmen);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(swordsmen, target, swordsmen->getPosition())));
	ASSERT_FALSE(target->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), blocker);
	ASSERT_TRUE(act(BattleAction::makeMove(blocker, BattleHex(14, 5))));
	ASSERT_EQ(battle()->battleActiveUnit(), distant);
	ASSERT_TRUE(act(BattleAction::makeDefend(distant)));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	ASSERT_TRUE(battle()->isMeleeAttackPossible(shooter, blocker, shooter->getPosition()));
	ASSERT_GT(battle()->getReachability(shooter).distToNearestNeighbour(shooter, distant), shooter->getMovementRange());
	auto state = std::make_shared<HypotheticBattle>(gameHandler.get(), callback->getBattle(BattleID(0)));
	PotentialTargets targets(shooter, damageCache, state);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(state);
	const auto score = evaluator.evaluateExchange(shot, 0, targets, damageCache, state);
	const auto originalBlocker = blocker->acquireState();
	const auto distantHealth = distant->getAvailableHealth();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(shooter, blocker, shooter->getPosition())));
	ASSERT_FALSE(blocker->alive());
	EXPECT_EQ(distant->getAvailableHealth(), distantHealth);
	EXPECT_NEAR(score * 2, AttackPossibility::calculateDamageReduce(originalBlocker.get(), originalBlocker->getAvailableHealth(), damageCache), 0.01f);
}

TEST_F(BattleEvaluatorTest, RestrictedFreeShooterUsesMeleeAgainstAdjacentTarget)
{
	auto * archers = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(7, 5), 100);
	auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 20);
	archers->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FREE_SHOOTING, BonusSource::OTHER,
		0, BonusSourceID(), BonusSubtypeID(BonusCustomSubtype::freeShootingExceptAdjacent)));
	prepareDamageComparison({archers, swordsmen});
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), swordsmen);
	ASSERT_TRUE(battle()->battleCanShoot(archers));
	ASSERT_FALSE(battle()->battleCanShoot(archers, swordsmen->getPosition()));
	evaluateAsDefender();
	PotentialTargets targets(swordsmen, damageCache, hypotheticalBattle);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(hypotheticalBattle);
	const auto attack = AttackPossibility::evaluate(BattleAttackInfo(swordsmen, archers, 0, false), swordsmen->getPosition(), damageCache, hypotheticalBattle);
	const auto score = evaluator.evaluateExchange(attack, 0, targets, damageCache, hypotheticalBattle);
	const auto originalArchers = archers->acquireState();
	const auto originalSwordsmen = swordsmen->acquireState();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(swordsmen, archers, swordsmen->getPosition())));
	ASSERT_TRUE(archers->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), archers);
	ASSERT_FALSE(battle()->battleCanShoot(archers, swordsmen->getPosition()));
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(archers, swordsmen, archers->getPosition())));
	ASSERT_TRUE(swordsmen->alive());
	const auto expected = AttackPossibility::calculateDamageReduce(originalArchers.get(), originalArchers->getAvailableHealth() - archers->getAvailableHealth(), damageCache)
		- AttackPossibility::calculateDamageReduce(originalSwordsmen.get(), originalSwordsmen->getAvailableHealth() - swordsmen->getAvailableHealth(), damageCache)
		+ attack.shootersBlockedDmg;
	EXPECT_NEAR(score * 2, expected, 0.01f);
}

TEST_F(BattleEvaluatorTest, MoraleAttackCanSwitchToSurvivingEnemy)
{
	const auto * first = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(7, 5), 1);
	const auto * second = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(8, 4), 1);
	auto * active = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 20);
	evaluateAsDefender();
	BattleEvaluator before(gameHandler, callback, hypotheticalBattle, damageCache, active,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto initial = before.selectStackAction(active);
	ASSERT_EQ(initial.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_TRUE(act(initial));
	ASSERT_NE(first->alive(), second->alive());
	const auto * survivor = first->alive() ? first : second;
	active->afterGetsTurn(BattleUnitTurnReason::MORALE);
	damageCache = DamageCache();
	evaluateAsDefender();
	BattleEvaluator after(gameHandler, callback, hypotheticalBattle, damageCache, active,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto next = after.selectStackAction(active);
	ASSERT_EQ(next.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_EQ(next.target.size(), 2);
	EXPECT_EQ(next.target[1].hexValue, survivor->getPosition());
}

TEST_F(BattleEvaluatorTest, ReturningHarpyLeavesShooterUnblocked)
{
	const auto * shooter = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(7, 5), 50);
	const auto * fodder = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(8, 4), 1);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(12, 5), 10);
	const BattleHex attackFrom(8, 5);
	const auto attack = evaluateMelee(harpies, fodder, attackFrom);
	const auto predictedPosition = attack.attackerState->getPosition();

	ASSERT_TRUE(this->attackFrom(harpies, fodder->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	ASSERT_TRUE(battle()->battleCanShoot(shooter));
	EXPECT_EQ(predictedPosition, harpies->getPosition()) << "returning Harpy prediction differs from server position";
	EXPECT_EQ(attack.shootersBlockedDmg, 0) << "returning Harpy cannot block the shooter from its attack hex";
}

TEST_F(BattleEvaluatorTest, RetaliationBindPreventsHarpyReturnInExchange)
{
	const auto * dendroids = addStack(BattleSide::ATTACKER, creatureByName("core:dendroidGuard"), BattleHex(8, 5), 100);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 1000);
	const BattleHex attackFrom(7, 5);
	const auto attack = evaluateMelee(harpies, dendroids, attackFrom);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	const auto * predicted = hypotheticalBattle->battleGetUnitByID(harpies->unitId());
	EXPECT_TRUE(predicted->hasBonusOfType(BonusType::BIND_EFFECT));

	ASSERT_TRUE(this->attackFrom(harpies, dendroids->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	ASSERT_TRUE(dendroids->alive());
	ASSERT_TRUE(harpies->hasBonusOfType(BonusType::BIND_EFFECT));
	ASSERT_EQ(harpies->getPosition(), attackFrom);
	EXPECT_EQ(attack.attackerState->getPosition(), harpies->getPosition());
	EXPECT_EQ(predicted->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, HarpyReturnsWhenBindRetaliationIsBlocked)
{
	const auto * dendroids = addStack(BattleSide::ATTACKER, creatureByName("core:dendroidGuard"), BattleHex(8, 5), 100);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 1000);
	blockRetaliation(harpies);
	const BattleHex start = harpies->getPosition();
	const BattleHex attackFrom(7, 5);
	const auto attack = evaluateMelee(harpies, dendroids, attackFrom);

	ASSERT_TRUE(this->attackFrom(harpies, dendroids->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	EXPECT_FALSE(harpies->hasBonusOfType(BonusType::BIND_EFFECT));
	ASSERT_EQ(harpies->getPosition(), start);
	EXPECT_EQ(attack.attackerState->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, DestroyedCloneCannotBindReturningHarpy)
{
	addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 8), 1);
	auto * dendroids = addStack(BattleSide::ATTACKER, creatureByName("core:dendroidGuard"), BattleHex(8, 5), 1000);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 10);
	makeClone(dendroids);
	const auto health = dendroids->getAvailableHealth();
	const BattleHex start = harpies->getPosition();
	const BattleHex from(7, 5);
	const auto attack = evaluateMelee(harpies, dendroids, from);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	EXPECT_FALSE(hypotheticalBattle->battleGetUnitByID(harpies->unitId())->hasBonusOfType(BonusType::BIND_EFFECT));
	const auto damage = battle()->battleEstimateDamage(BattleAttackInfo(harpies, dendroids, 0, false)).damage;
	ASSERT_GT(damage.min, 0);
	ASSERT_LT(damage.max, health);
	ASSERT_TRUE(this->attackFrom(harpies, dendroids->getPosition(), from));
	ASSERT_FALSE(dendroids->alive());
	ASSERT_FALSE(harpies->hasBonusOfType(BonusType::BIND_EFFECT));
	ASSERT_EQ(harpies->getPosition(), start);
	EXPECT_EQ(attack.attackerState->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, RetaliationPetrificationPreventsHarpyReturnInExchange)
{
	auto * basilisks = addPetrifyingBasilisks();
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(12, 5), 1000);
	const BattleHex attackFrom(9, 5);
	const auto attack = evaluateMelee(harpies, basilisks, attackFrom);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	const auto * predicted = hypotheticalBattle->battleGetUnitByID(harpies->unitId());
	EXPECT_TRUE(predicted->hasBonusOfType(BonusType::NOT_ACTIVE));

	ASSERT_TRUE(this->attackFrom(harpies, basilisks->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	ASSERT_TRUE(basilisks->alive());
	ASSERT_TRUE(harpies->hasBonusOfType(BonusType::NOT_ACTIVE));
	ASSERT_EQ(harpies->getPosition(), attackFrom);
	EXPECT_EQ(attack.attackerState->getPosition(), harpies->getPosition());
	EXPECT_EQ(predicted->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, PetrifiedHarpyDoesNotAttackOnNextTurn)
{
	const auto * reserve = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 8), 1);
	auto * basilisks = addPetrifyingBasilisks();
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(12, 5), 500);
	harpies->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::OTHER, 100, BonusSourceID()));
	for(auto * unit : {basilisks, harpies})
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	ASSERT_TRUE(act(BattleAction::makeWait(harpies)));
	ASSERT_EQ(battle()->battleActiveUnit(), basilisks);
	ASSERT_TRUE(act(BattleAction::makeDefend(basilisks)));
	ASSERT_EQ(battle()->battleActiveUnit(), reserve);
	ASSERT_TRUE(act(BattleAction::makeDefend(reserve)));
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	evaluateAsDefender();
	hypotheticalBattle->removeUnit(reserve->unitId());
	PotentialTargets targets(harpies, damageCache, hypotheticalBattle);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 2);
	evaluator.updateReachabilityMap(hypotheticalBattle);
	const BattleHex from(9, 5);
	const auto attack = AttackPossibility::evaluate(BattleAttackInfo(harpies, basilisks, 0, false), from, damageCache, hypotheticalBattle);
	const auto score = evaluator.evaluateExchange(attack, 0, targets, damageCache, hypotheticalBattle);
	auto expectedState = std::make_shared<HypotheticBattle>(gameHandler.get(), hypotheticalBattle);
	BattleExchangeVariant expected;
	expected.trackAttack(attack, expectedState, damageCache);
	expectedState->nextRound();
	expected.trackAttack(expectedState->getForUpdate(basilisks->unitId()), expectedState->getForUpdate(harpies->unitId()), false, false,
		damageCache, expectedState, BattleExchangeVariant::AttackMode::FIRST_STRIKE);
	EXPECT_FALSE(expectedState->battleGetUnitByID(harpies->unitId())->hasBonusOfType(BonusType::NOT_ACTIVE));
	ASSERT_TRUE(expectedState->battleGetUnitByID(basilisks->unitId())->alive());
	expected.trackAttack(expectedState->getForUpdate(harpies->unitId()), expectedState->getForUpdate(basilisks->unitId()), false, true,
		damageCache, expectedState, BattleExchangeVariant::AttackMode::FIRST_STRIKE);
	EXPECT_NEAR(score, expected.getScore().enemyDamageReduce - expected.getScore().ourDamageReduce, 0.01f);
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(harpies, basilisks->getPosition(), from)));
	ASSERT_TRUE(harpies->hasBonusOfType(BonusType::NOT_ACTIVE));
	ASSERT_EQ(battle()->battleActiveUnit(), basilisks);
	basilisks->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_AFTER_ATTACK,
		BonusSource::OTHER, -100, BonusSourceID(), BonusSubtypeID(SpellID(SpellID::STONE_GAZE))));
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(basilisks, harpies, basilisks->getPosition())));
	ASSERT_FALSE(harpies->hasBonusOfType(BonusType::NOT_ACTIVE));
	ASSERT_TRUE(basilisks->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	const auto health = basilisks->getAvailableHealth();
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(harpies, basilisks, harpies->getPosition())));
	EXPECT_LT(basilisks->getAvailableHealth(), health);
}

TEST_F(BattleEvaluatorTest, RetaliationSlowRemovesHasteInExchange)
{
	auto * swordsmen = addStack(BattleSide::ATTACKER, creatureByName("core:swordsman"), BattleHex(8, 5), 1000);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 1000);
	swordsmen->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_AFTER_ATTACK,
		BonusSource::OTHER, 100, BonusSourceID(), BonusSubtypeID(SpellID(SpellID::SLOW))));
	const int baseSpeed = harpies->getMovementRange();
	teachSpell(defenderSideHero, SpellID::HASTE);
	ASSERT_TRUE(castOn(defenderSideHero, SpellID::HASTE, harpies));
	ASSERT_GT(harpies->getMovementRange(), baseSpeed);
	const BattleHex attackFrom(7, 5);
	const auto attack = evaluateMelee(harpies, swordsmen, attackFrom);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	const auto * predicted = hypotheticalBattle->battleGetUnitByID(harpies->unitId());
	const int predictedSpeed = predicted->getMovementRange();

	ASSERT_TRUE(this->attackFrom(harpies, swordsmen->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	ASSERT_TRUE(swordsmen->alive());
	ASSERT_LT(harpies->getMovementRange(), baseSpeed);
	EXPECT_EQ(predictedSpeed, harpies->getMovementRange());
	EXPECT_EQ(attack.attackerState->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, RetaliationDispelsHypotheticalHasteBeforeReturn)
{
	const auto * flies = addStack(BattleSide::ATTACKER, creatureByName("core:serpentFly"), BattleHex(12, 5), 1000);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 1000);
	teachSpell(defenderSideHero, SpellID::HASTE);
	evaluateAsDefender();
	spells::BattleCast cast(hypotheticalBattle.get(), defenderSideHero, spells::Mode::HERO, SpellID(SpellID::HASTE).toSpell());
	cast.castEval(hypotheticalBattle->getServerCallback(), spells::Target{spells::Destination(hypotheticalBattle->battleGetUnitByID(harpies->unitId()))});
	const auto * hasted = hypotheticalBattle->battleGetUnitByID(harpies->unitId());
	ASSERT_GT(hasted->getMovementRange(), harpies->getMovementRange());
	const BattleHex from(11, 5);
	const auto attack = AttackPossibility::evaluate(BattleAttackInfo(hasted, flies, 0, false), from, damageCache, hypotheticalBattle);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	const auto * predicted = hypotheticalBattle->battleGetUnitByID(harpies->unitId());
	ASSERT_TRUE(castOn(defenderSideHero, SpellID::HASTE, harpies));
	ASSERT_TRUE(this->attackFrom(harpies, flies->getPosition(), from));
	ASSERT_TRUE(harpies->alive());
	ASSERT_TRUE(flies->alive());
	EXPECT_EQ(predicted->getMovementRange(), harpies->getMovementRange());
	EXPECT_EQ(predicted->getPosition(), harpies->getPosition());
}

TEST_F(BattleEvaluatorTest, NestedBattleCanRemoveExtendedSpell)
{
	addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(12, 5), 100);
	const auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 10);
	teachSpell(defenderSideHero, SpellID::HASTE);
	ASSERT_TRUE(castOn(defenderSideHero, SpellID::HASTE, harpies));
	evaluateAsDefender();
	const auto haste = harpies->getBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::HASTE))));
	ASSERT_NE(haste, nullptr);
	Bonus extended = *haste;
	++extended.turnsRemain;
	hypotheticalBattle->updateUnitBonus(harpies->unitId(), {extended});
	auto child = std::make_shared<HypotheticBattle>(gameHandler.get(), hypotheticalBattle);
	child->removeUnitBonus(harpies->unitId(), {extended});
	EXPECT_FALSE(child->battleGetUnitByID(harpies->unitId())->hasBonus(Selector::sourceType()(BonusSource::SPELL_EFFECT)));
}

TEST_F(BattleEvaluatorTest, HypotheticalRoundsExpireTimedEffects)
{
	addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(12, 5), 100);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpy"), BattleHex(3, 5), 10);
	auto effect = std::make_shared<Bonus>(BonusDuration::N_TURNS, BonusType::NOT_ACTIVE, BonusSource::OTHER, 0, BonusSourceID());
	effect->turnsRemain = 2;
	harpies->addNewBonus(effect);
	beginCombat();
	evaluateAsDefender();
	for(int round = 0; round < 2; ++round)
	{
		hypotheticalBattle->nextRound();
		battle()->nextRound();
		EXPECT_EQ(hypotheticalBattle->battleGetUnitByID(harpies->unitId())->canMove(), harpies->canMove());
	}
	EXPECT_TRUE(harpies->canMove());
}

TEST_F(BattleEvaluatorTest, ReturningHarpyWithLostSpeedUsesServerDestination)
{
	addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(1, 8), 10);
	const auto * fodder = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(5, 5), 1);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(14, 5), 10);
	harpies->addNewBonus(std::make_shared<Bonus>(BonusDuration::UNTIL_OWN_ATTACK, BonusType::STACKS_SPEED, BonusSource::OTHER, 2, BonusSourceID()));
	const BattleHex start = harpies->getPosition();
	const BattleHex attackFrom(6, 5);
	const auto attack = evaluateMelee(harpies, fodder, attackFrom);
	const auto predictedPosition = attack.attackerState->getPosition();

	ASSERT_TRUE(this->attackFrom(harpies, fodder->getPosition(), attackFrom));
	ASSERT_TRUE(harpies->alive());
	ASSERT_NE(harpies->getPosition(), start);
	EXPECT_EQ(predictedPosition, harpies->getPosition()) << "speed loss should shorten the predicted return";
}

TEST_F(BattleEvaluatorTest, ReturningGroundUnitWithLostSpeedStopsPartway)
{
	addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(1, 8), 10);
	const auto * fodder = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(5, 5), 1);
	auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(10, 5), 10);
	swordsmen->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::RETURN_AFTER_STRIKE, BonusSource::OTHER, 0, BonusSourceID()));
	swordsmen->addNewBonus(std::make_shared<Bonus>(BonusDuration::UNTIL_OWN_ATTACK, BonusType::STACKS_SPEED, BonusSource::OTHER, 2, BonusSourceID()));
	const BattleHex start = swordsmen->getPosition();
	const BattleHex attackFrom(6, 5);
	const auto attack = evaluateMelee(swordsmen, fodder, attackFrom);
	const auto predictedPosition = attack.attackerState->getPosition();

	ASSERT_TRUE(this->attackFrom(swordsmen, fodder->getPosition(), attackFrom));
	ASSERT_TRUE(swordsmen->alive());
	ASSERT_NE(swordsmen->getPosition(), start);
	ASSERT_NE(swordsmen->getPosition(), attackFrom);
	EXPECT_EQ(predictedPosition, swordsmen->getPosition());
}

TEST_F(BattleEvaluatorTest, MeleeAttackReleasesShooterBlockInExchange)
{
	const auto * shooter = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(7, 5), 50);
	const auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(8, 4), 100);
	const auto * swordsmen = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 10);
	const BattleHex attackFrom(9, 4);
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	evaluateAsDefender();
	const auto attack = AttackPossibility::evaluate(BattleAttackInfo(swordsmen, enemy, 0, false), attackFrom, damageCache, hypotheticalBattle);
	BattleExchangeVariant exchange;
	exchange.trackAttack(attack, hypotheticalBattle, damageCache);
	const auto * predictedSwordsmen = hypotheticalBattle->battleGetUnitByID(swordsmen->unitId());
	const auto * predictedShooter = hypotheticalBattle->battleGetUnitByID(shooter->unitId());

	ASSERT_TRUE(this->attackFrom(swordsmen, enemy->getPosition(), attackFrom));
	ASSERT_TRUE(swordsmen->alive());
	ASSERT_TRUE(battle()->battleCanShoot(shooter));
	EXPECT_EQ(predictedSwordsmen->getPosition(), swordsmen->getPosition());
	EXPECT_TRUE(hypotheticalBattle->battleCanShoot(predictedShooter)) << "moving attacker should release the shooter in the exchange";
}

TEST_F(BattleEvaluatorTest, HarpiesCanStayToBlockAnEnemyShooter)
{
	auto * shooter = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(7, 5), 50);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(12, 5), 10);
	auto * ally = addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(14, 5), 500);
	for(auto * unit : {shooter, harpies})
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	evaluateAsDefender();
	BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, harpies,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto action = evaluator.selectStackAction(harpies);
	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK_NO_RETURN) << "Harpies should stay to prevent the Marksmen from shooting";
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(harpies->unitSide()), action));
	ASSERT_TRUE(harpies->alive());
	ASSERT_FALSE(battle()->battleCanShoot(shooter));
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	const auto enemyCallback = std::make_shared<CBattleCallback>(attackerSideHero->getOwner(), nullptr);
	enemyCallback->onBattleStarted(battle());
	BattleEvaluator opponent(gameHandler, enemyCallback, shooter, attackerSideHero->getOwner(), BattleID(0), BattleSide::ATTACKER, 1.0f, 2);
	const auto response = opponent.selectStackAction(shooter);
	ASSERT_NE(response.actionType, EActionType::SHOOT);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()), response));
	EXPECT_TRUE(harpies->alive());
	EXPECT_TRUE(ally->alive());
}

TEST_F(BattleEvaluatorTest, ReturningHarpyAllowsShooterResponseInExchange)
{
	auto * shooter = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(7, 5), 50);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(12, 5), 10);
	auto * ally = addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(14, 5), 500);
	for(auto * unit : {shooter, harpies, ally})
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	evaluateAsDefender();
	PotentialTargets targets(harpies, damageCache, hypotheticalBattle);
	BattleExchangeEvaluator evaluator(callback->getBattle(BattleID(0)), gameHandler, 1.0f, 1);
	evaluator.updateReachabilityMap(hypotheticalBattle);
	const BattleHex from(8, 5);
	const BattleAttackInfo info(harpies, shooter, 0, false);
	auto returning = AttackPossibility::evaluate(info, from, damageCache, hypotheticalBattle, true);
	auto staying = AttackPossibility::evaluate(info, from, damageCache, hypotheticalBattle, false);
	returning.shootersBlockedDmg = 0;
	staying.shootersBlockedDmg = 0;
	const auto returningScore = evaluator.evaluateExchange(returning, 0, targets, damageCache, hypotheticalBattle);
	const auto stayingScore = evaluator.evaluateExchange(staying, 0, targets, damageCache, hypotheticalBattle);
	EXPECT_GT(stayingScore, returningScore);

	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(harpies->unitSide()),
		BattleAction::makeMeleeAttack(harpies, shooter->getPosition(), from, true)));
	ASSERT_TRUE(shooter->alive());
	ASSERT_EQ(battle()->battleActiveUnit(), shooter);
	ASSERT_TRUE(battle()->battleCanShoot(shooter));
	const auto healthBefore = ally->getAvailableHealth();
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(shooter->unitSide()),
		BattleAction::makeShotAttack(shooter, ally)));
	EXPECT_LT(ally->getAvailableHealth(), healthBefore);
}

TEST_F(BattleEvaluatorTest, HarpiesReturnAfterKillingShooter)
{
	auto * distantShooter = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(1, 8), 10);
	auto * target = addStack(BattleSide::ATTACKER, creatureByName("core:marksman"), BattleHex(7, 7), 20);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(12, 5), 100);
	for(auto * unit : {distantShooter, target, harpies})
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	const auto start = harpies->getPosition();
	evaluateAsDefender();
	BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, harpies,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto action = evaluator.selectStackAction(harpies);
	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK) << "a killed shooter cannot justify staying within the other shooter's full range";
	ASSERT_EQ(action.target.size(), 2);
	ASSERT_EQ(action.target[1].hexValue, target->getPosition());
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(harpies->unitSide()), action));
	ASSERT_FALSE(target->alive());
	ASSERT_EQ(harpies->getPosition(), start);
	ASSERT_TRUE(battle()->battleHasShootingPenalty(distantShooter, harpies->getPosition()));
	ASSERT_EQ(battle()->battleActiveUnit(), distantShooter);
	const auto enemyCallback = std::make_shared<CBattleCallback>(attackerSideHero->getOwner(), nullptr);
	enemyCallback->onBattleStarted(battle());
	BattleEvaluator opponent(gameHandler, enemyCallback, distantShooter, attackerSideHero->getOwner(), BattleID(0), BattleSide::ATTACKER, 1.0f, 2);
	const auto response = opponent.selectStackAction(distantShooter);
	ASSERT_EQ(response.actionType, EActionType::SHOOT);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(distantShooter->unitSide()), response));
	EXPECT_GE(harpies->getCount(), 99);
}

TEST_F(BattleEvaluatorTest, HarpiesReturnWhenStayingExposesThemToMelee)
{
	auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(7, 5), 100);
	auto * harpies = addStack(BattleSide::DEFENDER, creatureByName("core:harpyHag"), BattleHex(12, 5), 10);
	harpies->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	beginCombat();
	ASSERT_EQ(battle()->battleActiveUnit(), harpies);
	const auto start = harpies->getPosition();
	evaluateAsDefender();
	BattleEvaluator evaluator(gameHandler, callback, hypotheticalBattle, damageCache, harpies,
		defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);
	const auto action = evaluator.selectStackAction(harpies);
	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_EQ(action.target.size(), 2);
	EXPECT_EQ(BattleHex::getDistance(enemy->getPosition(), action.target[0].hexValue), 1);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(harpies->unitSide()), action));
	ASSERT_TRUE(enemy->alive());
	EXPECT_EQ(harpies->getPosition(), start);
	for(const auto & hex : battle()->battleGetAvailableHexes(enemy, false))
		EXPECT_FALSE(battle()->isMeleeAttackPossible(enemy, harpies, hex));
}

TEST_F(BattleEvaluatorTest, AttacksSuitableTargetOnWayToMovementWaypoint)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(1, 8), 30);
	auto * griffin = addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(7, 4), 1);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(14, 5), 3);

	addObstacle();

	BattleHex movementWaypoint(9, 7);
	BattleHexArray movementWaypoints{movementWaypoint};
	auto reachability = battle()->getReachability(behemoth);
	ASSERT_EQ(reachability.distances[movementWaypoint.toInt()], behemoth->getMovementRange(0));
	auto action = moveTowards(behemoth, movementWaypoints, elf->getAttackableHexes(behemoth));

	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_EQ(action.target.size(), 2);
	EXPECT_EQ(action.target[1].hexValue, griffin->getPosition());
}

TEST_F(BattleEvaluatorTest, SkipsTargetIfAttackWouldDelayPrimaryTarget)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(2, 7), 30);
	addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(13, 1), 1);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(13, 5), 3);

	auto action = moveTowards(behemoth, elf);

	EXPECT_EQ(action.actionType, EActionType::WALK);
}

TEST_F(BattleEvaluatorTest, SkipsUnprofitableTargetWithoutDelayingPrimaryTarget)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(1, 8), 30);
	addStack(BattleSide::ATTACKER, creatureByName("core:angel"), BattleHex(7, 4), 20);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(14, 5), 3);

	addObstacle();

	BattleHexArray movementWaypoints{BattleHex(9, 7)};
	auto action = moveTowards(behemoth, movementWaypoints, elf->getAttackableHexes(behemoth));

	EXPECT_EQ(action.actionType, EActionType::WALK);
}

TEST_F(BattleEvaluatorTest, EnemyOnlyAreaAttackDoesNotDamageAnAlly)
{
	const auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(7, 5), 100);
	const auto * hydra = addStack(BattleSide::DEFENDER, creatureByName("core:hydra"), BattleHex(8, 5), 10);
	const auto * ally = addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(8, 4), 100);

	auto attack = evaluateMelee(hydra, enemy, hydra->getPosition());
	EXPECT_GT(attack.defenderDamageReduce, 0);
	EXPECT_EQ(attack.collateralDamageReduce, 0);
	EXPECT_FALSE(vstd::contains_if(attack.affectedUnits, [ally](const auto & unit)
	{
		return unit->unitId() == ally->unitId();
	}));
}

TEST_F(BattleEvaluatorTest, DragonBreathStillDamagesAnAllyBehindTheTarget)
{
	const auto * enemy = addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(7, 5), 100);
	addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(6, 5), 100);
	const auto * dragon = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(8, 5), 3);

	auto attack = evaluateMelee(dragon, enemy, dragon->getPosition());
	EXPECT_GT(attack.collateralDamageReduce, 0);
}

TEST_F(BattleEvaluatorTest, DoubleAttackReceivesOnlyOneRetaliation)
{
	const auto * griffins = addStack(BattleSide::ATTACKER, creatureByName("core:royalGriffin"), BattleHex(8, 5), 50);
	const auto * crusaders = addStack(BattleSide::DEFENDER, creatureByName("core:crusader"), BattleHex(9, 5), 20);
	ASSERT_EQ(crusaders->getTotalAttacks(false), 2);
	ASSERT_TRUE(griffins->hasBonusOfType(BonusType::UNLIMITED_RETALIATIONS));

	DamageEstimation retaliation;
	battle()->battleEstimateDamage(BattleAttackInfo(crusaders, griffins, 0, false), &retaliation);
	const int64_t retaliationDamage = (retaliation.damage.min + retaliation.damage.max) / 2;
	ASSERT_GT(retaliationDamage, 0);

	auto attack = evaluateMelee(crusaders, griffins, crusaders->getPosition());

	EXPECT_EQ(attack.attackerState->getAvailableHealth(), crusaders->getAvailableHealth() - retaliationDamage);
}

/// Drives the whole hero-spell decision of BattleAI and reports what it chose to cast. Spells are
/// the one part of the AI that is blind to plain damage numbers, so a scenario grants exactly one
/// spell and asks whether the AI found it worth casting at all.
class BattleEvaluatorSpellTest : public BattleEvaluatorTest
{
public:
	/// Captures the decision instead of sending it to a server, which tests do not run
	class DecisionRecorder : public CBattleCallback
	{
	public:
		using CBattleCallback::CBattleCallback;

		std::optional<BattleAction> spellAction;

		void battleMakeSpellAction(const BattleID &, const BattleAction & action) override
		{
			spellAction = action;
		}
	};

	/// Lets our hero - the one defending - cast the given spell, and nothing else
	void teachDefender(SpellID spell, int spellPower = 10)
	{
		giveArtifact(defenderSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
		defenderSideHero->addSpellToSpellbook(spell);
		defenderSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, spellPower, ChangeValueMode::ABSOLUTE);
		defenderSideHero->setPrimarySkill(PrimarySkill::KNOWLEDGE, spellPower, ChangeValueMode::ABSOLUTE);
		defenderSideHero->mana = 9999;
	}

	/// Runs one turn of our stack exactly as BattleAI would: pick the best action first, so that a
	/// spell is only cast when it beats what the stack could do on its own, then offer the spell
	bool canBreachWalls = false;

	std::optional<BattleAction> decideSpell(const CStack * activeStack)
	{
		const auto side = activeStack->unitSide();
		const auto owner = (side == BattleSide::DEFENDER ? defenderSideHero : attackerSideHero)->getOwner();
		auto recorder = std::make_shared<DecisionRecorder>(owner, nullptr);
		recorder->onBattleStarted(battle());

		BattleEvaluator evaluator(gameHandler, recorder, activeStack, owner, BattleID(0), side, 1.0f, 2);

		evaluator.selectStackAction(activeStack);
		evaluator.attemptCastingSpell(activeStack);
		canBreachWalls = evaluator.canBreachWalls();

		return recorder->spellAction;
	}

	/// The unit a recorded spell action was aimed at
	const CStack * targetOf(const BattleAction & action) const
	{
		if(action.target.empty())
			return nullptr;

		return battle()->battleGetStackByPos(action.target.front().hexValue, true);
	}
};

TEST_F(BattleEvaluatorSpellTest, SlowsTheEnemyItIsAlreadyStandingNextTo)
{
	teachDefender(SpellID::SLOW);

	// adjacent, so that attacking is a live option and the spell has to be worth more than it.
	// A fast enemy has the most to lose from being slowed, and loses no health by it
	auto * cavalier = addStack(BattleSide::ATTACKER, creatureByName("core:cavalier"), BattleHex(7, 5), 20);
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 30);

	auto action = decideSpell(ours);

	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(action->spell, SpellID(SpellID::SLOW));
	EXPECT_EQ(targetOf(*action), cavalier);
}

TEST_F(BattleEvaluatorSpellTest, HastensOwnUnitAlthoughItAddsNoDamage)
{
	teachDefender(SpellID::HASTE);

	addStack(BattleSide::ATTACKER, creatureByName("core:swordsman"), BattleHex(1, 5), 30);
	// slow enough that the walk across the field costs it a real share of the battle
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:zombie"), BattleHex(15, 5), 40);

	auto action = decideSpell(ours);

	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(action->spell, SpellID(SpellID::HASTE));
	EXPECT_EQ(targetOf(*action), ours);
}

TEST_F(BattleEvaluatorSpellTest, SlowsTheEnemyThatIsWorthMore)
{
	teachDefender(SpellID::SLOW);

	auto * angels = addStack(BattleSide::ATTACKER, creatureByName("core:angel"), BattleHex(1, 3), 10);
	addStack(BattleSide::ATTACKER, creatureByName("core:peasant"), BattleHex(1, 7), 10);
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(15, 5), 30);

	auto action = decideSpell(ours);

	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(targetOf(*action), angels);
}

TEST_F(BattleEvaluatorSpellTest, PrefersRestoringManyCreaturesToDamageSpell)
{
	teachDefender(SpellID::RESURRECTION);
	defenderSideHero->setSecSkillLevel(SecondarySkill::EARTH_MAGIC, 2, ChangeValueMode::ABSOLUTE);
	defenderSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));

	addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(7, 5), 15);
	const auto * active = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(8, 5), 40);
	auto * wounded = addStack(BattleSide::DEFENDER, creatureByName("core:peasant"), BattleHex(14, 5), 300);
	// a single survivor must not cap the value of restoring the rest, or Magic Arrow would look better
	int64_t damage = wounded->getAvailableHealth() - wounded->getMaxHealth();
	wounded->damage(damage);
	ASSERT_EQ(wounded->getCount(), 1);

	auto action = decideSpell(active);

	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(action->spell, SpellID(SpellID::RESURRECTION));
	EXPECT_EQ(targetOf(*action), wounded);
}

TEST_F(BattleEvaluatorSpellTest, DeclinesUselessSpellWhenStackCannotAct)
{
	teachDefender(SpellID::PROTECTION_FROM_AIR);

	addStack(BattleSide::ATTACKER, creatureByName("core:swordsman"), BattleHex(1, 5), 30);
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(15, 5), 30);
	ours->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BIND_EFFECT, BonusSource::OTHER, 0, BonusSourceID()));
	ASSERT_EQ(ours->getMovementRange(), 0);

	auto action = decideSpell(ours);

	EXPECT_FALSE(action.has_value());
}

TEST_F(BattleEvaluatorSpellTest, PrefersCheaperSpellForAnEquivalentFinishingKill)
{
	teachDefender(SpellID::IMPLOSION, 18);
	defenderSideHero->addSpellToSpellbook(SpellID(SpellID::FIREBALL));
	defenderSideHero->setSecSkillLevel(SecondarySkill::FIRE_MAGIC, 3, ChangeValueMode::ABSOLUTE);
	defenderSideHero->setSecSkillLevel(SecondarySkill::EARTH_MAGIC, 3, ChangeValueMode::ABSOLUTE);
	addStack(BattleSide::ATTACKER, creatureByName("core:zealot"), BattleHex(1, 2), 3);
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:devil"), BattleHex(6, 2), 1);
	int64_t damage = ours->getAvailableHealth() - 39;
	ours->damage(damage);

	auto action = decideSpell(ours);
	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(action->spell, SpellID(SpellID::FIREBALL));
}

/// Attacker of a town whose catapult is already gone
class BattleEvaluatorSiegeTest : public BattleEvaluatorSpellTest
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startSiege();
		battle()->stacks.clear();
		teachSpell(attackerSideHero, SpellID::EARTHQUAKE);
	}

};

TEST_F(BattleEvaluatorSiegeTest, BreachesIntactWallsForWalkingUnits)
{
	auto * ours = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(3, 5), 30);
	addStack(BattleSide::DEFENDER, creatureByName("core:archer"), BattleHex(14, 5), 20);

	auto action = decideSpell(ours);
	EXPECT_TRUE(canBreachWalls);
	ASSERT_TRUE(action.has_value());
	EXPECT_EQ(action->spell, SpellID(SpellID::EARTHQUAKE));
}

TEST_F(BattleEvaluatorSiegeTest, IgnoresWallsThatAlreadyHaveBreach)
{
	auto * ours = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(3, 5), 30);
	addStack(BattleSide::DEFENDER, creatureByName("core:archer"), BattleHex(14, 5), 20);
	battle()->setWallState(EWallPart::BOTTOM_WALL, EWallState::DESTROYED);

	auto action = decideSpell(ours);
	EXPECT_FALSE(canBreachWalls);
	EXPECT_FALSE(action.has_value() && action->spell == SpellID(SpellID::EARTHQUAKE));
}

TEST_F(BattleEvaluatorSiegeTest, CannotBreachWallsUnderSpellLevelLimit)
{
	auto * ours = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(3, 5), 30);
	addStack(BattleSide::DEFENDER, creatureByName("core:archer"), BattleHex(14, 5), 20);
	// Recanter's Cloak forbids Earthquake, a level 3 spell
	attackerSideHero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BLOCK_MAGIC_ABOVE, BonusSource::OTHER, 2, BonusSourceID()));

	decideSpell(ours);
	EXPECT_FALSE(canBreachWalls);
}
}
