/*
 * PlayerZoneArrangementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/PlayerZoneArrangement.h"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <vector>

namespace test
{

using namespace ::testing;

namespace
{

PlayerZoneInfo makeZone(
	TRmgTemplateZoneId id,
	int owner,
	bool participates,
	int level,
	int size,
	float3 center,
	std::vector<std::pair<TRmgTemplateZoneId, rmg::EConnectionType>> links)
{
	PlayerZoneInfo zone;
	zone.id = id;
	zone.owner = owner;
	zone.participates = participates;
	zone.level = level;
	zone.size = size;
	zone.center = center;
	zone.links = std::move(links);
	return zone;
}

PlayerZoneInfo hubLinked(TRmgTemplateZoneId id, int owner, bool participates, float3 center = {})
{
	return makeZone(id, owner, participates, 0, 30, center, {{5, rmg::EConnectionType::GUARDED}});
}

std::vector<std::vector<TRmgTemplateZoneId>> sortedGroups(std::vector<std::vector<TRmgTemplateZoneId>> groups)
{
	for(auto & group : groups)
		std::sort(group.begin(), group.end());
	std::sort(groups.begin(), groups.end());
	return groups;
}

using CenterKey = std::tuple<float, float, int>;

std::vector<CenterKey> centerBagFromZones(const std::vector<PlayerZoneInfo> & zones)
{
	std::vector<CenterKey> bag;
	for(const auto & zone : zones)
		bag.emplace_back(zone.center.x, zone.center.y, zone.center.z);
	std::sort(bag.begin(), bag.end());
	return bag;
}

std::vector<CenterKey> centerBagFromMap(const std::map<TRmgTemplateZoneId, float3> & centers)
{
	std::vector<CenterKey> bag;
	for(const auto & [id, center] : centers)
	{
		(void)id;
		bag.emplace_back(center.x, center.y, center.z);
	}
	std::sort(bag.begin(), bag.end());
	return bag;
}

bool holdsDiagonal(const float3 & a, const float3 & b)
{
	return std::abs(a.x - b.x) > 0.5f && std::abs(a.y - b.y) > 0.5f;
}

}

TEST(InterchangeableZonesTest, FourZonesLinkedToSameHubFormOneGroup)
{
	const std::vector<PlayerZoneInfo> zones = {
		hubLinked(1, 1, true),
		hubLinked(2, 2, true),
		hubLinked(3, 3, true),
		hubLinked(4, 4, true),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{1, 2, 3, 4}));
}

TEST(InterchangeableZonesTest, ExtraLinkKeepsZoneOutOfGroup)
{
	auto extra = hubLinked(1, 1, true);
	extra.links.emplace_back(6, rmg::EConnectionType::GUARDED);
	const std::vector<PlayerZoneInfo> zones = {
		extra,
		hubLinked(2, 2, true),
		hubLinked(3, 3, true),
		hubLinked(4, 4, true),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{2, 3, 4}));
}

TEST(InterchangeableZonesTest, DifferentSizeIsNotGrouped)
{
	auto small = hubLinked(1, 1, true);
	small.size = 20;
	const std::vector<PlayerZoneInfo> zones = {
		small,
		hubLinked(2, 2, true),
		hubLinked(3, 3, true),
		hubLinked(4, 4, true),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{2, 3, 4}));
}

TEST(InterchangeableZonesTest, DifferentLevelIsNotGrouped)
{
	auto underground = hubLinked(1, 1, true);
	underground.level = 1;
	const std::vector<PlayerZoneInfo> zones = {
		underground,
		hubLinked(2, 2, true),
		hubLinked(3, 3, true),
		hubLinked(4, 4, true),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{2, 3, 4}));
}

TEST(InterchangeableZonesTest, MutualLinkToSameHubIsGrouped)
{
	const std::vector<PlayerZoneInfo> zones = {
		makeZone(1, 1, true, 0, 30, {}, {
			{5, rmg::EConnectionType::GUARDED},
			{2, rmg::EConnectionType::GUARDED},
		}),
		makeZone(2, 2, true, 0, 30, {}, {
			{5, rmg::EConnectionType::GUARDED},
			{1, rmg::EConnectionType::GUARDED},
		}),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{1, 2}));
}

TEST(InterchangeableZonesTest, DifferentConnectionTypeKeepsZoneOutOfGroup)
{
	auto wide = hubLinked(1, 1, true);
	wide.links = {{5, rmg::EConnectionType::WIDE}};
	const std::vector<PlayerZoneInfo> zones = {
		wide,
		hubLinked(2, 2, true),
		hubLinked(3, 3, true),
		hubLinked(4, 4, true),
	};
	const auto groups = sortedGroups(findInterchangeableZones(zones));
	ASSERT_EQ(groups.size(), 1u);
	EXPECT_EQ(groups[0], (std::vector<TRmgTemplateZoneId>{2, 3, 4}));
}

TEST(ArrangeByPlayerOrderTest, NeighbouringOwnersTakeADiagonal)
{
	const std::vector<PlayerZoneInfo> zones = {
		hubLinked(1, 1, true, float3(0.f, 0.f, 0)),
		hubLinked(2, 2, true, float3(1.f, 0.f, 0)),
		hubLinked(3, 3, true, float3(1.f, 1.f, 0)),
		hubLinked(4, 4, true, float3(0.f, 1.f, 0)),
	};
	const auto arranged = arrangeByPlayerOrder(zones);
	ASSERT_EQ(arranged.size(), 4u);
	EXPECT_TRUE(holdsDiagonal(arranged.at(1), arranged.at(2)));
	EXPECT_EQ(centerBagFromMap(arranged), centerBagFromZones(zones));
}

TEST(ArrangeByPlayerOrderTest, ParticipatingOwnersOneAndThreeTakeADiagonal)
{
	const std::vector<PlayerZoneInfo> zones = {
		hubLinked(1, 1, true, float3(0.f, 0.f, 0)),
		hubLinked(2, 2, false, float3(1.f, 0.f, 0)),
		hubLinked(3, 3, true, float3(1.f, 1.f, 0)),
		hubLinked(4, 4, false, float3(0.f, 1.f, 0)),
	};
	const auto arranged = arrangeByPlayerOrder(zones);
	ASSERT_EQ(arranged.size(), 4u);
	EXPECT_TRUE(holdsDiagonal(arranged.at(1), arranged.at(3)));
}

TEST(ArrangeByPlayerOrderTest, EightRingCentersPlaceOneTwoAndThreeFourOpposite)
{
	constexpr float h = 0.7071067811865476f;
	const std::vector<float3> ring = {
		float3(1.f, 0.f, 0),
		float3(h, h, 0),
		float3(0.f, 1.f, 0),
		float3(-h, h, 0),
		float3(-1.f, 0.f, 0),
		float3(-h, -h, 0),
		float3(0.f, -1.f, 0),
		float3(h, -h, 0),
	};
	std::vector<PlayerZoneInfo> zones;
	for(int i = 0; i < 8; ++i)
		zones.push_back(hubLinked(i + 1, i + 1, i < 4, ring[i]));

	const auto arranged = arrangeByPlayerOrder(zones);
	ASSERT_EQ(arranged.size(), 8u);
	EXPECT_NEAR(arranged.at(1).dist2d(arranged.at(2)), 2.0, 1e-5);
	EXPECT_NEAR(arranged.at(3).dist2d(arranged.at(4)), 2.0, 1e-5);
	const float3 v12 = arranged.at(2) - arranged.at(1);
	const float3 v34 = arranged.at(4) - arranged.at(3);
	EXPECT_NEAR(v12.x * v34.x + v12.y * v34.y, 0.0, 1e-4);
}

TEST(ArrangeByPlayerOrderTest, SameInputIsDeterministic)
{
	const std::vector<PlayerZoneInfo> zones = {
		hubLinked(1, 1, true, float3(0.f, 0.f, 0)),
		hubLinked(2, 2, true, float3(1.f, 0.f, 0)),
		hubLinked(3, 3, true, float3(1.f, 1.f, 0)),
		hubLinked(4, 4, true, float3(0.f, 1.f, 0)),
	};
	const auto first = arrangeByPlayerOrder(zones);
	const auto second = arrangeByPlayerOrder(zones);
	ASSERT_EQ(first.size(), second.size());
	for(const auto & [id, center] : first)
	{
		EXPECT_FLOAT_EQ(center.x, second.at(id).x);
		EXPECT_FLOAT_EQ(center.y, second.at(id).y);
		EXPECT_EQ(center.z, second.at(id).z);
	}
}

TEST(PlayerCrowdingTest, NoPairsGivesZero)
{
	EXPECT_FLOAT_EQ(playerCrowding({}), 0.f);
}

TEST(PlayerCrowdingTest, TouchingDistanceGivesOne)
{
	EXPECT_FLOAT_EQ(playerCrowding({{1.f, 1.f}}), 1.f);
}

TEST(PlayerCrowdingTest, TwiceTheDistanceGivesHalf)
{
	EXPECT_FLOAT_EQ(playerCrowding({{2.f, 1.f}}), 0.5f);
}

TEST(PlayerCrowdingTest, ZeroDistanceIsClampedToTen)
{
	EXPECT_FLOAT_EQ(playerCrowding({{0.f, 1.f}}), 10.f);
}

TEST(PlayerPairWeightTest, OwnersOneAndTwoBothInGameGiveTwo)
{
	EXPECT_FLOAT_EQ(playerPairWeight(1, 2, true), 2.f);
}

TEST(PlayerPairWeightTest, PairOneTwoOutweighsEveryOtherPairAmongOwnersOneToEight)
{
	const float heaviest = playerPairWeight(1, 2, true);
	for(int a = 1; a <= 8; ++a)
	{
		for(int b = a + 1; b <= 8; ++b)
		{
			if(a == 1 && b == 2)
				continue;
			EXPECT_GT(heaviest, playerPairWeight(a, b, true));
		}
	}
}

TEST(PlayerPairWeightTest, WeightIsIndependentOfOwnerOrder)
{
	EXPECT_FLOAT_EQ(playerPairWeight(1, 2, true), playerPairWeight(2, 1, true));
	EXPECT_FLOAT_EQ(playerPairWeight(3, 8, true), playerPairWeight(8, 3, true));
}

TEST(PlayerPairWeightTest, PairWithOwnerOutsideTheGameGivesOne)
{
	EXPECT_FLOAT_EQ(playerPairWeight(1, 2, false), 1.f);
}

}
