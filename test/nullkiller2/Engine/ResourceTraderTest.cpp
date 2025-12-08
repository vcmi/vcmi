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
	ObjectInstanceID id;
	int marketEfficiency;

public:
	explicit MockMarket(const int marketEfficiency) : marketEfficiency(marketEfficiency), IMarket(nullptr) {}
	~MockMarket() override = default;

	MOCK_METHOD(std::set<EMarketMode>, availableModes, (), (const, override));
	ObjectInstanceID getObjInstanceID() const override
	{
		return id;
	}
	int getMarketEfficiency() const override
	{
		return marketEfficiency;
	}
};

class MockBuildAnalyzer final : public NK2AI::BuildAnalyzer
{
public:
	explicit MockBuildAnalyzer() : BuildAnalyzer(nullptr) {}
	MOCK_METHOD(bool, isGoldPressureOverMax, (), (const, override));
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

TEST(Nullkiller2_Engine_ResourceTrader, tradeHelper_crystalsToGold)
{
	const TResources missingNow01 = Nulkiller2TestUtils::res(0, 0, 0, 4000, 0, 0, 75000);
	const TResources missingNow02 = Nulkiller2TestUtils::res(0, 0, 0, 0, 0, 0, 10000);
	const TResources income = Nulkiller2TestUtils::res(2, 2, 2, 4000, 2, 2, 1000);
	const TResources freeAfterMissingTotal = Nulkiller2TestUtils::res(1000, 1000, 1000, 0, 1002, 1000, 0);

	MockBuildAnalyzer ba;
	EXPECT_CALL(ba, isGoldPressureOverMax()).Times(0);
	MockMarket m01(1);
	MockMarket m20(20); // maxes out at 0.5 effectiveness anyway after 9 castles
	MockCCallback cc;

	// Case: sells 50% of crystals
	EXPECT_CALL(cc, trade(m01.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(4)), TradeItemBuy(GameResID(6)), 501, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m01, missingNow01, income, freeAfterMissingTotal, ba, cc);

	// Case: only 200 crystals because that's enough for getting the desired goal
	EXPECT_CALL(cc, trade(m01.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(4)), TradeItemBuy(GameResID(6)), 200, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m01, missingNow02, income, freeAfterMissingTotal, ba, cc);

	// Case: only 40 crystals because that's enough for getting the desired goal
	EXPECT_CALL(cc, trade(m20.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(4)), TradeItemBuy(GameResID(6)), 40, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m20, missingNow02, income, freeAfterMissingTotal, ba, cc);
}

TEST(Nullkiller2_Engine_ResourceTrader, tradeHelper_goldToGems)
{
	const TResources missingNow01 = Nulkiller2TestUtils::res(0, 0, 0, 4000, 0, 200, 0);
	const TResources missingNow02 = Nulkiller2TestUtils::res(0, 0, 0, 4000, 0, 4, 0);
	const TResources income = Nulkiller2TestUtils::res(2, 2, 2, 4000, 2, 2, 1000);
	const TResources freeAfterMissingTotal = Nulkiller2TestUtils::res(100, 100, 100, 0, 0, 0, 100000);

	MockBuildAnalyzer ba;
	EXPECT_CALL(ba, isGoldPressureOverMax()).Times(3).WillRepeatedly(testing::Return(false));
	MockMarket m(1);
	MockMarket m20(20); // maxes out at 0.5 effectiveness anyway after 9 castles
	MockCCallback cc;

	// Case: sells 50% of gold
	EXPECT_CALL(cc, trade(m.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(6)), TradeItemBuy(GameResID(5)), 50000, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m, missingNow01, income, freeAfterMissingTotal, ba, cc);

	// Case: only 20,000 gold because that's enough for getting the desired goal
	EXPECT_CALL(cc, trade(m.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(6)), TradeItemBuy(GameResID(5)), 20000, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m, missingNow02, income, freeAfterMissingTotal, ba, cc);

	// Case: only 4,000 gold because that's enough for getting the desired goal
	EXPECT_CALL(cc, trade(m20.getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, TradeItemSell(GameResID(6)), TradeItemBuy(GameResID(5)), 4000, nullptr)).Times(1);
	NK2AI::ResourceTrader::tradeHelper(NK2AI::ResourceTrader::EXPENDABLE_BULK_RATIO, m20, missingNow02, income, freeAfterMissingTotal, ba, cc);
}
