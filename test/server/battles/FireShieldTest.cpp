/*
 * FireShieldTest.cpp, part of VCMI engine
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

/// One scenario: which creatures meet, what the casting hero knows and carries,
/// and how much health the attacker must lose to the fire shield.
struct FireShieldCase
{
	const char * name;
	int shieldedCreature;  ///< attacker-side unit, receives the Fire Shield spell
	int attackingCreature; ///< defender-side unit, strikes it and takes the reflected damage
	int skill;             ///< secondary skill given to the casting hero, -1 for none
	int mastery;           ///< 1 basic, 2 advanced, 3 expert
	int artifact;          ///< artifact equipped on the casting hero, -1 for none
	int64_t expectedDamage;
};

}

/// Fire shield reflects part of the damage a melee attacker deals back at it. The amount
/// passes through the spell damage pipeline of the shielded unit's hero, so hero skills and
/// artifacts scale it, and the attacker's own fire resistances cut it down again.
///
/// Every scenario is the same battle: the casting hero shields its own unit, the enemy hero
/// blesses its own unit so that it always deals its maximum damage, and that enemy attacks.
/// The health the attacker loses is the reflected damage.
class FireShieldTest : public BattleTestFixture, public ::testing::WithParamInterface<FireShieldCase>
{
public:
	/// The shielded stack is oversized on purpose: the reflected amount is capped by its
	/// remaining health, and no scenario is meant to hit that cap.
	static constexpr int32_t shieldedCount = 5000;
	static constexpr int32_t attackingCount = 1000;
};

TEST_P(FireShieldTest, reflectsExpectedDamage)
{
	const auto & scenario = GetParam();

	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FIRE_SHIELD);
	attackerSideHero->mana = 9999;

	giveArtifact(defenderSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	defenderSideHero->addSpellToSpellbook(SpellID(SpellID::BLESS));
	defenderSideHero->mana = 9999;

	if(scenario.skill >= 0)
		attackerSideHero->setSecSkillLevel(SecondarySkill(scenario.skill), scenario.mastery, ChangeValueMode::ABSOLUTE);

	if(scenario.artifact >= 0)
		giveArtifact(attackerSideHero, ArtifactID(scenario.artifact), ArtifactPosition::MISC1);

	startBattle();

	CStack * shielded = addStack(BattleSide::ATTACKER, CreatureID(scenario.shieldedCreature), BattleHex(leftHex), shieldedCount);
	CStack * attacking = addStack(BattleSide::DEFENDER, CreatureID(scenario.attackingCreature), BattleHex(rightHex), attackingCount);
	ASSERT_NE(shielded, nullptr);
	ASSERT_NE(attacking, nullptr);

	// retaliation would injure the attacker as well, hiding the reflected damage
	blockRetaliation(attacking);

	// a fire-immune target refuses the spell, which is what leaves its own shield untouched
	castOn(attackerSideHero, SpellID::FIRE_SHIELD, shielded);

	// bless collapses the damage range onto its maximum, making the reflected amount exact.
	// It comes from the enemy hero, whose kit no scenario touches, so nothing granted to the
	// casting hero can reach the damage the attack itself deals
	ASSERT_TRUE(castOn(defenderSideHero, SpellID(SpellID::BLESS), attacking));

	const int64_t healthBefore = attacking->getAvailableHealth();

	ASSERT_TRUE(attack(attacking, BattleHex(leftHex)));

	EXPECT_EQ(healthBefore - attacking->getAvailableHealth(), scenario.expectedDamage) << scenario.name;
}

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int stoneGolem = 33;
constexpr int efreet = 52;
constexpr int efreetSultan = 53;
constexpr int fireElemental = 114;
constexpr int hornedDemon = 49;
constexpr int waterElemental = 115;
constexpr int goldGolem = 116;
constexpr int diamondGolem = 117;

// what the casting hero may be given
constexpr int noSkill = -1;
constexpr int fireMagic = 14;
constexpr int sorcery = 25;
constexpr int noArtifact = -1;
constexpr int orbOfFire = 81; // Orb of Tempestuous Fire, +50% to fire spell damage
constexpr int orbOfVulnerability = 93; // negates natural immunities battle-wide

constexpr int basic = 1;
constexpr int advanced = 2;
constexpr int expert = 3;
}

// 1000 pikemen blessed to their maximum damage reflect a base of 3000 off a plain target, so
// the undisturbed reflection is the shield percentage of that: 600 at base mastery, 750
// advanced, 900 expert. Sorcery and the orb scale that, the attacker's own fire resistance
// cuts it down.
INSTANTIATE_TEST_SUITE_P(Scenarios, FireShieldTest, ::testing::Values(
	// the spell alone, and the mastery that decides its percentage
	FireShieldCase{"plain",              pikeman, pikeman, noSkill,   0,        noArtifact,  600},
	FireShieldCase{"fireMagicBasic",     pikeman, pikeman, fireMagic, basic,    noArtifact,  600},
	FireShieldCase{"fireMagicAdvanced",  pikeman, pikeman, fireMagic, advanced, noArtifact,  750},
	FireShieldCase{"fireMagicExpert",    pikeman, pikeman, fireMagic, expert,   noArtifact,  900},

	// sorcery scales the reflected damage like any other spell damage
	FireShieldCase{"sorceryBasic",       pikeman, pikeman, sorcery,   basic,    noArtifact,  630},
	FireShieldCase{"sorceryAdvanced",    pikeman, pikeman, sorcery,   advanced, noArtifact,  660},
	FireShieldCase{"sorceryExpert",      pikeman, pikeman, sorcery,   expert,   noArtifact,  690},

	// the orb adds its fire school bonus on top of both
	FireShieldCase{"orb",                pikeman, pikeman, noSkill,   0,        orbOfFire,   900},
	FireShieldCase{"orbAndFireMagic",    pikeman, pikeman, fireMagic, expert,   orbOfFire,  1350},
	FireShieldCase{"orbAndSorcery",      pikeman, pikeman, sorcery,   expert,   orbOfFire,  1035},

	// a fire-immune attacker takes nothing at all
	FireShieldCase{"immuneEfreet",       pikeman, efreet,        noSkill, 0, noArtifact, 0},
	FireShieldCase{"immuneElemental",    pikeman, fireElemental, noSkill, 0, noArtifact, 0},

	// golems reduce magic damage, so they take a fraction of what they reflect back
	FireShieldCase{"resistantStoneGolem",   pikeman, stoneGolem,   noSkill, 0, noArtifact, 300},
	FireShieldCase{"resistantGoldGolem",    pikeman, goldGolem,    noSkill, 0, noArtifact, 390},
	FireShieldCase{"resistantDiamondGolem", pikeman, diamondGolem, noSkill, 0, noArtifact, 196},

	// water elementals take double damage from this spell. 3218 rather than 3220 because the
	// damage calculator works in doubles and 7000 * 1.15 lands just below 8050
	FireShieldCase{"vulnerableWaterElemental", pikeman, waterElemental, noSkill, 0, noArtifact, 3218},

	// an efreet sultan already burns its attackers; the spell it is immune to adds nothing,
	// so expert mastery still reflects only the creature's own 20%
	FireShieldCase{"efreetSultanDoesNotStack", efreetSultan, pikeman, fireMagic, expert, noArtifact, 600},

	// with the Orb of Vulnerability the sultan loses that immunity and does accept the spell, so
	// both shields sit on the same unit. They share a stacking group, so only the stronger one is
	// active: the creature's own 20% until expert mastery makes the spell's 30% win
	FireShieldCase{"efreetSultanVulnerable",       efreetSultan, pikeman, noSkill,   0,      orbOfVulnerability, 600},
	FireShieldCase{"efreetSultanVulnerableExpert", efreetSultan, pikeman, fireMagic, expert, orbOfVulnerability, 900}
),
	[](const ::testing::TestParamInfo<FireShieldCase> & info) { return info.param.name; });

/// Every scenario above blesses the attacker, which collapses its damage range and hides which end
/// of that range the reflection is taken from. This one leaves the range open.
class FireShieldRollTest : public BattleTestFixture
{
};

TEST_F(FireShieldRollTest, reflectsTheBlowThatLandedRatherThanTheBestPossibleRoll)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FIRE_SHIELD);
	attackerSideHero->mana = 9999;

	startBattle();

	// horned demons on both sides: 10 attack against 10 defence, and nothing else either stack
	// carries touches the damage, so what the blow deals is what it would deal undefended
	CStack * shielded = addStack(BattleSide::ATTACKER, CreatureID(hornedDemon), BattleHex(leftHex), 5000);
	CStack * attacking = addStack(BattleSide::DEFENDER, CreatureID(hornedDemon), BattleHex(rightHex), 1000);
	ASSERT_NE(shielded, nullptr);
	ASSERT_NE(attacking, nullptr);

	blockRetaliation(attacking);
	ASSERT_TRUE(castOn(attackerSideHero, SpellID::FIRE_SHIELD, shielded));

	const int64_t shieldedHealthBefore = shielded->getAvailableHealth();
	const int64_t attackingHealthBefore = attacking->getAvailableHealth();

	ASSERT_TRUE(attack(attacking, BattleHex(leftHex)));

	const int64_t dealt = shieldedHealthBefore - shielded->getAvailableHealth();
	const int64_t reflected = attackingHealthBefore - attacking->getAvailableHealth();

	// 1000 demons deal 7 to 9 each, and the reflection would follow the 9000 end if it were taken
	// from the range rather than from the roll
	ASSERT_LT(dealt, 9000);
	EXPECT_EQ(reflected, dealt * 20 / 100);
}
