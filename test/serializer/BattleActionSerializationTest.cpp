/*
 * BattleActionSerializationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/battle/BattleAction.h"
#include "../../lib/serializer/CMemorySerializer.h"

TEST(BattleActionSerializationTest, actionOfPreviousVersionIsReadWithItsType)
{
	constexpr auto previousVersion = ESerializationVersion::VISITABLE_FROM_TOP_IN_OBJECT_TYPE;

	CMemorySerializer serializer;
	serializer.oser.version = previousVersion;
	serializer.iser.version = previousVersion;

	// layout of BattleAction before its type was an identifier
	BattleSide side = BattleSide::DEFENDER;
	ui32 stackNumber = 5;
	int32_t actionType = 9;
	SpellID spell = SpellID::NONE;
	std::vector<BattleAction::DestinationInfo> target(1);
	target[0].unitValue = -1000;
	target[0].hexValue = BattleHex(42);
	serializer.oser & side;
	serializer.oser & stackNumber;
	serializer.oser & actionType;
	serializer.oser & spell;
	serializer.oser & target;

	BattleAction action;
	serializer.iser & action;

	EXPECT_TRUE(serializer.isFullyRead());
	EXPECT_EQ(action.actionType, BattleActionID::RANGED_ATTACK);
	EXPECT_EQ(action.side, BattleSide::DEFENDER);
	EXPECT_EQ(action.stackNumber, 5u);
	ASSERT_EQ(action.target.size(), 1u);
	EXPECT_EQ(action.target[0].hexValue, BattleHex(42));
}

TEST(BattleActionSerializationTest, actionNamesItsTypeByIdentifier)
{
	BattleAction written;
	written.side = BattleSide::ATTACKER;
	written.stackNumber = 3;
	written.actionType = BattleActionID::ADJACENT_SPELL;

	CMemorySerializer serializer;
	serializer.oser & written;

	BattleSide side;
	ui32 stackNumber;
	std::string actionType;
	serializer.iser & side;
	serializer.iser & stackNumber;
	serializer.iser & actionType;
	EXPECT_EQ(actionType, "core:adjacentSpell");

	CMemorySerializer roundTrip;
	roundTrip.oser & written;
	BattleAction read;
	roundTrip.iser & read;
	EXPECT_EQ(read.actionType, BattleActionID::ADJACENT_SPELL);
}
