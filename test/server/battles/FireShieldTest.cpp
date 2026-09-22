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

/// Fire shield integration scenarios for spell-power and resistance modifiers
class FireShieldTest : public BattleTestFixture, public ::testing::WithParamInterface<FireShieldCase>
{
public:
	/// Prevents remaining-health limits from affecting reflected damage
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

	// retaliation would injure the attacker as well, hiding the reflected damage
	blockRetaliation(attacking);

	// Fire immunity rejects the spell but preserves the creature ability.
	castOn(attackerSideHero, SpellID::FIRE_SHIELD, shielded);

	// Bless fixes attack damage at the maximum. The unmodified enemy hero casts it to isolate shield modifiers.
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

// Base reflected damage is 600/750/900 by mastery. Sorcery and Orb of Vulnerability increase it;
// fire resistance reduces it.
INSTANTIATE_TEST_SUITE_P(Scenarios, FireShieldTest, ::testing::Values(
	// Spell mastery controls the base percentage.
	FireShieldCase{"plain",              pikeman, pikeman, noSkill,   0,        noArtifact,  600},
	FireShieldCase{"fireMagicBasic",     pikeman, pikeman, fireMagic, basic,    noArtifact,  600},
	FireShieldCase{"fireMagicAdvanced",  pikeman, pikeman, fireMagic, advanced, noArtifact,  750},
	FireShieldCase{"fireMagicExpert",    pikeman, pikeman, fireMagic, expert,   noArtifact,  900},

	// Sorcery scales reflected damage.
	FireShieldCase{"sorceryBasic",       pikeman, pikeman, sorcery,   basic,    noArtifact,  630},
	FireShieldCase{"sorceryAdvanced",    pikeman, pikeman, sorcery,   advanced, noArtifact,  660},
	FireShieldCase{"sorceryExpert",      pikeman, pikeman, sorcery,   expert,   noArtifact,  690},

	// Orb of Fire adds its school bonus.
	FireShieldCase{"orb",                pikeman, pikeman, noSkill,   0,        orbOfFire,   900},
	FireShieldCase{"orbAndFireMagic",    pikeman, pikeman, fireMagic, expert,   orbOfFire,  1350},
	FireShieldCase{"orbAndSorcery",      pikeman, pikeman, sorcery,   expert,   orbOfFire,  1035},

	// Fire immunity prevents reflected damage.
	FireShieldCase{"immuneEfreet",       pikeman, efreet,        noSkill, 0, noArtifact, 0},
	FireShieldCase{"immuneElemental",    pikeman, fireElemental, noSkill, 0, noArtifact, 0},

	// Golem resistance reduces reflected damage.
	FireShieldCase{"resistantStoneGolem",   pikeman, stoneGolem,   noSkill, 0, noArtifact, 300},
	FireShieldCase{"resistantGoldGolem",    pikeman, goldGolem,    noSkill, 0, noArtifact, 390},
	FireShieldCase{"resistantDiamondGolem", pikeman, diamondGolem, noSkill, 0, noArtifact, 196},

	// Floating-point calculation produces 3218 instead of the integer-arithmetic result 3220.
	FireShieldCase{"vulnerableWaterElemental", pikeman, waterElemental, noSkill, 0, noArtifact, 3218},

	// Efreet Sultan immunity leaves only its native 20% fire shield.
	FireShieldCase{"efreetSultanDoesNotStack", efreetSultan, pikeman, fireMagic, expert, noArtifact, 600},

	// Orb of Vulnerability permits the spell; the shared stacking group selects the larger percentage.
	FireShieldCase{"efreetSultanVulnerable",       efreetSultan, pikeman, noSkill,   0,      orbOfVulnerability, 600},
	FireShieldCase{"efreetSultanVulnerableExpert", efreetSultan, pikeman, fireMagic, expert, orbOfVulnerability, 900}
),
	[](const ::testing::TestParamInfo<FireShieldCase> & info) { return info.param.name; });

/// Verifies reflection from rolled damage instead of the maximum damage range
class FireShieldRollTest : public BattleTestFixture
{
};

TEST_F(FireShieldRollTest, reflectsTheHitThatLandedRatherThanTheBestPossibleRoll)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID::FIRE_SHIELD);
	attackerSideHero->mana = 9999;

	startBattle();

	// Equal attack and defence make rolled and undefended damage equal.
	CStack * shielded = addStack(BattleSide::ATTACKER, CreatureID(hornedDemon), BattleHex(leftHex), 5000);
	CStack * attacking = addStack(BattleSide::DEFENDER, CreatureID(hornedDemon), BattleHex(rightHex), 1000);

	blockRetaliation(attacking);
	ASSERT_TRUE(castOn(attackerSideHero, SpellID::FIRE_SHIELD, shielded));

	const int64_t shieldedHealthBefore = shielded->getAvailableHealth();
	const int64_t attackingHealthBefore = attacking->getAvailableHealth();

	ASSERT_TRUE(attack(attacking, BattleHex(leftHex)));

	const int64_t dealt = shieldedHealthBefore - shielded->getAvailableHealth();
	const int64_t reflected = attackingHealthBefore - attacking->getAvailableHealth();

	// A range-based implementation would reflect from the 9000 maximum.
	ASSERT_LT(dealt, 9000);
	EXPECT_EQ(reflected, dealt * 20 / 100);
}
