/*
 * TownHeroAssignmentTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/StartInfo.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapping/CMap.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

class TownHeroAssignmentTest : public QuestTest
{
public:
	void startGame(int townCount = 1, int heroCount = 1)
	{
		const std::array townPositions = {
			int3(9, 5, 0),
			int3(20, 5, 0),
			int3(30, 5, 0)
		};
		const std::array heroPositions = {
			int3(5, 5, 0),
			int3(6, 5, 0)
		};

		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER);

		for(int index = 0; index < townCount; ++index)
			builder.randomTown(townPositions.at(index), PLAYER);

		for(int index = 0; index < heroCount; ++index)
			builder.hero(heroPositions.at(index), HeroTypeID(index), PLAYER);

		startWithMap(std::move(builder));
		towns = findAll<CGTownInstance>();
		heroes = findAll<CGHeroInstance>();
	}

	CGTownInstance * town(size_t index = 0)
	{
		return towns.at(index);
	}

	CGHeroInstance * hero(size_t index = 0)
	{
		return heroes.at(index);
	}

	void placeHeroAtTown(size_t heroIndex = 0, size_t townIndex = 0)
	{
		hero(heroIndex)->pos = hero(heroIndex)->convertFromVisitablePos(town(townIndex)->visitablePos());
	}

private:
	std::vector<CGTownInstance *> towns;
	std::vector<CGHeroInstance *> heroes;
};
}

TEST_F(TownHeroAssignmentTest, loadKeepsValidAssignmentAndRemovesStaleSlot)
{
	startGame(3, 2);

	placeHeroAtTown(0, 0);
	town(0)->setVisitingHero(hero(0));
	town(1)->setVisitingHero(hero(1));
	placeHeroAtTown(1, 2);
	town(2)->setVisitingHero(hero(1));

	gameState->updateOnLoad(StartInfo());

	EXPECT_EQ(town(0)->getVisitingHero(), hero(0));
	EXPECT_EQ(hero(0)->getVisitedTown(), town(0));
	EXPECT_EQ(town(1)->getVisitingHero(), nullptr);
	EXPECT_EQ(town(2)->getVisitingHero(), hero(1));
	EXPECT_EQ(hero(1)->getVisitedTown(), town(2));
}

TEST_F(TownHeroAssignmentTest, replacingVisitorDetachesDisplacedHero)
{
	startGame(1, 2);

	town()->setVisitingHero(hero(0));
	town()->setVisitingHero(hero(1));

	EXPECT_EQ(town()->getVisitingHero(), hero(1));
	EXPECT_EQ(hero(0)->getVisitedTown(), nullptr);
	EXPECT_EQ(hero(1)->getVisitedTown(), town());
}

TEST_F(TownHeroAssignmentTest, inconsistentRuntimeAssignmentsThrow)
{
	startGame(2, 2);

	town(0)->setVisitingHero(hero(0));
	town(1)->setVisitingHero(hero(0));
	town(0)->setGarrisonedHero(hero(1));
	town(1)->setGarrisonedHero(hero(1));

	EXPECT_THROW(town(0)->setVisitingHero(nullptr), std::runtime_error);
	EXPECT_THROW(town(0)->setGarrisonedHero(nullptr), std::runtime_error);
	EXPECT_EQ(town(1)->getVisitingHero(), hero(0));
	EXPECT_EQ(town(1)->getGarrisonHero(), hero(1));
}
