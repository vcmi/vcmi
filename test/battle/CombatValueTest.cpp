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

#include "../../lib/GameLibrary.h"
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

TEST(CombatValueTest, cachedWorthMatchesWhatTheModelAnswers)
{
	auto stack = makeStack(CreatureID::AZURE_DRAGON, 7);
	const auto expected = LIBRARY->combatValues->getAIValue(*stack, stack->getType()) * 7;

	// asked twice, so that the cached answer is checked as well as the computed one
	EXPECT_EQ(stack->estimateCombatValue(), static_cast<ui64>(expected));
	EXPECT_EQ(stack->estimateCombatValue(), static_cast<ui64>(expected));
}

TEST(CombatValueTest, cachedWorthFollowsBonusChanges)
{
	auto stack = makeStack(CreatureID::ARCHER, 10);
	const auto before = stack->estimateCombatValue();

	stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH,
		BonusSource::OTHER, 100, BonusSourceID()));

	EXPECT_GT(stack->estimateCombatValue(), before);
}
