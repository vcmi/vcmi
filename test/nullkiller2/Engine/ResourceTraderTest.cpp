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

#include "AI/Nullkiller2/Engine/ResourceTrader.h"
#include "test/nullkiller2/Nulkiller2TestUtils.h"

class MockMarket final : public IMarket
{
public:
	explicit MockMarket(IGameInfoCallback * cb) : IMarket(cb) {}
	~MockMarket() override = default;

	MOCK_METHOD(bool, allowsTrade, (const EMarketMode mode), (const, override));
	MOCK_METHOD(bool, getOffer, (int id1, int id2, int & val1, int & val2, EMarketMode mode), (const, override));
	MOCK_METHOD(int, availableUnits, (const EMarketMode mode, const int marketItemSerial), (const, override));
	MOCK_METHOD(std::vector<TradeItemBuy>, availableItemsIds, (const EMarketMode mode), (const, override));
	MOCK_METHOD(std::set<EMarketMode>, availableModes, (), (const, override));
	MOCK_METHOD(int, getMarketEfficiency, (), (const, override));
	ObjectInstanceID getObjInstanceID() const override
	{
		return ObjectInstanceID();
	}
};

class MockBuildAnalyzer final : public NK2AI::BuildAnalyzer
{
public:
	explicit MockBuildAnalyzer() : BuildAnalyzer(nullptr) {}
	MOCK_METHOD(bool, getGoldPressure, (), (const));
};

class MockCCallback final : public CCallback
{
public:
	MockCCallback() : CCallback(nullptr, std::nullopt, nullptr) {}
	MOCK_METHOD(
		void,
		trade,
		(const ObjectInstanceID marketId, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero),
		(override)
	);
};

TEST(Nullkiller2_Engine_ResourceTrader, tradeHelper)
{
	const TResources missingNow = Nulkiller2TestUtils::res(0, 0, 0, 0, 0, 0, 193445, 0);
	const TResources income = Nulkiller2TestUtils::res(2, 2, 2, 2, 2, 2, 1000, 0);
	const TResources freeAfterMissingTotal = Nulkiller2TestUtils::res(1000, 1000, 1000, 1001, 1002, 1000, 0, 0);

	MockMarket market(nullptr);
	EXPECT_CALL(market, getOffer(4, 6, testing::_, testing::_, EMarketMode::RESOURCE_RESOURCE)).Times(1);
	MockBuildAnalyzer buildAnalyzer;
	MockCCallback callback;
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, market, missingNow, income, freeAfterMissingTotal, buildAnalyzer, callback);
}

// ResourceTrader: Free [13919, 13883, 13921, 13857, 13792, 13883, 14, 0]. FreeAfterMissingTotal [13859, 13819, 13891, 13833, 13718, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193445, 0]
// ResourceTrader: got offer: 1 of mostExpendable 2 for 125 of mostWanted: 6
// ResourceTrader: Traded 1547 of 2 for 125 receivedPerUnit of 6
// ResourceTrader: Free [13919, 13883, 13921, 13857, 13792, 13883, 14, 0]. FreeAfterMissingTotal [13859, 13819, 12344, 13833, 13718, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 70, 0]
// ResourceTrader: got offer: 1 of mostExpendable 0 for 125 of mostWanted: 6
// ResourceTrader: Traded 1 of 0 for 125 receivedPerUnit of 6
// ResourceTrader: Free [13908, 13883, 12374, 13857, 13722, 13883, 414, 0]. FreeAfterMissingTotal [13848, 13819, 12344, 13833, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193075, 0]
// ResourceTrader: got offer: 1 of mostExpendable 0 for 125 of mostWanted: 6
// ResourceTrader: Traded 1544 of 0 for 125 receivedPerUnit of 6
// ResourceTrader: Free [13908, 13883, 12374, 13857, 13722, 13883, 414, 0]. FreeAfterMissingTotal [12304, 13819, 12344, 13833, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 75, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 250 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 250 receivedPerUnit of 6
// ResourceTrader: Free [12364, 13883, 12374, 13841, 13722, 13883, 24, 0]. FreeAfterMissingTotal [12304, 13819, 12344, 13817, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 193465, 0]
// ResourceTrader: got offer: 1 of mostExpendable 1 for 250 of mostWanted: 6
// ResourceTrader: Traded 773 of 1 for 250 receivedPerUnit of 6
// ResourceTrader: Free [12364, 13883, 12374, 13841, 13722, 13883, 24, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13817, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 215, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 250 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 250 receivedPerUnit of 6
// ResourceTrader: Free [12364, 13110, 12374, 13837, 13722, 13883, 52524, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13813, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 140965, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 250 of mostWanted: 6
// ResourceTrader: Traded 563 of 3 for 250 receivedPerUnit of 6
// ResourceTrader: Free [12364, 13110, 12374, 13837, 13722, 13883, 52524, 0]. FreeAfterMissingTotal [12304, 13046, 12344, 13250, 13648, 13763, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 215, 0]
// ResourceTrader: got offer: 1 of mostExpendable 5 for 250 of mostWanted: 6
// ResourceTrader: Traded 1 of 5 for 250 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10930, 10925, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10900, 10925, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10930, 10925, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10900, 10809, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10929, 10809, 3, 47, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10899, 10809, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17453, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10929, 10809, 3, 47, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10783, 10809, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 53, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10813, 10808, 3, 22, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10783, 10808, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17478, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10813, 10808, 3, 22, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10783, 10692, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 78, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10812, 10692, 3, 27, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10782, 10692, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17473, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10812, 10692, 3, 27, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10666, 10692, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 73, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10696, 10691, 3, 2, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10666, 10691, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17498, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10696, 10691, 3, 2, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10666, 10575, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 98, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10695, 10575, 3, 22, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10665, 10575, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17478, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10695, 10575, 3, 22, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10549, 10575, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 78, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10579, 10574, 3, 42, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10549, 10574, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17458, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10579, 10574, 3, 42, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10549, 10458, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 58, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10578, 10458, 3, 62, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10548, 10458, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17438, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10578, 10458, 3, 62, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10432, 10458, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 38, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10462, 10457, 3, 17, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10432, 10457, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17483, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10462, 10457, 3, 17, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10432, 10341, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 83, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10461, 10341, 3, 27, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10431, 10341, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17473, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10461, 10341, 3, 27, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10315, 10341, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 73, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10345, 10340, 3, 2, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10315, 10340, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17498, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10345, 10340, 3, 2, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10315, 10224, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 98, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10344, 10224, 3, 42, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10314, 10224, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17458, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10344, 10224, 3, 42, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10198, 10224, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 58, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10228, 10223, 3, 17, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10198, 10223, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17483, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10228, 10223, 3, 17, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10198, 10107, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 83, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10227, 10107, 3, 57, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10197, 10107, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17443, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10227, 10107, 3, 57, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10081, 10107, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 43, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10111, 10106, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10081, 10106, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10111, 10106, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10081, 9990, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10110, 9990, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 10080, 9990, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 10110, 9990, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9964, 9990, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9994, 9989, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9964, 9989, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9994, 9989, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9964, 9873, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9993, 9873, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9963, 9873, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9993, 9873, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9847, 9873, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9877, 9872, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9847, 9872, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9877, 9872, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9847, 9756, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9876, 9756, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9846, 9756, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9876, 9756, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9730, 9756, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9760, 9755, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9730, 9755, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17493, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9760, 9755, 3, 7, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9730, 9639, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 93, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9759, 9639, 3, 57, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9729, 9639, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17443, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9759, 9639, 3, 57, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9613, 9639, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 43, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9643, 9638, 3, 47, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9613, 9638, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 17453, 0]
// ResourceTrader: got offer: 1 of mostExpendable 4 for 150 of mostWanted: 6
// ResourceTrader: Traded 116 of 4 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9643, 9638, 3, 47, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9613, 9522, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 53, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9642, 9522, 3, 5232, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9612, 9522, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 12268, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 81 of 3 for 150 receivedPerUnit of 6
// ResourceTrader: Free [2097, 268, 1168, 9642, 9522, 3, 5232, 0]. FreeAfterMissingTotal [2097, 268, 1168, 9531, 9522, 3, 0, 0]. MissingNow  [0, 0, 0, 0, 0, 0, 118, 0]
// ResourceTrader: got offer: 1 of mostExpendable 3 for 150 of mostWanted: 6
// ResourceTrader: Traded 1 of 3 for 150 receivedPerUnit of 6
