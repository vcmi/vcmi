/*
 * StartingHeroTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/TinyMapGameTest.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

class StartingHeroTest : public TinyMapGameTest
{
protected:
	const PlayerColor player = PlayerColor(0);
	const int3 townPos = int3(10, 10, 0);

	const Services * gameServices() override { return LIBRARY; }

	TinyH3M::TinyH3MBuilder mapWithMainTown()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(player)
			.playerMainTown(player, townPos, true)
			.town(townPos, FactionID::CASTLE, player);
		return builder;
	}
};

TEST_F(StartingHeroTest, heroIsGeneratedInMainTown)
{
	startWithMap(mapWithMainTown());

	auto * town = findAll<CGTownInstance>().front();
	ASSERT_NE(town->getVisitingHero(), nullptr);
	EXPECT_EQ(town->getVisitingHero()->getOwner(), player);
}

TEST_F(StartingHeroTest, heroIsNotGeneratedInMainTownThatAlreadyHasVisitingHero)
{
	auto builder = mapWithMainTown();
	builder.hero(townPos + int3(-1, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	auto * town = findAll<CGTownInstance>().front();
	ASSERT_NE(town->getVisitingHero(), nullptr);
	EXPECT_EQ(town->getVisitingHero()->getHeroTypeID(), HeroTypeID(0));
	EXPECT_EQ(findAll<CGHeroInstance>().size(), 1);
}
