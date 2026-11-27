/*
 * BattleDrawTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/gameState/TavernHeroesPool.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../server/CGameHandler.h"
#include "../../../server/activities/ActivityProcessor.h"
#include "../../../server/activities/BattleActivities.h"

namespace
{
constexpr int pikeman = 0;
}

/// A side left with nothing but summoned units can not win, so the battle ends in a draw whether
/// its enemy is defeated or flees from it
class BattleDrawTest : public BattleTestFixture
{
public:
	static constexpr int32_t attackerCount = 10;

	/// Stack of the army the hero brought into the battle
	CStack * armyStack(BattleSide side) const
	{
		for(const auto & unit : battle()->stacks)
			if(unit->unitSide() == side && unit->unitSlot() == SlotID(0))
				return unit.get();
		return nullptr;
	}

	const CStack * summonedStack(BattleSide side) const
	{
		for(const auto & unit : battle()->stacks)
			if(unit->unitSide() == side && unit->summoned && unit->alive())
				return unit.get();
		return nullptr;
	}

	/// Both heroes bring pikemen, and the battle ends through the battle activity of their players
	void startBattleWithArmies()
	{
		startGame();
		attackerSideHero->setCreature(SlotID(0), CreatureID(pikeman), attackerCount);
		startBattle();
		gameHandler->activities->addActivity(std::make_shared<BattleActivity>(gameHandler.get(), battle()));
	}

	/// The defender summons elementals and loses the only stack of its army
	void leaveDefenderWithSummonedUnitsOnly()
	{
		const SpellID summon = spellByName("vcmi-test:testSummonSameUnit");
		teachSpell(defenderSideHero, summon);
		ASSERT_TRUE(castAsHero(defenderSideHero, summon, battle::Target{}));

		const CStack * army = armyStack(BattleSide::DEFENDER);
		injure(army, army->getAvailableHealth());
		ASSERT_FALSE(battle()->battleHasPermanentUnits(BattleSide::DEFENDER));
	}

	bool actAsAttacker(const BattleAction & action)
	{
		battle()->activeStack = armyStack(BattleSide::ATTACKER)->unitId();
		return act(action);
	}

	TavernSlotRole tavernRole(const CGHeroInstance * hero) const
	{
		return gameState()->heroesPool->getSlotRole(hero->getHeroTypeID());
	}
};

TEST_F(BattleDrawTest, retreatFromSummonedUnitsSendsBothHeroesToTavernWithSingleUnit)
{
	startBattleWithArmies();
	giveArtifact(defenderSideHero, ArtifactID::GRAIL, ArtifactPosition::BACKPACK_START);
	leaveDefenderWithSummonedUnitsOnly();
	const auto attackerExperience = attackerSideHero->exp;
	const auto defenderExperience = defenderSideHero->exp;

	ASSERT_TRUE(actAsAttacker(BattleAction::makeRetreat(BattleSide::ATTACKER)));

	ASSERT_EQ(server.battleResults.size(), 1u);
	EXPECT_EQ(server.battleResults.front().result, EBattleResult::ESCAPE);
	EXPECT_EQ(server.battleResults.front().winner, BattleSide::NONE);

	for(const auto * hero : {attackerSideHero, defenderSideHero})
	{
		EXPECT_EQ(tavernRole(hero), TavernSlotRole::RETREATED);
		EXPECT_EQ(hero->stacksCount(), 1);
		EXPECT_EQ(hero->getStackCount(SlotID(0)), 1);
	}
	EXPECT_EQ(attackerSideHero->exp, attackerExperience);
	EXPECT_EQ(defenderSideHero->exp, defenderExperience);
	EXPECT_FALSE(defenderSideHero->hasArt(ArtifactID::GRAIL));
}

TEST_F(BattleDrawTest, surrenderToSummonedUnitsSendsBothHeroesToTavernWithTheirArmies)
{
	startBattleWithArmies();
	leaveDefenderWithSummonedUnitsOnly();

	ASSERT_TRUE(actAsAttacker(BattleAction::makeSurrender(BattleSide::ATTACKER)));

	ASSERT_EQ(server.battleResults.size(), 1u);
	EXPECT_EQ(server.battleResults.front().result, EBattleResult::SURRENDER);
	EXPECT_EQ(server.battleResults.front().winner, BattleSide::NONE);

	EXPECT_EQ(tavernRole(attackerSideHero), TavernSlotRole::SURRENDERED);
	EXPECT_EQ(attackerSideHero->getStackCount(SlotID(0)), attackerCount);

	// the summoned units are gone with the battle, so the defender has a single unit to return with
	EXPECT_EQ(tavernRole(defenderSideHero), TavernSlotRole::SURRENDERED);
	EXPECT_EQ(defenderSideHero->stacksCount(), 1);
}

TEST_F(BattleDrawTest, defeatingEnemyWithSummonedUnitsOnlyLosesBothHeroes)
{
	startBattleWithArmies();
	giveArtifact(defenderSideHero, ArtifactID::GRAIL, ArtifactPosition::BACKPACK_START);
	leaveDefenderWithSummonedUnitsOnly();
	const auto defenderExperience = defenderSideHero->exp;

	const CStack * attackerArmy = armyStack(BattleSide::ATTACKER);
	injure(attackerArmy, attackerArmy->getAvailableHealth());
	ASSERT_TRUE(defend(summonedStack(BattleSide::DEFENDER)));

	ASSERT_EQ(server.battleResults.size(), 1u);
	EXPECT_EQ(server.battleResults.front().result, EBattleResult::NORMAL);
	EXPECT_EQ(server.battleResults.front().winner, BattleSide::NONE);

	EXPECT_EQ(tavernRole(attackerSideHero), TavernSlotRole::NONE);
	EXPECT_EQ(tavernRole(defenderSideHero), TavernSlotRole::NONE);
	EXPECT_EQ(defenderSideHero->exp, defenderExperience);
	EXPECT_FALSE(defenderSideHero->hasArt(ArtifactID::GRAIL));
}
