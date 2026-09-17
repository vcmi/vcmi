/*
 * CombatValueTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_BonusBearer.h"
#include "mock/mock_UnitInfo.h"
#include "mock/mock_UnitEnvironment.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/battle/CombatValue.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/mapObjects/army/CStackInstance.h"

namespace
{

std::unique_ptr<CStackInstance> makeStack(const CreatureID & creature, TQuantity count)
{
	return std::make_unique<CStackInstance>(nullptr, creature, count, false);
}

}

TEST(CombatValueTest, stackIsWorthAsMuchAsTheCreaturesInIt)
{
	auto single = makeStack(CreatureID::ARCHER, 1);
	auto many = makeStack(CreatureID::ARCHER, 50);

	EXPECT_GT(single->estimateCombatValue(), 0u);
	EXPECT_EQ(many->estimateCombatValue(), single->estimateCombatValue() * 50);
}

TEST(CombatValueTest, stackWorthMatchesWhatTheModelAnswersForItsCreature)
{
	auto stack = makeStack(CreatureID::AZURE_DRAGON, 7);
	const auto expected = LIBRARY->combatValues->getAIValue(stack->getType()) * 7;

	EXPECT_EQ(stack->estimateCombatValue(), static_cast<ui64>(expected));
}

/// An army on the map is valued by its creature type, so that armies which exist and armies which
/// are only proposed - creatures about to be bought - stay comparable. Bonuses reaching one stack
/// and not another are only accounted for in battle, where every unit has a bearer to read.
TEST(CombatValueTest, mapWorthIgnoresBonusesOnTheStack)
{
	auto stack = makeStack(CreatureID::ARCHER, 10);
	const auto before = stack->estimateCombatValue();

	stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH,
		BonusSource::OTHER, 100, BonusSourceID()));

	EXPECT_EQ(stack->estimateCombatValue(), before);
}

namespace test
{
using namespace ::testing;

/// A battle unit of a real creature, so that the model has actual stats to read
class CombatValueUnitTest : public Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;
	battle::CUnitStateDetached subject;

	static constexpr int32_t count = 20;

	CombatValueUnitTest()
		: subject(&infoMock, &bonusMock)
	{
		const auto * archer = CreatureID(CreatureID::ARCHER).toCreature();

		// a detached unit carries no creature node, so the stats the model reads are supplied here
		const auto giveBonus = [this](BonusType type, int value, const BonusSubtypeID & subtype = {})
		{
			bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, type,
				BonusSource::CREATURE_ABILITY, value, BonusSourceID(), subtype));
		};

		giveBonus(BonusType::STACK_HEALTH, archer->getBaseHitPoints());
		giveBonus(BonusType::STACKS_SPEED, archer->getBaseSpeed());
		giveBonus(BonusType::PRIMARY_SKILL, archer->getBaseAttack(), BonusSubtypeID(PrimarySkill::ATTACK));
		giveBonus(BonusType::PRIMARY_SKILL, archer->getBaseDefense(), BonusSubtypeID(PrimarySkill::DEFENSE));
		giveBonus(BonusType::CREATURE_DAMAGE, archer->getBaseDamageMin(), BonusCustomSubtype::creatureDamageMin);
		giveBonus(BonusType::CREATURE_DAMAGE, archer->getBaseDamageMax(), BonusCustomSubtype::creatureDamageMax);

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(count));
		EXPECT_CALL(infoMock, unitType()).WillRepeatedly(Return(archer));
		EXPECT_CALL(envMock, unitHasAmmoCart(_)).WillRepeatedly(Return(false));

		subject.localInit(&envMock);
	}
};

TEST_F(CombatValueUnitTest, undamagedUnitIsWorthItsCreaturesTogether)
{
	const auto perCreature = LIBRARY->combatValues->getAIValue(subject, subject.unitType());

	EXPECT_GT(perCreature, 0);
	EXPECT_EQ(subject.estimateCombatValue(), static_cast<uint64_t>(perCreature * count));
}

TEST_F(CombatValueUnitTest, woundedUnitIsWorthLessThanAWholeOne)
{
	const auto whole = subject.estimateCombatValue();

	int64_t damage = subject.getMaxHealth() * 5;
	subject.damage(damage);

	ASSERT_EQ(subject.getCount(), count - 5);
	EXPECT_LT(subject.estimateCombatValue(), whole);
}

}
