/*
 * RebirthTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/bonuses/BonusCustomTypes.h"

namespace
{

/// Phoenixes placed, and how many of them the 20% rebirth brings back. Chosen so that the share is
/// a whole number of creatures, which is what makes the rebirth deterministic rather than rolled.
constexpr int32_t phoenixCount = 10;
constexpr int32_t rebornCount = 2;

constexpr int32_t slayerCount = 100;

}

/// A phoenix answers the blow that killed it by coming back with a fifth of the stack it started
/// as, once per battle. What kills it does not matter; what it was does - a clone leaves nothing
/// to come back.
class RebirthTest : public BattleTestFixture
{
public:
	/// A stack of phoenixes on the defending side, and enough black dragons facing it to kill them
	/// all in one blow.
	void setUpBattle(int32_t phoenixes = phoenixCount)
	{
		startGame();
		startBattle();

		phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex), phoenixes);
		slayer = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), slayerCount);
		ASSERT_NE(phoenix, nullptr);
		ASSERT_NE(slayer, nullptr);
	}

	/// Strikes the phoenix with everything the dragons have, which is more than enough to kill the
	/// whole stack in one blow.
	void slay()
	{
		ASSERT_TRUE(attack(slayer, BattleHex(rightHex)));
	}

	CStack * phoenix = nullptr;
	CStack * slayer = nullptr;
};

TEST_F(RebirthTest, ComesBackWithAFifthOfTheStackItStartedAs)
{
	setUpBattle();
	beginCombat();

	slay();

	EXPECT_TRUE(phoenix->alive()) << "the whole point of the ability is that the stack is not gone";
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}

/// The rebirth is spent the first time it is needed, so a second death is final.
TEST_F(RebirthTest, ComesBackOnlyOnce)
{
	setUpBattle();
	beginCombat();

	slay();
	ASSERT_TRUE(phoenix->alive());

	slay();

	EXPECT_FALSE(phoenix->alive());
}

/// The blow that killed it ends the attack it belonged to - the dragons do not get to strike the
/// reborn stack with the rest of their blows. Disabled: the stack currently comes back while the
/// attack is still running, so the blows after the lethal one land on it.
TEST_F(RebirthTest, DISABLED_TheKillingBlowIsTheLastOfItsAttack)
{
	setUpBattle();

	slayer->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::OTHER, 1, BonusSourceID()));
	ASSERT_EQ(slayer->getTotalAttacks(false), 2);

	beginCombat();

	slay();

	EXPECT_TRUE(phoenix->alive()) << "the second blow struck a stack that was dead when it was thrown";
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}

/// Coming back costs the stack its answer for the round, so the next attacker strikes it for free.
TEST_F(RebirthTest, AnswersNobodyDuringTheRoundItCameBackIn)
{
	setUpBattle();

	CStack * follower = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), 1);
	ASSERT_NE(follower, nullptr);

	beginCombat();

	slay();
	ASSERT_TRUE(phoenix->alive());

	ASSERT_TRUE(attack(follower, BattleHex(rightHex + 1)));

	EXPECT_TRUE(phoenix->alive()) << "the follower is far too small to finish the reborn stack";
	EXPECT_EQ(follower->getCount(), 1) << "a single pikeman does not survive being answered by phoenixes";
}

/// A clone is a copy that leaves nothing behind, so there is nothing to come back.
TEST_F(RebirthTest, ACloneDoesNotComeBack)
{
	setUpBattle();
	beginCombat();

	// marking the stack a clone is the shortest way to the state a cloned one would be in
	makeClone(phoenix);

	slay();

	EXPECT_FALSE(phoenix->alive());
}

/// What killed it is not part of the ability - a spell that wipes the stack is answered the same
/// way a blow is.
TEST_F(RebirthTest, ComesBackFromADeathBySpell)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::IMPLOSION));
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 9999;

	startBattle();

	phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex), phoenixCount);
	slayer = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), slayerCount);
	ASSERT_NE(phoenix, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::IMPLOSION), phoenix));

	EXPECT_TRUE(phoenix->alive());
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}

/// A stack too small for a fifth of it to be one creature still comes back, when the rebirth is
/// the kind that guarantees one.
TEST_F(RebirthTest, TheGuaranteedKindAlwaysBringsBackOne)
{
	setUpBattle(1);

	phoenix->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::REBIRTH, BonusSource::OTHER, 20, BonusSourceID(), BonusCustomSubtype::rebirthSpecial));

	beginCombat();

	slay();

	EXPECT_TRUE(phoenix->alive());
	EXPECT_EQ(phoenix->getCount(), 1);
}
