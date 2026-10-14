/*
 * LifeDrainTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

namespace
{

/// Sized so that one attack of the victims opens a wound far larger than any drain can close,
/// without wiping the vampires out.
constexpr int32_t vampireCount = 100;
constexpr int32_t victimCount = 800;

}

/// Life drain restores the vampire lords health equal to the damage they dealt, bringing back
/// fallen creatures of their own stack once the survivors are whole again. Nothing is restored
/// beyond what the stack lost, and nothing at all is drained out of the undead.
///
/// The vampires are wounded first, since a stack at full health has nowhere to put drained life.
class LifeDrainTest : public BattleTestFixture
{
public:
	void setUpBattle(const std::string & victimCreature)
	{
		startGame();
		startBattle();

		victim = addStack(BattleSide::ATTACKER, creatureByName(victimCreature), BattleHex(leftHex), victimCount);
		vampires = addStack(BattleSide::DEFENDER, creatureByName("core:vampireLord"), BattleHex(rightHex), vampireCount);
		ASSERT_NE(victim, nullptr);
		ASSERT_NE(vampires, nullptr);

		// retaliation on either side would drain and wound outside of the attack being measured
		blockRetaliation(victim);
		blockRetaliation(vampires);

		// vampires are undead, so no blessing will reach them - the effect of one is granted
		// directly instead, to collapse both damage ranges onto a single value
		forceMaximumDamage(victim);
		forceMaximumDamage(vampires);
	}

	void woundVampires()
	{
		ASSERT_TRUE(attack(victim, BattleHex(rightHex)));

		ASSERT_TRUE(vampires->alive());
		ASSERT_LT(vampires->getCount(), vampireCount) << "the wounding attack was meant to kill some vampires";
	}

	/// Health the stack is short of its full strength, which is the most a drain can restore.
	int64_t missingHealth() const
	{
		return static_cast<int64_t>(vampireCount) * vampires->getMaxHealth() - vampires->getAvailableHealth();
	}

	CStack * victim = nullptr;
	CStack * vampires = nullptr;
};

TEST_F(LifeDrainTest, RestoresHealthEqualToTheDamageDealt)
{
	setUpBattle("core:pikeman");
	woundVampires();

	const int64_t vampireHealthBefore = vampires->getAvailableHealth();
	const int64_t victimHealthBefore = victim->getAvailableHealth();
	const int64_t roomToHeal = missingHealth();

	ASSERT_TRUE(attack(vampires, BattleHex(leftHex)));

	const int64_t damageDealt = victimHealthBefore - victim->getAvailableHealth();
	ASSERT_GT(damageDealt, 0);
	ASSERT_LT(damageDealt, roomToHeal) << "the drain was capped by the wound, so it is not what is measured here";

	EXPECT_EQ(vampires->getAvailableHealth() - vampireHealthBefore, damageDealt);
}

TEST_F(LifeDrainTest, BringsBackFallenVampires)
{
	setUpBattle("core:pikeman");
	woundVampires();

	const int32_t countBefore = vampires->getCount();
	const int64_t healthBefore = vampires->getAvailableHealth();
	const int64_t victimHealthBefore = victim->getAvailableHealth();

	ASSERT_TRUE(attack(vampires, BattleHex(leftHex)));

	// the drained health patches up the wounded creature at the top of the stack first, and only
	// what is left over raises whole creatures behind it
	EXPECT_EQ(vampires->getAvailableHealth(), healthBefore + victimHealthBefore - victim->getAvailableHealth());
	EXPECT_GT(vampires->getCount(), countBefore);
}

TEST_F(LifeDrainTest, DrainsNothingFromTheUndead)
{
	setUpBattle("core:skeleton");
	woundVampires();

	const int64_t healthBefore = vampires->getAvailableHealth();

	ASSERT_TRUE(attack(vampires, BattleHex(leftHex)));

	EXPECT_EQ(vampires->getAvailableHealth(), healthBefore);
}

TEST_F(LifeDrainTest, UnwoundedStackDrainsNothing)
{
	setUpBattle("core:pikeman");

	const int64_t healthBefore = vampires->getAvailableHealth();
	ASSERT_EQ(healthBefore, static_cast<int64_t>(vampireCount) * vampires->getMaxHealth());

	ASSERT_TRUE(attack(vampires, BattleHex(leftHex)));

	EXPECT_EQ(vampires->getAvailableHealth(), healthBefore);
}
