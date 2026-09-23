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
#include "lib/battle/CObstacleInstance.h"
#include "lib/battle/CPlayerBattleCallback.h"
#include "lib/callback/CBattleCallback.h"
#include "server/CGameHandler.h"

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

	BattleAction moveTowards(
		const CStack * stack,
		const BattleHexArray & movementTargets,
		const BattleHexArray & finalDestinationHexes)
	{
		std::shared_ptr<Environment> environment = gameHandler;
		auto callback = std::make_shared<CBattleCallback>(defenderSideHero->getOwner(), nullptr);
		callback->onBattleStarted(battle());

		auto hypotheticalBattle = std::make_shared<HypotheticBattle>(environment.get(), callback->getBattle(BattleID(0)));
		DamageCache damageCache;
		damageCache.buildDamageCache(hypotheticalBattle, BattleSide::DEFENDER);
		PotentialTargets targets(stack, damageCache, hypotheticalBattle);
		BattleEvaluator evaluator(environment, callback, hypotheticalBattle, damageCache, stack,
			defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);

		return evaluator.goTowardsNearest(stack, movementTargets, targets, finalDestinationHexes);
	}

	BattleAction moveTowards(const CStack * stack, const CStack * target)
	{
		auto primaryTargetHexes = target->getAttackableHexes(stack);
		return moveTowards(stack, primaryTargetHexes, primaryTargetHexes);
	}

	void addObstacle()
	{
		const auto * obstacleInfo = LIBRARY->obstacles()->getByName("core:12");
		ASSERT_NE(obstacleInfo, nullptr);

		auto obstacle = std::make_shared<CObstacleInstance>();
		obstacle->ID = obstacleInfo->obstacle.getNum(); // ObDtS03: the dead tree and rocks from the reported battle
		obstacle->pos = BattleHex(9, 6);
		battle()->obstacles.push_back(obstacle);
	}
};

TEST_F(BattleEvaluatorTest, AttacksSuitableTargetOnWayToReportedMovementWaypoint)
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

/// Drives the whole hero-spell decision of BattleAI and reports what it chose to cast. Spells are
/// the one part of the AI that is blind to plain damage numbers, so a scenario grants exactly one
/// spell and asks whether the AI found it worth casting at all.
class BattleEvaluatorSpellTest : public BattleTestFixture
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

	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->stacks.clear();
	}

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
	std::optional<BattleAction> decideSpell(const CStack * activeStack)
	{
		std::shared_ptr<Environment> environment = gameHandler;
		auto callback = std::make_shared<DecisionRecorder>(defenderSideHero->getOwner(), nullptr);
		callback->onBattleStarted(battle());

		BattleEvaluator evaluator(environment, callback, activeStack, defenderSideHero->getOwner(),
			BattleID(0), BattleSide::DEFENDER, 1.0f, 2);

		evaluator.selectStackAction(activeStack);
		evaluator.attemptCastingSpell(activeStack);

		return callback->spellAction;
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

TEST_F(BattleEvaluatorSpellTest, DeclinesMagicDefenceAgainstAnEnemyWithoutMagic)
{
	// the enemy hero was stripped of its magic by the fixture and its army casts nothing, so a ward
	// against a school of magic guards against something that can not happen
	teachDefender(SpellID::PROTECTION_FROM_AIR);

	addStack(BattleSide::ATTACKER, creatureByName("core:swordsman"), BattleHex(1, 5), 30);
	auto * ours = addStack(BattleSide::DEFENDER, creatureByName("core:swordsman"), BattleHex(15, 5), 30);

	auto action = decideSpell(ours);

	EXPECT_FALSE(action.has_value());
}
}
