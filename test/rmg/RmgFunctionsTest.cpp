/*
 * RmgFunctionsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rmg/Functions.h"

class RmgFunctionsTest : public testing::Test
{
protected:
	MapObjectSubID crystalCavern{4};
	MapObjectSubID sandCrystalMine;
	std::vector<MapObjectSubID> candidates;

	void SetUp() override
	{
		const auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), std::string("core:mine"), std::string("vcmi-test:sandCrystalMine"));
		ASSERT_TRUE(identifier.has_value());
		sandCrystalMine = MapObjectSubID(*identifier);
		candidates = {crystalCavern, sandCrystalMine};
	}
};

TEST_F(RmgFunctionsTest, KeepsBothCrystalMinesOnSand)
{
	EXPECT_THAT(filterSubtypesPlaceableOn(Obj::MINE, candidates, TerrainId::SAND), testing::ElementsAre(crystalCavern, sandCrystalMine));
}

TEST_F(RmgFunctionsTest, DropsSandOnlyMineOnGrass)
{
	EXPECT_THAT(filterSubtypesPlaceableOn(Obj::MINE, candidates, TerrainId::GRASS), testing::ElementsAre(crystalCavern));
}

TEST_F(RmgFunctionsTest, DropsAllCrystalMinesOnWater)
{
	EXPECT_THAT(filterSubtypesPlaceableOn(Obj::MINE, candidates, TerrainId::WATER), testing::IsEmpty());
}
