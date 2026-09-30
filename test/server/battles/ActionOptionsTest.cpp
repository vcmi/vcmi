/*
 * ActionOptionsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/PossiblePlayerBattleAction.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"

#include <vcmi/spells/Magic.h>

VCMI_LIB_NAMESPACE_BEGIN

/// Prints the value of PossiblePlayerBattleAction::Actions, which gtest would otherwise print as raw bytes.
static void PrintTo(const PossiblePlayerBattleAction & option, std::ostream * os)
{
	*os << "{action " << static_cast<int>(option.get()) << ", spell " << option.spell().getNum() << "}";
}

VCMI_LIB_NAMESPACE_END

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int archer = 2;
constexpr int masterGenie = 37;
constexpr int harpy = 72;
constexpr int stormElemental = 127; // casts Protection from Air

using Option = PossiblePlayerBattleAction;

/// Options of one kind of unit outside of the tactics phase, with nothing next to it.
struct UnitOptionsCase
{
	const char * name;
	int creature;
	/// Grants the abilities no Heroes 3 creature combines with the rest, or nullptr.
	void (*grant)(CStack * unit);
	/// Spells the client lets a SPELLCASTER unit choose from.
	std::vector<SpellID> spells;
	std::vector<Option> expected;
};

/// How the client aims a hero spell, depending on the aim types of the spell at the hero's mastery.
struct AimCase
{
	const char * name;
	SpellID spell;
	int mastery; ///< of every magic school
	Option::Actions expected;
};

}

/// Options the client offers for the active unit, which it tries in turn on the hovered hex.
class ActionOptionsTest : public BattleTestFixture
{
public:
	static inline const BattleHex unitHex = BattleHex(3, 5);

	std::vector<Option> optionsOf(const CStack * unit, const std::vector<SpellID> & spells = {}, bool tacticsMode = false) const
	{
		BattleClientInterfaceData data;
		data.creatureSpellsToCast = spells;
		data.tacticsMode = tacticsMode;
		return battle()->getClientActionsForStack(unit, data);
	}
};

class UnitOptionsTest : public ActionOptionsTest, public ::testing::WithParamInterface<UnitOptionsCase>
{
};

TEST_P(UnitOptionsTest, offersOptionsOfUnitAbilities)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(scenario.creature), unitHex, 10);
	if(scenario.grant)
		scenario.grant(unit);

	EXPECT_THAT(optionsOf(unit, scenario.spells), ::testing::UnorderedElementsAreArray(scenario.expected)) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Units, UnitOptionsTest, ::testing::Values(
	UnitOptionsCase{"melee", pikeman, nullptr, {},
		{Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"shooter", archer, nullptr, {},
		{Option::SHOOT, Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"returning", harpy, nullptr, {},
		{Option::ATTACK_AND_RETURN, Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"longWeapon", pikeman, [](CStack * unit)
		{
			unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));
		}, {},
		{Option::LONG_WEAPON_ATTACK, Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"spellcaster", stormElemental, nullptr, {SpellID::PROTECTION_FROM_AIR},
		{Option(Option::AIMED_SPELL_CREATURE, SpellID::PROTECTION_FROM_AIR), Option::SHOOT, Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"randomSpellcaster", masterGenie, nullptr, {},
		{Option::RANDOM_GENIE_SPELL, Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"adjacentSpellcaster", pikeman, [](CStack * unit)
		{
			BattleTestFixture::grantSpell(unit, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
		}, {},
		{Option(Option::WALK_AND_SPELLCAST, SpellID::BLESS), Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK}},
	UnitOptionsCase{"firstAidTent", CreatureID::FIRST_AID_TENT, nullptr, {},
		{Option::HEAL}},
	// the catapult has nothing to shoot at outside of a siege
	UnitOptionsCase{"catapultInField", CreatureID::CATAPULT, nullptr, {},
		{}}
),
	[](const ::testing::TestParamInfo<UnitOptionsCase> & info) { return info.param.name; });

TEST_F(ActionOptionsTest, shooterNextToEnemyCannotShoot)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), unitHex, 10);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), unitHex.cloneInDirection(BattleHex::RIGHT), 10);

	EXPECT_THAT(optionsOf(shooter), ::testing::UnorderedElementsAre(Option::ATTACK, Option::WALK_AND_ATTACK, Option::MOVE_STACK));
}

TEST_F(ActionOptionsTest, catapultShootsAtStandingWalls)
{
	startGame();
	startSiege();

	CStack * catapult = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), unitHex, 1);

	EXPECT_THAT(optionsOf(catapult), ::testing::UnorderedElementsAre(Option::CATAPULT));
}

TEST_F(ActionOptionsTest, tacticsPhaseOffersOnlyMovingAndChoosingUnits)
{
	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(archer), unitHex, 10);

	EXPECT_THAT(optionsOf(unit, {}, true), ::testing::UnorderedElementsAre(Option::MOVE_TACTICS, Option::CHOOSE_TACTICS_STACK));
}

class SpellAimTest : public ActionOptionsTest, public ::testing::WithParamInterface<AimCase>
{
};

TEST_P(SpellAimTest, aimsHeroSpellByItsTargets)
{
	const auto & scenario = GetParam();

	startGame();

	for(auto school : {SecondarySkill::FIRE_MAGIC, SecondarySkill::AIR_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC})
		attackerSideHero->setSecSkillLevel(school, scenario.mastery, ChangeValueMode::ABSOLUTE);

	startBattle();

	const auto aim = battle()->getCasterAction(scenario.spell.toSpell(), attackerSideHero, spells::Mode::HERO);
	EXPECT_EQ(aim, Option(scenario.expected, scenario.spell)) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Spells, SpellAimTest, ::testing::Values(
	AimCase{"unit",             SpellID::BLESS,           0, Option::AIMED_SPELL_CREATURE},
	AimCase{"everyUnit",        SpellID::BLESS,           3, Option::NO_LOCATION},
	AimCase{"nothing",          SpellID::ARMAGEDDON,      0, Option::NO_LOCATION},
	AimCase{"anyHex",           SpellID::FIREBALL,        0, Option::ANY_LOCATION},
	AimCase{"freeHexes",        SpellID::FIRE_WALL,       0, Option::FREE_LOCATION},
	// the OBSTACLE aim type is reported as LOCATION, so the OBSTACLE option is never offered
	AimCase{"obstacleAsAnyHex", SpellID::REMOVE_OBSTACLE, 0, Option::ANY_LOCATION},
	AimCase{"unitThenHex",      SpellID::TELEPORT,        0, Option::TELEPORT},
	AimCase{"deadThenLiveUnit", SpellID::SACRIFICE,       0, Option::SACRIFICE}
),
	[](const ::testing::TestParamInfo<AimCase> & info) { return info.param.name; });
