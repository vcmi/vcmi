/*
 * SpellTargetStepTest.cpp, part of VCMI engine
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
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"

namespace
{
// creatures
constexpr int pikeman = 0;

/// Cursors of the destinations a hero chooses for a spell, depending on the aim types of the spell at the hero's mastery.
struct StepCase
{
	const char * name;
	SpellID spell;
	int mastery; ///< of every magic school
	std::vector<std::string> cursors; ///< one per destination; none for a spell cast without a target
};

}

/// What a spell shows while the player chooses its destinations one by one.
class SpellTargetStepTest : public BattleTestFixture
{
public:
	static inline const BattleHex unitHex = BattleHex(3, 5);
	static inline const BattleHex freeHex = BattleHex(8, 5);

	void setMastery(int mastery)
	{
		for(auto school : {SecondarySkill::FIRE_MAGIC, SecondarySkill::AIR_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC})
			attackerSideHero->setSecSkillLevel(school, mastery, ChangeValueMode::ABSOLUTE);
	}

	spells::TargetStepDescription describe(SpellID spell, const battle::Target & partialTarget, const BattleHex & hoveredHex) const
	{
		const spells::BattleCast cast(battle(), attackerSideHero, spells::Mode::HERO, spell.toSpell());
		return spell.toSpell()->battleMechanics(&cast)->describeTargetStep(partialTarget, hoveredHex);
	}

	static std::string toString(const MetaString & text)
	{
		return text.toString(LIBRARY->staticTexts());
	}
};

class SpellStepCursorTest : public SpellTargetStepTest, public ::testing::WithParamInterface<StepCase>
{
};

TEST_P(SpellStepCursorTest, showsCursorOfEachDestination)
{
	const auto & scenario = GetParam();

	startGame();
	setMastery(scenario.mastery);
	startBattle();

	addStack(BattleSide::ATTACKER, CreatureID(pikeman), unitHex, 10);

	battle::Target partialTarget;
	for(const auto & cursor : scenario.cursors)
	{
		EXPECT_EQ(describe(scenario.spell, partialTarget, unitHex).cursor, cursor) << scenario.name;
		partialTarget.emplace_back(unitHex);
	}

	// nothing is left to choose
	EXPECT_EQ(describe(scenario.spell, partialTarget, unitHex).cursor, "") << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Spells, SpellStepCursorTest, ::testing::Values(
	StepCase{"unit",             SpellID::BLESS,           0, {"castSpell"}},
	StepCase{"everyUnit",        SpellID::BLESS,           3, {}},
	StepCase{"nothing",          SpellID::ARMAGEDDON,      0, {}},
	StepCase{"anyHex",           SpellID::FIREBALL,        0, {"castSpell"}},
	StepCase{"freeHexes",        SpellID::FIRE_WALL,       0, {"castSpell"}},
	// the OBSTACLE aim type is reported as LOCATION
	StepCase{"obstacleAsAnyHex", SpellID::REMOVE_OBSTACLE, 0, {"castSpell"}},
	StepCase{"unitThenHex",      SpellID::TELEPORT,        0, {"castSpell", "combatTeleport"}},
	StepCase{"deadThenLiveUnit", SpellID::SACRIFICE,       0, {"castSpell", "combatSacrifice"}}
),
	[](const ::testing::TestParamInfo<StepCase> & info) { return info.param.name; });

TEST_F(SpellTargetStepTest, unitSpellNamesSpellAndUnit)
{
	startGame();
	startBattle();

	addStack(BattleSide::ATTACKER, CreatureID(pikeman), unitHex, 10);

	const auto shown = describe(SpellID::BLESS, {}, unitHex);
	EXPECT_EQ(toString(shown.text), "Cast Bless on Pikemen");
	EXPECT_FALSE(shown.blockedText.empty());
}

TEST_F(SpellTargetStepTest, damageSpellShowsDamageAndKills)
{
	startGame();
	startBattle();

	addStack(BattleSide::DEFENDER, CreatureID(pikeman), unitHex, 10);

	const auto shown = describe(SpellID::FIREBALL, {}, unitHex);
	EXPECT_THAT(toString(shown.text), ::testing::MatchesRegex("Cast Fireball \\([0-9]+ damage, [0-9]+ will perish\\)"));
	EXPECT_TRUE(shown.blockedText.empty());

	EXPECT_EQ(toString(describe(SpellID::FIREBALL, {}, freeHex).text), "Cast Fireball");
}

TEST_F(SpellTargetStepTest, spellThatNeedsFreeHexesNamesItselfWhereBlocked)
{
	startGame();
	startBattle();

	const auto shown = describe(SpellID::FIRE_WALL, {}, freeHex);
	EXPECT_EQ(toString(shown.text), "Cast Fire Wall");
	EXPECT_THAT(toString(shown.blockedText), ::testing::HasSubstr("Fire Wall"));
}

TEST_F(SpellTargetStepTest, secondDestinationHasItsOwnTexts)
{
	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(pikeman), unitHex, 10);
	CStack * victim = addStack(BattleSide::ATTACKER, CreatureID(pikeman), freeHex, 1);

	battle::Target chosen;
	chosen.emplace_back(unit);

	const auto teleport = describe(SpellID::TELEPORT, chosen, freeHex.cloneInDirection(BattleHex::RIGHT));
	EXPECT_FALSE(teleport.text.empty());
	EXPECT_FALSE(teleport.blockedText.empty());
	EXPECT_NE(toString(teleport.text), toString(teleport.blockedText));

	const auto sacrifice = describe(SpellID::SACRIFICE, chosen, victim->getPosition());
	EXPECT_THAT(toString(sacrifice.text), ::testing::HasSubstr("Pikeman"));
	EXPECT_FALSE(sacrifice.blockedText.empty());
}
