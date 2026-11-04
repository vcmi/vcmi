/*
 * CampaignOverridesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "campaign/CampaignHandler.h"
#include "campaign/CampaignState.h"
#include "filesystem/Filesystem.h"
#include "mapping/CMapHeader.h"

// otherwise the enemy hero would be given to the player through power-ranked placeholders
TEST(CampaignOverridesTest, enemyHeroCarriedOverIsReservedByNextScenario)
{
	if(!CResourceHandler::get()->existsResource(ResourcePath("DATA/AB", EResType::CAMPAIGN)))
		GTEST_SKIP() << "Armageddon's Blade campaign is not installed";

	auto campaign = CampaignHandler::getCampaign("DATA/AB");
	HeroTypeID enemyHero = campaign->scenario(CampaignScenarioID(6)).enemyHeroCarriedOver;

	ASSERT_TRUE(enemyHero.hasValue());
	EXPECT_TRUE(campaign->getMapHeader(CampaignScenarioID(7))->reservedCampaignHeroes.count(enemyHero));
}
