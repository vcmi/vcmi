/*
 * CGCreatureJoiningTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../quest/QuestTest.h"
#include "../mock/TinyH3MBuilder.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/texts/CGeneralTextHandler.h"

// The join offer of a wandering monster must name how many creatures join
// when only part of the stack does.

namespace
{
const CreatureID pikeman(0);
const CreatureID archangel(13);
const int3 heroPos(5, 5, 0);
const int3 monsterPos(6, 5, 0);
constexpr int8_t compliant = 0;
}

class CGCreatureJoiningTest : public QuestTest
{
protected:
	CGCreature * startWithMonster(uint16_t count)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PlayerColor(0))
			.hero(heroPos, HeroTypeID(0), PlayerColor(0))
			.heroGarrison({{archangel, 5}})
			.monster(monsterPos, pikeman, count, compliant);
		startWithMap(std::move(builder));
		return findFirst<CGCreature>();
	}

	std::string offerText(CGCreature * monster)
	{
		visit(findHeroAt(heroPos), monster);
		const auto & dialogs = gameEvents().blockingDialogs;
		if(dialogs.empty())
			return {};
		return dialogs.back().text.toString(LIBRARY->staticTexts());
	}
};

TEST_F(CGCreatureJoiningTest, PartialJoinForGoldNamesJoiningAndTotalCountAndWholeStackPrice)
{
	// Half of a 10-pikeman stack that joins only for money: the offer names 5 of 10
	// and charges the price of the whole stack (10 * 60 gold).
	auto * monster = startWithMonster(10);
	ASSERT_NE(monster, nullptr);
	monster->joiningPercentage = 50;
	monster->joinOnlyForMoney = true;

	EXPECT_EQ(offerText(monster), "5 of the 10 Pikemen will join your army for the sum of 600 gold. Do you accept?");
}

TEST_F(CGCreatureJoiningTest, PartialFreeJoinNamesJoiningAndTotalCount)
{
	auto * monster = startWithMonster(10);
	ASSERT_NE(monster, nullptr);
	monster->joiningPercentage = 50;

	EXPECT_EQ(offerText(monster), "{Followers}\n\n5 of the 10 Pikemen with a desire for greater glory wish to join you. Do you accept?");
}

TEST_F(CGCreatureJoiningTest, WholeStackJoinForGoldKeepsOriginalText)
{
	auto * monster = startWithMonster(10);
	ASSERT_NE(monster, nullptr);
	monster->joiningPercentage = 100;
	monster->joinOnlyForMoney = true;

	MetaString expected;
	expected.appendTextID("core.advevent.90");
	expected.replaceNumber(10);
	expected.replaceNumber(600);
	expected.replaceNamePlural(pikeman);
	EXPECT_EQ(offerText(monster), expected.toString(LIBRARY->staticTexts()));
}
