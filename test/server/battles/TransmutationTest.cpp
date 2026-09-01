/*
 * TransmutationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/CCreatureHandler.h"

namespace
{

constexpr int32_t victimCount = 50;
constexpr int32_t transmuterCount = 1;

}

/// Transmutation replaces the attacked stack with a stack of another creature. The victim is a
/// black dragon and the replacement a pikeman, so that keeping the creature count and keeping the
/// total health tell two very different stories.
class TransmutationTest : public BattleTestFixture
{
public:
	void setUpBattle(const std::string & transmuter, const std::string & victimCreature = "core:blackDragon")
	{
		startGame();
		startBattle();

		victim = addStack(BattleSide::ATTACKER, creatureByName(victimCreature), BattleHex(leftHex), victimCount);
		attacker = addStack(BattleSide::DEFENDER, creatureByName(transmuter), BattleHex(rightHex), transmuterCount);
		ASSERT_NE(victim, nullptr);
		ASSERT_NE(attacker, nullptr);

		blockRetaliation(attacker);
	}

	/// Whoever stands on the victim's hex once the attack is over - the original stack, or the
	/// one that replaced it.
	const CStack * unitOnVictimHex() const
	{
		return battle()->battleGetStackByPos(BattleHex(leftHex), true);
	}

	CStack * victim = nullptr;
	CStack * attacker = nullptr;
};

TEST_F(TransmutationTest, KeepsTheCreatureCount)
{
	setUpBattle("vcmi-test:testTransmuterCount");

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));

	const CStack * replacement = unitOnVictimHex();
	ASSERT_NE(replacement, nullptr);
	EXPECT_EQ(replacement->unitType()->getId(), creatureByName("core:pikeman"));
	EXPECT_EQ(replacement->getCount(), victimCount);
}

TEST_F(TransmutationTest, KeepsTheTotalHealth)
{
	setUpBattle("vcmi-test:testTransmuterHealth");

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));

	const CStack * replacement = unitOnVictimHex();
	ASSERT_NE(replacement, nullptr);
	EXPECT_EQ(replacement->unitType()->getId(), creatureByName("core:pikeman"));

	// 50 dragons of 300 health each are worth 1500 pikemen of 10. Damage already dealt does not
	// count: the health that carries over is what the stack was worth at full strength
	EXPECT_EQ(replacement->getCount(), victimCount * 300 / 10);
}

TEST_F(TransmutationTest, ImmuneVictimIsLeftAlone)
{
	setUpBattle("vcmi-test:testTransmuterCount");

	victim->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::TRANSMUTATION_IMMUNITY, BonusSource::OTHER, 0, BonusSourceID()));

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));

	const CStack * survivor = unitOnVictimHex();
	ASSERT_NE(survivor, nullptr);
	EXPECT_EQ(survivor->unitType()->getId(), creatureByName("core:blackDragon"));
	EXPECT_EQ(survivor->getCount(), victimCount);
}

TEST_F(TransmutationTest, NonLivingVictimIsLeftAlone)
{
	// a golem is not alive, and nothing that is not alive can be transmuted
	setUpBattle("vcmi-test:testTransmuterCount", "core:ironGolem");

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));

	const CStack * survivor = unitOnVictimHex();
	ASSERT_NE(survivor, nullptr);
	EXPECT_EQ(survivor->unitType()->getId(), creatureByName("core:ironGolem"));
}
