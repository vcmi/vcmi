/*
 * DamageCalculatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/battle/BattleAttackInfo.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/bonuses/BonusCustomTypes.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"

namespace
{

/// Scenarios are built from the units that already carry the factor under test, so their stats
/// decide what the expectations look like. The pairings below are picked so the attacker and the
/// defender have the same attack and defense and cancel each other out, leaving the factor under
/// test as the only one at work.
///
///   angel      20/20, 200 health, a flat 50 damage - the melee workhorse
///   archangel  30/30, 250 health, a flat 50 damage, hates devils
///   titan      24/24, 300 health, 40..60 damage, shoots and ignores the melee penalty
///   champion   16/16, 100 health, 20..25 damage, jousting
///   monk       12/7,  30 health,  10..12 damage, shoots and does suffer the melee penalty
///   hornedDemon 10/10, 40 health, 7..9 damage, no abilities whatsoever
constexpr auto angel = "core:angel";
constexpr auto archangel = "core:archangel";
constexpr auto archDevil = "core:archDevil";
constexpr auto titan = "core:titan";
constexpr auto champion = "core:champion";
constexpr auto blackKnight = "core:blackKnight";
constexpr auto pikeman = "core:pikeman";
constexpr auto monk = "core:monk";
constexpr auto crusader = "core:crusader";
constexpr auto sharpshooter = "core:sharpshooter";
constexpr auto medusa = "core:medusa";
constexpr auto hornedDemon = "core:hornedDemon";
constexpr auto demon = "core:demon";
constexpr auto behemoth = "core:behemoth";
constexpr auto ancientBehemoth = "core:ancientBehemoth";
constexpr auto magicElemental = "core:magicElemental";
constexpr auto psychicElemental = "core:psychicElemental";
constexpr auto blackDragon = "core:blackDragon";
constexpr auto giant = "core:giant";
constexpr auto ballista = "core:ballista";

constexpr int32_t stackSize = 100;

constexpr int basic = 1;
constexpr int advanced = 2;
constexpr int expert = 3;

}

/// Damage estimation for one attack, with every input the scenario cares about set explicitly.
/// Nothing here casts a spell or resolves an attack: a bonus that a spell would grant is granted
/// directly, so that a failure points at the calculator rather than at whatever produced the bonus.
class DamageCalculatorTestBase : public BattleTestFixture
{
public:
	// two free hexes in the middle of the field, and the hex on the far side of the defender, for
	// the scenarios that need the attacker to stand behind its target
	static constexpr int attackerHex = leftHex;
	static constexpr int defenderHex = rightHex;
	static constexpr int behindHex = rightHex + 1;

	// opposite edges of the same row, far enough apart for a shot to be penalised
	static constexpr int nearEdgeHex = 5 * GameConstants::BFIELD_WIDTH + 1;
	static constexpr int farEdgeHex = 5 * GameConstants::BFIELD_WIDTH + 15;

	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
	}

	CStack * attacker(const std::string & creature, int32_t count = stackSize, const BattleHex & hex = BattleHex(attackerHex))
	{
		return addStack(BattleSide::ATTACKER, creatureByName(creature), hex, count);
	}

	CStack * defender(const std::string & creature, int32_t count = stackSize, const BattleHex & hex = BattleHex(defenderHex))
	{
		return addStack(BattleSide::DEFENDER, creatureByName(creature), hex, count);
	}

	DamageEstimation estimate(const CStack * from, const CStack * to, int chargeDistance = 0, bool shooting = false) const
	{
		BattleAttackInfo info(from, to, chargeDistance, shooting);
		return battle()->calculateDmgRange(info);
	}

	static void grant(CStack * stack, BonusType type, int32_t value, const BonusSubtypeID & subtype = BonusSubtypeID(), BonusSource source = BonusSource::OTHER)
	{
		stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, type, source, value, BonusSourceID(), subtype));
	}

	/// Casts a spell for real, so that a scenario covers the spell and its configuration as well as
	/// the factor it feeds. The caster is equipped on the spot and taught every school to the given
	/// mastery, which is what decides how strong the effect comes out.
	void cast(CGHeroInstance * caster, SpellID spell, const CStack * target, int mastery = MasteryLevel::NONE)
	{
		if(!caster->hasSpellbook())
			giveArtifact(caster, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

		caster->addSpellToSpellbook(spell);
		caster->mana = 9999;

		for(const auto & school : {"airMagic", "fireMagic", "waterMagic", "earthMagic"})
			setSkill(caster, school, mastery);

		ASSERT_TRUE(castOn(caster, spell, target)) << "the game refused to cast spell " << spell.getNum();
	}

	static void setSkill(CGHeroInstance * hero, const std::string & skill, int level)
	{
		auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "secondarySkill", skill);
		ASSERT_TRUE(identifier.has_value()) << "unknown secondary skill " << skill;
		hero->setSecSkillLevel(SecondarySkill(*identifier), level, ChangeValueMode::ABSOLUTE);
	}
};

class DamageCalculatorTest : public DamageCalculatorTestBase
{
};

// ---- no factors at all ------------------------------------------------------------------------

/// The whole suite reads against this: angels have as much attack as defense, so two of them
/// cancel out and the stack deals exactly what its creatures are worth.
TEST_F(DamageCalculatorTest, EqualAttackAndDefenseLeaveCreatureDamageAlone)
{
	auto result = estimate(attacker(angel), defender(angel));

	EXPECT_EQ(result.damage.min, 5000);
	EXPECT_EQ(result.damage.max, 5000);
	EXPECT_EQ(result.kills.min, 25);
	EXPECT_EQ(result.kills.max, 25);
}

/// Titans span 40 to 60 damage, and shrug off the melee penalty their own shooter status would
/// otherwise bring, so the range passes through untouched.
TEST_F(DamageCalculatorTest, DamageRangeFollowsCreatureDamageRange)
{
	auto result = estimate(attacker(titan), defender(titan));

	EXPECT_EQ(result.damage.min, 4000);
	EXPECT_EQ(result.damage.max, 6000);
	EXPECT_EQ(result.kills.min, 13);
	EXPECT_EQ(result.kills.max, 20);
}

// ---- attack and defense ------------------------------------------------------------------------

TEST_F(DamageCalculatorTest, AttackAdvantageAddsFivePercentPerPoint)
{
	attackerSideHero->setPrimarySkill(PrimarySkill::ATTACK, 10, ChangeValueMode::ABSOLUTE); // 30 against 20

	auto result = estimate(attacker(angel), defender(angel));

	EXPECT_EQ(result.damage.min, 7500);
	EXPECT_EQ(result.kills.min, 37);
}

TEST_F(DamageCalculatorTest, AttackAdvantageIsCappedAtFourHundredPercent)
{
	attackerSideHero->setPrimarySkill(PrimarySkill::ATTACK, 200, ChangeValueMode::ABSOLUTE); // +1000%, capped

	EXPECT_EQ(estimate(attacker(angel), defender(angel)).damage.min, 25000);
}

TEST_F(DamageCalculatorTest, DefenseAdvantageRemovesTwoAndAHalfPercentPerPoint)
{
	defenderSideHero->setPrimarySkill(PrimarySkill::DEFENSE, 10, ChangeValueMode::ABSOLUTE); // 30 against 20

	auto result = estimate(attacker(angel), defender(angel));

	EXPECT_EQ(result.damage.min, 3750);
	EXPECT_EQ(result.kills.min, 18);
}

TEST_F(DamageCalculatorTest, DefenseAdvantageIsCappedAtSeventyPercent)
{
	defenderSideHero->setPrimarySkill(PrimarySkill::DEFENSE, 100, ChangeValueMode::ABSOLUTE); // -250%, capped

	EXPECT_EQ(estimate(attacker(angel), defender(angel)).damage.min, 1500);
}

/// Behemoths strip part of the defense of whatever they hit, and the ancient ones strip more.
TEST_F(DamageCalculatorTest, BehemothIgnoresPartOfTheTargetDefense)
{
	// 40% of 20 defense, rounded up to 9, leaves 11 against 17 attack
	EXPECT_EQ(estimate(attacker(behemoth), defender(angel)).damage.min, 3900);
}

TEST_F(DamageCalculatorTest, AncientBehemothIgnoresMoreOfTheTargetDefense)
{
	// 80% of 20 defense, rounded up to 17, leaves 3 against 19 attack
	EXPECT_EQ(estimate(attacker(ancientBehemoth), defender(angel)).damage.min, 5400);
}

/// No Heroes 3 creature lowers the attack of whoever strikes it, so the bonus is granted directly.
TEST_F(DamageCalculatorTest, EnemyAttackReductionLowersEffectiveAttack)
{
	auto * target = defender(angel);
	grant(target, BonusType::ENEMY_ATTACK_REDUCTION, 50); // 20 attack halved to 10 against 20 defense

	EXPECT_EQ(estimate(attacker(angel), target).damage.min, 3750);
}

/// What survives the reduction is rounded down, so a share that does not divide evenly costs the
/// attacker the whole point rather than half of one.
TEST_F(DamageCalculatorTest, EnemyAttackReductionRoundsTheRemainingAttackDown)
{
	auto * target = defender(angel);
	grant(target, BonusType::ENEMY_ATTACK_REDUCTION, 57); // 43% of 20 attack is 8.6, leaving 8

	EXPECT_EQ(estimate(attacker(angel), target).damage.min, 3500);
}

// ---- hero secondary skills ---------------------------------------------------------------------

namespace
{

struct SkillCase
{
	const char * name;
	const char * skill;
	int mastery;
	bool onDefender;   ///< armorer belongs to the side being hit
	const char * unit; ///< used on both sides, so attack and defense cancel out
	bool shooting;
	int64_t expectedMin;
	int64_t expectedMax;
};

}

class SecondarySkillDamageTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<SkillCase>
{
};

TEST_P(SecondarySkillDamageTest, scalesDamageByItsMastery)
{
	const auto & scenario = GetParam();

	setSkill(scenario.onDefender ? defenderSideHero : attackerSideHero, scenario.skill, scenario.mastery);

	auto result = estimate(attacker(scenario.unit), defender(scenario.unit), 0, scenario.shooting);

	EXPECT_EQ(result.damage.min, scenario.expectedMin) << scenario.name;
	EXPECT_EQ(result.damage.max, scenario.expectedMax) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, SecondarySkillDamageTest, ::testing::Values(
	// 100 angels deal 5000 in melee before any skill applies
	SkillCase{"offenceBasic",     "offence", basic,    false, angel, false, 5500, 5500},
	SkillCase{"offenceAdvanced",  "offence", advanced, false, angel, false, 6000, 6000},
	SkillCase{"offenceExpert",    "offence", expert,   false, angel, false, 6500, 6500},

	// and take 5000 before the armorer of the defending hero cuts it down
	SkillCase{"armorerBasic",     "armorer", basic,    true,  angel, false, 4750, 4750},
	SkillCase{"armorerAdvanced",  "armorer", advanced, true,  angel, false, 4500, 4500},
	SkillCase{"armorerExpert",    "armorer", expert,   true,  angel, false, 4250, 4250},

	// 100 titans shoot for 4000..6000, and archery is worth more than offence at every mastery
	SkillCase{"archeryBasic",     "archery", basic,    false, titan, true,  4400, 6600},
	SkillCase{"archeryAdvanced",  "archery", advanced, false, titan, true,  5000, 7500},
	SkillCase{"archeryExpert",    "archery", expert,   false, titan, true,  6000, 9000},

	// each of the three only applies to the kind of attack it belongs to
	SkillCase{"offenceIgnoredWhenShooting", "offence", expert, false, titan, true,  4000, 6000},
	SkillCase{"archeryIgnoredInMelee",      "archery", expert, false, titan, false, 4000, 6000}
),
	[](const ::testing::TestParamInfo<SkillCase> & info) { return info.param.name; });

// ---- luck, death blow and the rest of the attack flags ------------------------------------------

namespace
{

struct FlagCase
{
	const char * name;
	bool lucky;
	bool deathBlow;
	bool unlucky;
	int64_t expected;
};

}

class AttackFlagDamageTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<FlagCase>
{
};

TEST_P(AttackFlagDamageTest, scalesDamage)
{
	const auto & scenario = GetParam();

	BattleAttackInfo info(attacker(angel), defender(angel), 0, false);
	info.luckyStrike = scenario.lucky;
	info.deathBlow = scenario.deathBlow;
	info.unluckyStrike = scenario.unlucky;

	EXPECT_EQ(battle()->calculateDmgRange(info).damage.min, scenario.expected) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, AttackFlagDamageTest, ::testing::Values(
	FlagCase{"plain",      false, false, false, 5000},
	FlagCase{"lucky",      true,  false, false, 10000},
	FlagCase{"deathBlow",  false, true,  false, 10000},
	FlagCase{"unlucky",    false, false, true,  2500},
	// luck adds to the attack factors and misfortune multiplies into the defense ones, so the two
	// are worked out apart from each other - here they happen to land back on the plain damage
	FlagCase{"luckyAndUnlucky", true, false, true, 5000}
),
	[](const ::testing::TestParamInfo<FlagCase> & info) { return info.param.name; });

// ---- jousting ----------------------------------------------------------------------------------

/// Champions gain 5% per hex charged, and black knights have exactly as much defense as a champion
/// has attack, so nothing else moves the number.
TEST_F(DamageCalculatorTest, JoustingScalesWithChargeDistance)
{
	auto result = estimate(attacker(champion), defender(blackKnight), 4);

	EXPECT_EQ(result.damage.min, 2400);
	EXPECT_EQ(result.damage.max, 3000);
}

TEST_F(DamageCalculatorTest, JoustingNeedsAChargeToApply)
{
	auto result = estimate(attacker(champion), defender(blackKnight), 0);

	EXPECT_EQ(result.damage.min, 2000);
	EXPECT_EQ(result.damage.max, 2500);
}

/// Pikemen are immune to a cavalry charge, so the same charge buys the champion nothing. Their
/// 5 defense against 16 attack is what the rest of the number is.
TEST_F(DamageCalculatorTest, ChargeImmunityBlocksJousting)
{
	const auto * source = attacker(champion);
	const auto * target = defender(pikeman);

	EXPECT_EQ(estimate(source, target, 4).damage.min, 3100);
	EXPECT_EQ(estimate(source, target, 4).damage.max, 3875);
}

// ---- hatred --------------------------------------------------------------------------------------

/// Archangels hate arch devils for half again as much damage, on top of their 2 points of attack
/// advantage over them.
TEST_F(DamageCalculatorTest, HateAddsDamageAgainstTheHatedCreature)
{
	EXPECT_EQ(estimate(attacker(archangel), defender(archDevil)).damage.min, 8000);
}

TEST_F(DamageCalculatorTest, HateIsIgnoredAgainstEveryOtherCreature)
{
	// 10 points of attack over an angel, and no hatred of it
	EXPECT_EQ(estimate(attacker(archangel), defender(angel)).damage.min, 7500);
}

/// Hating a trait rather than a creature has no carrier in Heroes 3, so the test creature hates
/// everything that shoots - which a sharpshooter does, and a horned demon does not.
TEST_F(DamageCalculatorTest, HatedTraitAddsDamageAgainstCreaturesCarryingIt)
{
	const auto * source = attacker("vcmi-test:testDamageTraitHater");

	EXPECT_EQ(estimate(source, defender(sharpshooter)).damage.min, 1500);
}

TEST_F(DamageCalculatorTest, HatedTraitIsIgnoredAgainstCreaturesWithoutIt)
{
	const auto * source = attacker("vcmi-test:testDamageTraitHater");

	EXPECT_EQ(estimate(source, defender(hornedDemon)).damage.min, 1000);
}

// ---- revenge ---------------------------------------------------------------------------------

TEST_F(DamageCalculatorTest, RevengeAddsNothingToAnUnharmedStack)
{
	EXPECT_EQ(estimate(attacker("vcmi-test:testDamageRevenge"), defender(hornedDemon)).damage.min, 1000);
}

TEST_F(DamageCalculatorTest, RevengeGrowsAsTheStackIsWornDown)
{
	auto * source = attacker("vcmi-test:testDamageRevenge");

	int64_t inflicted = 75 * 100; // 100 creatures of 100 health down to 25
	source->damage(inflicted);
	ASSERT_EQ(source->getCount(), 25);

	// sqrt(101 * 100 / (2500 + 100) - 1) = +169.84%, over a base of 25 * 10
	EXPECT_EQ(estimate(source, defender(hornedDemon)).damage.min, 674);
}

// ---- attacking from behind ---------------------------------------------------------------------

TEST_F(DamageCalculatorTest, VulnerableFromBackAppliesWhenTheAttackerHasToTurnAround)
{
	// the attacker stands on the far side of its target, so it has to reverse in order to strike
	const auto * source = attacker(hornedDemon, stackSize, BattleHex(behindHex));
	const auto * target = defender("vcmi-test:testDamageVulnerableBack");

	auto result = estimate(source, target);

	EXPECT_EQ(result.damage.min, 1050);
	EXPECT_EQ(result.damage.max, 1350);
}

TEST_F(DamageCalculatorTest, VulnerableFromBackIsIgnoredFromTheFront)
{
	const auto * source = attacker(hornedDemon);
	const auto * target = defender("vcmi-test:testDamageVulnerableBack");

	auto result = estimate(source, target);

	EXPECT_EQ(result.damage.min, 700);
	EXPECT_EQ(result.damage.max, 900);
}

// ---- damage reduction --------------------------------------------------------------------------

namespace
{

/// Shield and Air Shield reduce damage through the same bonus and differ only in the kind of attack
/// their subtype answers. Both are cast for real, so how much they take off is whatever their own
/// configuration says at the mastery they were cast at.
struct ReductionCase
{
	const char * name;
	SpellID spell;
	int mastery;
	bool shooting;
	int64_t expected;
};

}

class DamageReductionTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<ReductionCase>
{
};

TEST_P(DamageReductionTest, reducesDamageOfTheMatchingAttack)
{
	const auto & scenario = GetParam();

	// titans shoot and ignore the melee penalty, so the same pair works for both kinds of attack
	const auto * target = defender(titan);

	// both spells are for the caster's own side, so the shielded stack is shielded by its own hero
	cast(defenderSideHero, scenario.spell, target, scenario.mastery);

	EXPECT_EQ(estimate(attacker(titan), target, 0, scenario.shooting).damage.min, scenario.expected) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, DamageReductionTest, ::testing::Values(
	// 100 titans deal 4000 before any reduction. Shield takes off 15% and 30% at expert mastery
	ReductionCase{"shieldAgainstMelee",       SpellID::SHIELD,     MasteryLevel::NONE,   false, 3400},
	ReductionCase{"expertShieldAgainstMelee", SpellID::SHIELD,     MasteryLevel::EXPERT, false, 2800},
	ReductionCase{"shieldIgnoredWhenShot",    SpellID::SHIELD,     MasteryLevel::EXPERT, true,  4000},

	// and Air Shield 25% and 50%
	ReductionCase{"airShieldAgainstShots",       SpellID::AIR_SHIELD, MasteryLevel::NONE,   true,  3000},
	ReductionCase{"expertAirShieldAgainstShots", SpellID::AIR_SHIELD, MasteryLevel::EXPERT, true,  2000},
	ReductionCase{"airShieldIgnoredInMelee",     SpellID::AIR_SHIELD, MasteryLevel::EXPERT, false, 4000}
),
	[](const ::testing::TestParamInfo<ReductionCase> & info) { return info.param.name; });

/// The stone gaze of a medusa reduces every kind of damage, and comes from a spell rather than from
/// an ability. That is what tells it apart from armorer, which reads the same bonus type and would
/// leave 1000 rather than 2000 were the two counted together.
TEST_F(DamageCalculatorTest, PetrificationReducesDamageWithoutCountingAsArmorer)
{
	const auto * target = defender(titan);
	cast(attackerSideHero, SpellID::STONE_GAZE, target);

	EXPECT_EQ(estimate(attacker(titan), target).damage.min, 2000);
}

/// Blindness lowers the attack of whoever is under it, whatever they strike.
TEST_F(DamageCalculatorTest, BlindnessLowersTheDamageOfItsBearer)
{
	const auto * source = attacker(angel);

	// a hostile spell, so it is the other hero that casts it
	cast(defenderSideHero, SpellID::BLIND, source, MasteryLevel::BASIC);

	EXPECT_EQ(estimate(source, defender(angel)).damage.min, 2500);
}

/// A reduction that swallows the whole attack still leaves the one point of damage every hit deals.
/// No spell reduces damage that far, so the bonus is granted directly.
TEST_F(DamageCalculatorTest, DamageNeverDropsBelowOne)
{
	auto * target = defender(titan);
	grant(target, BonusType::GENERAL_DAMAGE_REDUCTION, 100, BonusSubtypeID(BonusCustomSubtype::damageTypeMelee));

	EXPECT_EQ(estimate(attacker(titan), target).damage.min, 1);
}

/// A reduction limited to ranged combat has no business applying to a melee attack.
TEST_F(DamageCalculatorTest, AttackReductionRespectsItsCombatLimiter)
{
	auto * source = attacker(angel);
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::GENERAL_ATTACK_REDUCTION, BonusSource::OTHER, 50, BonusSourceID());
	bonus->effectRange = BonusLimitEffect::ONLY_DISTANCE_FIGHT;
	source->addNewBonus(bonus);

	EXPECT_EQ(estimate(source, defender(angel)).damage.min, 5000);
}

/// Forgetfulness only ever bites a shot. Medusas carry it well: they shoot, they ignore the melee
/// penalty that would otherwise muddy the melee half of this, and unlike titans they are not immune
/// to mind spells, which the spell refuses to be cast on.
TEST_F(DamageCalculatorTest, ForgetfulnessHalvesRangedDamage)
{
	const auto * source = attacker(medusa);
	cast(defenderSideHero, SpellID::FORGETFULNESS, source);

	EXPECT_EQ(estimate(source, defender(medusa), 0, true).damage.min, 300);
}

TEST_F(DamageCalculatorTest, ForgetfulnessIsIgnoredInMelee)
{
	const auto * source = attacker(medusa);
	cast(defenderSideHero, SpellID::FORGETFULNESS, source);

	EXPECT_EQ(estimate(source, defender(medusa)).damage.min, 600);
}

// ---- range penalties -----------------------------------------------------------------------------

namespace
{

struct RangeCase
{
	const char * name;
	const char * shooter;
	const char * target;
	bool shooting;
	bool distant;
	int64_t expectedMin;
	int64_t expectedMax;
};

}

class RangePenaltyDamageTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<RangeCase>
{
};

TEST_P(RangePenaltyDamageTest, halvesPenalisedAttacks)
{
	const auto & scenario = GetParam();

	const auto * source = attacker(scenario.shooter, stackSize, BattleHex(scenario.distant ? nearEdgeHex : attackerHex));
	const auto * target = defender(scenario.target, stackSize, BattleHex(scenario.distant ? farEdgeHex : defenderHex));

	auto result = estimate(source, target, 0, scenario.shooting);

	EXPECT_EQ(result.damage.min, scenario.expectedMin) << scenario.name;
	EXPECT_EQ(result.damage.max, scenario.expectedMax) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, RangePenaltyDamageTest, ::testing::Values(
	// monks have as much attack as a crusader has defense, and shoot for 1000..1200 unpenalised
	RangeCase{"monkShootsPointBlank", monk, crusader, true,  false, 1000, 1200},
	RangeCase{"monkShootsFromAfar",   monk, crusader, true,  true,   500,  600},
	RangeCase{"monkStruckInMelee",    monk, crusader, false, false,  500,  600},

	// sharpshooters carry their own answer to the distance penalty
	RangeCase{"sharpshooterShootsFromAfar", sharpshooter, crusader, true, true, 800, 1000},

	// titans ignore the melee penalty instead
	RangeCase{"titanStrikesInMelee", titan, titan, false, false, 4000, 6000}
),
	[](const ::testing::TestParamInfo<RangeCase> & info) { return info.param.name; });

// ---- the elementals that halve their own damage ----------------------------------------------------

/// Magic elementals deal half damage to anything immune to spells of level five, which black
/// dragons are and angels are not.
TEST_F(DamageCalculatorTest, MagicElementalHalvesDamageAgainstHighLevelSpellImmunity)
{
	auto result = estimate(attacker(magicElemental), defender(blackDragon));

	EXPECT_EQ(result.damage.min, 562);
	EXPECT_EQ(result.damage.max, 937);
}

TEST_F(DamageCalculatorTest, MagicElementalDealsFullDamageToEverythingElse)
{
	auto result = estimate(attacker(magicElemental), defender(angel));

	EXPECT_EQ(result.damage.min, 1312);
	EXPECT_EQ(result.damage.max, 2187);
}

/// Psychic elementals do the same against anything immune to mind spells, such as giants.
TEST_F(DamageCalculatorTest, PsychicElementalHalvesDamageAgainstMindImmunity)
{
	auto result = estimate(attacker(psychicElemental), defender(giant));

	EXPECT_EQ(result.damage.min, 487);
	EXPECT_EQ(result.damage.max, 975);
}

TEST_F(DamageCalculatorTest, PsychicElementalDealsFullDamageToEverythingElse)
{
	auto result = estimate(attacker(psychicElemental), defender(angel));

	EXPECT_EQ(result.damage.min, 875);
	EXPECT_EQ(result.damage.max, 1750);
}

// ---- the ballista and its artillery bonus ------------------------------------------------------

/// Artillery doubles what a ballista deals on the shots that roll for it. Its base damage also
/// scales with the attack of its hero, which is zero here, so 100 ballistae shoot for 200..300.
TEST_F(DamageCalculatorTest, ArtilleryDoublesTheDamageOfADoubleDamageShot)
{
	setSkill(attackerSideHero, "artillery", expert);

	BattleAttackInfo info(attacker(ballista), defender(hornedDemon), 0, true);
	info.doubleDamage = true;

	auto result = battle()->calculateDmgRange(info);

	EXPECT_EQ(result.damage.min, 400);
	EXPECT_EQ(result.damage.max, 600);
}

TEST_F(DamageCalculatorTest, ArtilleryIsIgnoredOnAnOrdinaryShot)
{
	setSkill(attackerSideHero, "artillery", expert);

	auto result = estimate(attacker(ballista), defender(hornedDemon), 0, true);

	EXPECT_EQ(result.damage.min, 200);
	EXPECT_EQ(result.damage.max, 300);
}

// ---- base damage: bless and curse ---------------------------------------------------------------

TEST_F(DamageCalculatorTest, BlessCollapsesTheRangeOntoItsMaximum)
{
	const auto * source = attacker(titan);
	cast(attackerSideHero, SpellID(SpellID::BLESS), source);

	auto result = estimate(source, defender(titan));

	EXPECT_EQ(result.damage.min, 6000);
	EXPECT_EQ(result.damage.max, 6000);
}

TEST_F(DamageCalculatorTest, CurseCollapsesTheRangeOntoItsMinimum)
{
	const auto * source = attacker(titan);

	// a hostile spell, so it is the other hero that casts it
	cast(defenderSideHero, SpellID(SpellID::CURSE), source);

	auto result = estimate(source, defender(titan));

	EXPECT_EQ(result.damage.min, 4000);
	EXPECT_EQ(result.damage.max, 4000);
}

/// From advanced mastery the spell shifts the whole range by a point before collapsing it, which is
/// what makes an expert blessing worth more than a plain one.
TEST_F(DamageCalculatorTest, ExpertBlessAddsAPointBeforeCollapsing)
{
	const auto * source = attacker(titan);
	cast(attackerSideHero, SpellID(SpellID::BLESS), source, MasteryLevel::EXPERT); // 40..60 becomes 41..61

	auto result = estimate(source, defender(titan));

	EXPECT_EQ(result.damage.min, 6100);
	EXPECT_EQ(result.damage.max, 6100);
}

TEST_F(DamageCalculatorTest, ExpertCurseTakesAPointBeforeCollapsing)
{
	const auto * source = attacker(titan);
	cast(defenderSideHero, SpellID(SpellID::CURSE), source, MasteryLevel::EXPERT); // 40..60 becomes 39..59

	auto result = estimate(source, defender(titan));

	EXPECT_EQ(result.damage.min, 3900);
	EXPECT_EQ(result.damage.max, 3900);
}

// ---- damage cap ----------------------------------------------------------------------------------

TEST_F(DamageCalculatorTest, DamageCapLimitsBothDamageAndCasualties)
{
	// capped at 10% of one creature's 100 health, so a killing blow becomes a scratch
	auto result = estimate(attacker(hornedDemon), defender("vcmi-test:testDamageCapped"));

	EXPECT_EQ(result.damage.min, 10);
	EXPECT_EQ(result.damage.max, 10);
	EXPECT_EQ(result.kills.min, 0);
	EXPECT_EQ(result.kills.max, 0);
}

// ---- casualties ------------------------------------------------------------------------------------

namespace
{

struct CasualtyCase
{
	const char * name;
	int32_t attackerCount;
	int32_t defenderCount;
	int64_t woundTopCreature; ///< health taken off the defender before the attack
	int64_t expectedDamage;
	int32_t expectedKills;
};

}

class CasualtiesTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<CasualtyCase>
{
};

TEST_P(CasualtiesTest, countsWholeCreaturesOnly)
{
	const auto & scenario = GetParam();

	const auto * source = attacker(angel, scenario.attackerCount);
	auto * target = defender(angel, scenario.defenderCount);

	if(scenario.woundTopCreature > 0)
	{
		int64_t inflicted = scenario.woundTopCreature;
		target->damage(inflicted);
	}

	auto result = estimate(source, target);

	EXPECT_EQ(result.damage.min, scenario.expectedDamage) << scenario.name;
	EXPECT_EQ(result.kills.min, scenario.expectedKills) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, CasualtiesTest, ::testing::Values(
	// angels have 200 health each and deal a flat 50 damage
	CasualtyCase{"tooLittleToKillAnything",   3,    100, 0,   150,   0},
	CasualtyCase{"exactlyOneCreature",        4,    100, 0,   200,   1},
	CasualtyCase{"remainderIsNotACasualty",   5,    100, 0,   250,   1},
	CasualtyCase{"woundedTopCreatureFallsFirst", 7, 100, 100, 350,   2},
	CasualtyCase{"cannotExceedTheStackAttacked", 1000, 10, 0, 50000, 10}
),
	[](const ::testing::TestParamInfo<CasualtyCase> & info) { return info.param.name; });

// ---- air shield ------------------------------------------------------------------------------------

/// Air Shield costs the shooter what its own damage reduction says and nothing more. It used to
/// impose the distance penalty on top of that, off a check that compared the value of the bonus - a
/// percentage - against MasteryLevel::ADVANCED, so every Air Shield of every mastery halved the shot
/// a second time.
TEST_F(DamageCalculatorTest, AirShieldOnlyReducesDamageByItsOwnValue)
{
	const auto * target = defender(titan);
	cast(defenderSideHero, SpellID::AIR_SHIELD, target, MasteryLevel::EXPERT);

	// the two stacks stand next to each other, so nothing but the spell can penalise the shot
	EXPECT_EQ(estimate(attacker(titan), target, 0, true).damage.min, 2000);
}

/// A shot from across the field is penalised the once, whether or not the target is shielded.
TEST_F(DamageCalculatorTest, AirShieldDoesNotDeepenTheDistancePenalty)
{
	const auto * source = attacker(titan, stackSize, BattleHex(nearEdgeHex));
	const auto * target = defender(titan, stackSize, BattleHex(farEdgeHex));
	cast(defenderSideHero, SpellID::AIR_SHIELD, target, MasteryLevel::EXPERT);

	EXPECT_EQ(estimate(source, target, 0, true).damage.min, 1000);
}

// ---- frenzy ------------------------------------------------------------------------------------

/// Frenzy trades the defense of its bearer for attack. A behemoth makes whatever it fights ignore
/// part of its own defense, so there is that much less to trade: 20 defense less the 9 a behemoth
/// takes off leaves 11 to convert, for 31 attack against 17 defense.
TEST_F(DamageCalculatorTest, FrenzyBoostIsBuiltOnTheDefenceABehemothLeaves)
{
	const auto * source = attacker(angel);
	cast(attackerSideHero, SpellID(SpellID::FRENZY), source);

	EXPECT_EQ(estimate(source, defender(behemoth)).damage.min, 8500);
}

/// An ancient behemoth takes off 17, leaving 3 to convert, for 23 attack against 19 defense.
TEST_F(DamageCalculatorTest, FrenzyBoostIsBuiltOnTheDefenceAnAncientBehemothLeaves)
{
	const auto * source = attacker(angel);
	cast(attackerSideHero, SpellID(SpellID::FRENZY), source);

	EXPECT_EQ(estimate(source, defender(ancientBehemoth)).damage.min, 6000);
}

/// Against anything without such an ability the whole defense is converted.
TEST_F(DamageCalculatorTest, FrenzyConvertsTheWholeDefenceIntoAttack)
{
	const auto * source = attacker(angel);
	cast(attackerSideHero, SpellID(SpellID::FRENZY), source); // 20 attack plus 20 defense against 20

	EXPECT_EQ(estimate(source, defender(angel)).damage.min, 10000);
}

/// An advanced frenzy converts half again as much, for 50 attack against 20 defense.
TEST_F(DamageCalculatorTest, AdvancedFrenzyConvertsHalfAgainAsMuchDefence)
{
	const auto * source = attacker(angel);
	cast(attackerSideHero, SpellID(SpellID::FRENZY), source, MasteryLevel::ADVANCED);

	EXPECT_EQ(estimate(source, defender(angel)).damage.min, 12500);
}

/// And the unit that spent its defense has none left when it is the one being hit.
TEST_F(DamageCalculatorTest, FrenzyLeavesItsBearerWithNoDefence)
{
	const auto * target = defender(angel);
	cast(defenderSideHero, SpellID(SpellID::FRENZY), target);

	// 20 attack against no defense at all is worth +100%
	EXPECT_EQ(estimate(attacker(angel), target).damage.min, 10000);
}

// ---- slayer --------------------------------------------------------------------------------------

namespace
{

struct SlayerCase
{
	const char * name;
	int mastery;
	const char * target;
	int64_t expected;
};

}

class SlayerDamageTest : public DamageCalculatorTestBase, public ::testing::WithParamInterface<SlayerCase>
{
};

TEST_P(SlayerDamageTest, appliesOnlyWithinItsMastery)
{
	const auto & scenario = GetParam();

	const auto * source = attacker(angel);
	cast(attackerSideHero, SpellID(SpellID::SLAYER), source, scenario.mastery);

	EXPECT_EQ(estimate(source, defender(scenario.target)).damage.min, scenario.expected) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Scenarios, SlayerDamageTest, ::testing::Values(
	// angels are kings of the second level, and the 8 points of attack the spell grants are worth
	// +40% over their 20 defense - but only from the mastery that reaches a king that high
	SlayerCase{"masteryBelowTheKing",  MasteryLevel::NONE,     angel, 5000},
	SlayerCase{"basicIsStillTooLow",   MasteryLevel::BASIC,    angel, 5000},
	SlayerCase{"masteryCoversTheKing", MasteryLevel::ADVANCED, angel, 7000},
	SlayerCase{"masteryAboveTheKing",  MasteryLevel::EXPERT,   angel, 7000},
	// a demon is no king at all, so the spell buys nothing - the number is the attack advantage
	// of 20 over its 10 defense
	SlayerCase{"targetIsNoKing",       MasteryLevel::EXPERT,   demon, 7500}
),
	[](const ::testing::TestParamInfo<SlayerCase> & info) { return info.param.name; });

// ---- sign of a factor ---------------------------------------------------------------------------

/// A factor is read by its sign rather than by where it came from, so a mod that gives a
/// damage-raising bonus a negative value gets a mitigating factor out of it - and mitigation
/// multiplies rather than eating into the other boosts. Granted directly because nothing in the
/// game ships a bonus with such a value.
TEST_F(DamageCalculatorTest, ABoostTurnedNegativeMitigatesInsteadOfCancellingOtherBoosts)
{
	auto * source = attacker(hornedDemon);
	grant(source, BonusType::GENERAL_DAMAGE_PREMY, 100);
	grant(source, BonusType::PERCENTAGE_DAMAGE_BOOST, -25, BonusSubtypeID(BonusCustomSubtype::damageTypeMelee));

	// 700 doubled and then cut by a quarter. Were the two summed instead, this would be 1225
	auto result = estimate(source, defender(hornedDemon));

	EXPECT_EQ(result.damage.min, 1050);
	EXPECT_EQ(result.damage.max, 1350);
}
