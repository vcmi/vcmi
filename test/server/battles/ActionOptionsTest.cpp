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

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/bonuses/Bonus.h"

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int archer = 2;
constexpr int masterGenie = 37;
constexpr int harpy = 72;
constexpr int stormElemental = 127; // casts Protection from Air
constexpr int cyclop = 94; // shoots at walls

/// Options of one kind of unit outside of the tactics phase, with nothing next to it.
struct UnitOptionsCase
{
	const char * name;
	int creature;
	/// Grants the abilities no Heroes 3 creature combines with the rest, or nullptr.
	void (*grant)(CStack * unit);
	/// Actions whose types offer an option
	std::vector<EActionType> actions;
};

}

/// Options the client offers for the active unit, which it tries in turn on the hovered hex.
class ActionOptionsTest : public BattleTestFixture
{
public:
	static inline const BattleHex unitHex = BattleHex(3, 5);

	/// Types of the options that the action types offer
	std::vector<const BattleActionType *> typesOffered(const CStack * unit) const
	{
		std::vector<const BattleActionType *> result;
		for(const auto & option : optionsOf(unit))
			result.push_back(option.type);
		return result;
	}

	static std::vector<const BattleActionType *> typesOf(const std::vector<EActionType> & actions)
	{
		std::vector<const BattleActionType *> result;
		for(const auto action : actions)
			result.push_back(BattleActionType::find(action));
		return result;
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

	EXPECT_THAT(typesOffered(unit), ::testing::UnorderedElementsAreArray(typesOf(scenario.actions))) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Units, UnitOptionsTest, ::testing::Values(
	UnitOptionsCase{"melee", pikeman, nullptr,
		{EActionType::WALK, EActionType::WALK_AND_ATTACK}},
	UnitOptionsCase{"shooter", archer, nullptr,
		{EActionType::WALK, EActionType::SHOOT, EActionType::WALK_AND_ATTACK}},
	UnitOptionsCase{"returning", harpy, nullptr,
		{EActionType::WALK, EActionType::WALK_AND_ATTACK, EActionType::WALK_AND_ATTACK_NO_RETURN}},
	UnitOptionsCase{"longWeapon", pikeman, [](CStack * unit)
		{
			unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));
		},
		{EActionType::WALK, EActionType::WALK_AND_ATTACK, EActionType::LONG_WEAPON_ATTACK}},
	UnitOptionsCase{"spellcaster", stormElemental, nullptr,
		{EActionType::WALK, EActionType::SHOOT, EActionType::WALK_AND_ATTACK, EActionType::MONSTER_SPELL}},
	UnitOptionsCase{"randomSpellcaster", masterGenie, nullptr,
		{EActionType::WALK, EActionType::WALK_AND_ATTACK, EActionType::RANDOM_SPELL}},
	UnitOptionsCase{"adjacentSpellcaster", pikeman, [](CStack * unit)
		{
			BattleTestFixture::grantSpell(unit, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
		},
		{EActionType::WALK, EActionType::WALK_AND_ATTACK, EActionType::WALK_AND_CAST}},
	UnitOptionsCase{"firstAidTent", CreatureID::FIRST_AID_TENT, nullptr,
		{EActionType::STACK_HEAL}},
	// walls to shoot at exist only in a siege
	UnitOptionsCase{"wallShooterInField", cyclop, nullptr,
		{EActionType::WALK, EActionType::SHOOT, EActionType::WALK_AND_ATTACK}}
),
	[](const ::testing::TestParamInfo<UnitOptionsCase> & info) { return info.param.name; });

TEST_F(ActionOptionsTest, shooterNextToEnemyCannotShoot)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), unitHex, 10);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), unitHex.cloneInDirection(BattleHex::RIGHT), 10);

	EXPECT_THAT(typesOffered(shooter), ::testing::UnorderedElementsAreArray(typesOf({EActionType::WALK, EActionType::WALK_AND_ATTACK})));
}

TEST_F(ActionOptionsTest, tacticsPhaseOffersOnlyMoveWithoutPanelButton)
{
	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(archer), unitHex, 10);
	battle()->tacticDistance = 4;
	battle()->tacticsSide = BattleSide::ATTACKER;

	const auto options = optionsOf(unit);
	ASSERT_EQ(options.size(), 1);
	EXPECT_EQ(options.front().type, BattleActionType::find(EActionType::WALK));
	EXPECT_FALSE(options.front().button.has_value());
}
