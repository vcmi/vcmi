/*
 * RecruitHeroBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "Global.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AI/Nullkiller2/Behaviors/RecruitHeroBehavior.h"

class MockMarket : public IMarket
{
public:
	explicit MockMarket(IGameInfoCallback * cb)
		: IMarket(cb)
	{
	}
	~MockMarket() override = default;
	MOCK_METHOD(bool, getOffer, (int id1, int id2, int & val1, int & val2, EMarketMode mode), ());
	ObjectInstanceID getObjInstanceID() const override;
	int getMarketEfficiency() const override;
	std::set<EMarketMode> availableModes() const override;
};

TEST(Nullkiller2_Engine_ResourceTrader, tradeHelper)
{
	// auto * const market = new MockMarket(nullptr);
	// EXPECT_CALL(*market, getOffer(testing::internal::Any, testing::internal::Any, testing::internal::Any, testing::internal::Any, EMarketMode::RESOURCE_RESOURCE)).Times(1);
	// market->getOffer(0, 0, 0, 0, EMarketMode::RESOURCE_RESOURCE);
	// delete market;
}

TResources res(const int wood, const int mercury, const int ore, const int sulfur, const int crystals, const int gems, const int gold, const int mithril)
{
	TResources resources;
	resources[0] = wood;
	resources[1] = mercury;
	resources[2] = ore;
	resources[3] = sulfur;
	resources[4] = crystals;
	resources[5] = gems;
	resources[6] = gold;
	resources[7] = mithril;
	return resources;
}

// Nullkiller::handleTrading Free [13919, 13883, 13921, 13857, 13792, 13883, 14, 0]. FreeAfterMissingTotal [13859, 13819, 13891, 13833, 13718, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193445, 0]
// Nullkiller::handleTrading Traded 1547 of 2 for 125 of 6
// Nullkiller::handleTrading Free [13919, 13883, 13921, 13857, 13792, 13883, 14, 0]. FreeAfterMissingTotal [13859, 13819, 12344, 13833, 13718, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 70, 0]
// Nullkiller::handleTrading Traded 1 of 0 for 125 of 6
// Nullkiller::handleTrading Free [13908, 13883, 12374, 13857, 13722, 13883, 414, 0]. FreeAfterMissingTotal [13848, 13819, 12344, 13833, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193075, 0]
// Nullkiller::handleTrading Traded 1544 of 0 for 125 of 6
// Nullkiller::handleTrading Free [13908, 13883, 12374, 13857, 13722, 13883, 414, 0]. FreeAfterMissingTotal [12304, 13819, 12344, 13833, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 75, 0]
// Nullkiller::handleTrading Traded 1 of 3 for 250 of 6
// Nullkiller::handleTrading Free [12364, 13883, 12374, 13841, 13722, 13883, 24, 0]. FreeAfterMissingTotal [12304, 13819, 12344, 13817, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193465, 0]
// Nullkiller::handleTrading Traded 773 of 1 for 250 of 6
// Nullkiller::handleTrading Free [12364, 13883, 12374, 13841, 13722, 13883, 24, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13817, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 215, 0]
// Nullkiller::handleTrading Traded 1 of 3 for 250 of 6
// Nullkiller::handleTrading Free [12364, 13110, 12374, 13837, 13722, 13883, 52524, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13813, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 140965, 0]
// Nullkiller::handleTrading Traded 563 of 3 for 250 of 6
// Nullkiller::handleTrading Free [12364, 13110, 12374, 13837, 13722, 13883, 52524, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13250, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 215, 0]
// Nullkiller::handleTrading Traded 1 of 5 for 250 of 6
