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

#include "../../lib/CCreatureHandler.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CUnitState.h"

#include "mock/mock_BonusBearer.h"
#include "mock/mock_UnitEnvironment.h"
#include "mock/mock_UnitInfo.h"

namespace test
{
using namespace ::testing;

class DamageCalculatorTest : public Test
{
protected:
	class TestCallback : public CBattleInfoCallback
	{
	public:
		const IBattleInfo * getBattle() const override
		{
			return nullptr;
		}

		std::optional<PlayerColor> getPlayerID() const override
		{
			return std::nullopt;
		}
	};

	NiceMock<UnitInfoMock> attackerInfo;
	NiceMock<UnitInfoMock> defenderInfo;
	NiceMock<UnitEnvironmentMock> environment;
	BonusBearerMock attackerBonuses;
	BonusBearerMock defenderBonuses;
	battle::CUnitStateDetached attacker;
	battle::CUnitStateDetached defender;
	TestCallback callback;

	DamageCalculatorTest() : attacker(&attackerInfo, &attackerBonuses), defender(&defenderInfo, &defenderBonuses) {}

	void addBonus(
		BonusBearerMock & bearer,
		BonusType type,
		int value,
		BonusSubtypeID subtype = BonusSubtypeID(),
		BonusLimitEffect effectRange = BonusLimitEffect::NO_LIMIT
	)
	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, type, BonusSource::CREATURE_ABILITY, value, BonusSourceID(), subtype);
		bonus->effectRange = effectRange;
		bearer.addNewBonus(bonus);
	}

	void setupAttacker(int count, int damage, int attackValue, int defenseValue, int frenzy)
	{
		ON_CALL(attackerInfo, unitBaseAmount()).WillByDefault(Return(count));
		ON_CALL(attackerInfo, unitType()).WillByDefault(Return(CreatureID(0).toCreature()));

		addBonus(attackerBonuses, BonusType::STACK_HEALTH, 100);
		addBonus(attackerBonuses, BonusType::CREATURE_DAMAGE, damage, BonusCustomSubtype::creatureDamageBoth);
		addBonus(attackerBonuses, BonusType::PRIMARY_SKILL, attackValue, BonusSubtypeID(PrimarySkill::ATTACK));
		addBonus(attackerBonuses, BonusType::PRIMARY_SKILL, defenseValue, BonusSubtypeID(PrimarySkill::DEFENSE));
		if(frenzy != 0)
			addBonus(attackerBonuses, BonusType::IN_FRENZY, frenzy);

		attacker.localInit(&environment);
	}

	void setupDefender(int count, int health, int defenseValue, int reduction = 0, BonusLimitEffect effectRange = BonusLimitEffect::NO_LIMIT)
	{
		ON_CALL(defenderInfo, unitBaseAmount()).WillByDefault(Return(count));
		ON_CALL(defenderInfo, unitType()).WillByDefault(Return(CreatureID(0).toCreature()));

		addBonus(defenderBonuses, BonusType::STACK_HEALTH, health);
		addBonus(defenderBonuses, BonusType::PRIMARY_SKILL, defenseValue, BonusSubtypeID(PrimarySkill::DEFENSE));
		if(reduction != 0)
			addBonus(defenderBonuses, BonusType::ENEMY_DEFENCE_REDUCTION, reduction, BonusSubtypeID(), effectRange);

		defender.localInit(&environment);
	}

	DamageEstimation calculateDamage(bool shooting = false, bool ignoreDefenseFactors = false)
	{
		BattleAttackInfo attackInfo(&attacker, &defender, 0, shooting);
		attackInfo.ignoreDefenseFactors = ignoreDefenseFactors;
		return callback.calculateDmgRange(attackInfo);
	}
};

TEST_F(DamageCalculatorTest, frenzyUsesDefenseAfterAncientBehemothReduction)
{
	setupAttacker(1000, 30, 16, 13, 200);
	setupDefender(1000, 300, 19, 80);

	auto result = calculateDamage();

	EXPECT_EQ(result.damage.min, 31500);
	EXPECT_EQ(result.damage.max, 31500);
}

class FrenzyDefenseReductionTest
	: public DamageCalculatorTest
	, public WithParamInterface<std::pair<int, int64_t>>
{
};

TEST_P(FrenzyDefenseReductionTest, keepsExistingRounding)
{
	setupAttacker(1, 101, 0, 20, 100);
	setupDefender(1, 1000, 0, GetParam().first);

	auto result = calculateDamage();

	EXPECT_EQ(result.damage.min, GetParam().second);
	EXPECT_EQ(result.damage.max, GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(
	BehemothValues,
	FrenzyDefenseReductionTest,
	Values(std::pair<int, int64_t>{40, 156}, std::pair<int, int64_t>{80, 116}, std::pair<int, int64_t>{100, 101})
);

TEST_F(DamageCalculatorTest, defenseReductionDoesNotChangeAttackWithoutFrenzy)
{
	setupAttacker(1, 101, 10, 20, 0);
	setupDefender(1, 1000, 0, 80);

	auto result = calculateDamage();

	EXPECT_EQ(result.damage.min, 151);
	EXPECT_EQ(result.damage.max, 151);
}

TEST_F(DamageCalculatorTest, frenzyRoundsAfterDefenseReduction)
{
	setupAttacker(1, 101, 0, 2, 150);
	setupDefender(1, 1000, 0, 40);

	auto result = calculateDamage();

	EXPECT_EQ(result.damage.min, 106);
	EXPECT_EQ(result.damage.max, 106);
}

TEST_F(DamageCalculatorTest, normalDefenseReductionKeepsExactProductRounding)
{
	setupAttacker(1, 101, 0, 0, 0);
	setupDefender(1, 1000, 25);
	addBonus(attackerBonuses, BonusType::ENEMY_DEFENCE_REDUCTION, 80);

	auto result = calculateDamage();

	EXPECT_EQ(result.damage.min, 90);
	EXPECT_EQ(result.damage.max, 90);
}

TEST_F(DamageCalculatorTest, defenseReductionRespectsAttackRange)
{
	setupAttacker(1, 101, 0, 20, 100);
	setupDefender(1, 1000, 0, 80, BonusLimitEffect::ONLY_DISTANCE_FIGHT);

	auto meleeResult = calculateDamage(false, true);
	auto rangedResult = calculateDamage(true, true);

	EXPECT_EQ(meleeResult.damage.min, 202);
	EXPECT_EQ(rangedResult.damage.min, 116);
}

}
