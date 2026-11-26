/*
 * ZoneFactionOrderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/Functions.h"

#include <algorithm>

namespace test
{

using namespace ::testing;

class ZoneFactionOrderTest : public Test
{
protected:
	using HintMap = std::map<TRmgTemplateZoneId, std::vector<rmg::ZoneOptions::CTownHints>>;

	static rmg::ZoneOptions::CTownHints likeZone(TRmgTemplateZoneId zone)
	{
		rmg::ZoneOptions::CTownHints hint;
		hint.likeZone = zone;
		return hint;
	}

	static rmg::ZoneOptions::CTownHints notLikeZone(std::vector<TRmgTemplateZoneId> zones)
	{
		rmg::ZoneOptions::CTownHints hint;
		hint.notLikeZone = std::move(zones);
		return hint;
	}

	static rmg::ZoneOptions::CTownHints relatedToZoneTerrain(TRmgTemplateZoneId zone)
	{
		rmg::ZoneOptions::CTownHints hint;
		hint.relatedToZoneTerrain = zone;
		return hint;
	}

	static HintMap emptyHints(std::initializer_list<TRmgTemplateZoneId> ids)
	{
		HintMap hints;
		for(const auto id : ids)
			hints[id] = {};
		return hints;
	}

	static size_t indexOf(const std::vector<TRmgTemplateZoneId> & order, TRmgTemplateZoneId zone)
	{
		const auto it = std::find(order.begin(), order.end(), zone);
		EXPECT_NE(it, order.end()) << "zone " << zone << " missing from order";
		return static_cast<size_t>(it - order.begin());
	}
};

TEST_F(ZoneFactionOrderTest, NoHintsYieldsAscendingIds)
{
	/*
	 * Asserts: zones with no first-town hint are ordered by zone id.
	 * Pass: 1, 2, 3.
	 * Fail: any other permutation.
	 */
	const auto order = orderZonesByFirstTownHint(emptyHints({1, 2, 3}));
	const std::vector<TRmgTemplateZoneId> expected{1, 2, 3};
	EXPECT_EQ(order, expected);
}

TEST_F(ZoneFactionOrderTest, LikeZoneComesBeforeDependent)
{
	/*
	 * Asserts: a first hint likeZone names a zone that must be resolved first.
	 * Pass: zone 3 is before zone 1 when zone 1's first hint is likeZone 3.
	 * Fail: 1 is resolved before 3.
	 */
	auto hints = emptyHints({1, 2, 3});
	hints[1] = {likeZone(3)};
	const auto order = orderZonesByFirstTownHint(hints);
	EXPECT_LT(indexOf(order, 3), indexOf(order, 1));
}

TEST_F(ZoneFactionOrderTest, NotLikeZoneComesAfterEveryNamedZone)
{
	/*
	 * Asserts: a first hint notLikeZone depends on every named zone.
	 * Pass: zone 3 is last when its first hint is notLikeZone [1, 2, 4, 5].
	 * Fail: 3 appears before any of 1, 2, 4, 5.
	 */
	auto hints = emptyHints({1, 2, 3, 4, 5});
	hints[3] = {notLikeZone({1, 2, 4, 5})};
	const auto order = orderZonesByFirstTownHint(hints);
	const std::vector<TRmgTemplateZoneId> expected{1, 2, 4, 5, 3};
	EXPECT_EQ(order, expected);
}

TEST_F(ZoneFactionOrderTest, RelatedToZoneTerrainAddsEdge)
{
	/*
	 * Asserts: a first hint relatedToZoneTerrain is an edge to that zone.
	 * Pass: zone 4 is before zone 1.
	 * Fail: 1 is resolved before 4.
	 */
	auto hints = emptyHints({1, 2, 4});
	hints[1] = {relatedToZoneTerrain(4)};
	const auto order = orderZonesByFirstTownHint(hints);
	EXPECT_LT(indexOf(order, 4), indexOf(order, 1));
}

TEST_F(ZoneFactionOrderTest, SelfLikeZoneOnFirstHintAddsNoEdge)
{
	/*
	 * Asserts: a first hint that names its own zone adds no edge and is not a cycle.
	 * Pass: ascending ids, no throw.
	 * Fail: a throw, or a missing zone.
	 */
	auto hints = emptyHints({1, 2, 3});
	hints[2] = {likeZone(2)};
	const auto order = orderZonesByFirstTownHint(hints);
	const std::vector<TRmgTemplateZoneId> expected{1, 2, 3};
	EXPECT_EQ(order, expected);
}

TEST_F(ZoneFactionOrderTest, OnlyFirstSetFieldCounts)
{
	/*
	 * Asserts: likeZone wins over notLikeZone on the same first hint, so only likeZone is an edge.
	 * Pass: zone 1 depends on 2, not on 4; with zone 4 likeZone 1 there is no cycle.
	 * Fail: notLikeZone [4] is treated as an edge and the two first hints cycle.
	 */
	auto hints = emptyHints({1, 2, 4});
	auto both = likeZone(2);
	both.notLikeZone = {4};
	hints[1] = {both};
	hints[4] = {likeZone(1)};
	const auto order = orderZonesByFirstTownHint(hints);
	const std::vector<TRmgTemplateZoneId> expected{2, 1, 4};
	EXPECT_EQ(order, expected);
}

TEST_F(ZoneFactionOrderTest, LaterTownHintsAreNotEdges)
{
	/*
	 * Asserts: only the first town hint creates an ordering edge.
	 * Pass: zones 2 and 3 each likeZone the other on a second hint; no throw.
	 * Fail: the later hints are treated as a cycle.
	 */
	auto hints = emptyHints({2, 3});
	hints[2] = {rmg::ZoneOptions::CTownHints{}, likeZone(3)};
	hints[3] = {rmg::ZoneOptions::CTownHints{}, likeZone(2)};
	const auto order = orderZonesByFirstTownHint(hints);
	const std::vector<TRmgTemplateZoneId> expected{2, 3};
	EXPECT_EQ(order, expected);
}

TEST_F(ZoneFactionOrderTest, FirstHintMutualNotLikeZoneThrows)
{
	/*
	 * Asserts: first hints that name each other form a cycle.
	 * Pass: rmgException listing zones 2 and 3.
	 * Fail: an order is returned, or the exception does not name the zones.
	 */
	auto hints = emptyHints({2, 3});
	hints[2] = {notLikeZone({3})};
	hints[3] = {notLikeZone({2})};
	try
	{
		orderZonesByFirstTownHint(hints);
		FAIL() << "expected rmgException";
	}
	catch(const rmgException & e)
	{
		const std::string message = e.what();
		EXPECT_THAT(message, HasSubstr("cycle"));
		EXPECT_THAT(message, HasSubstr("zones 2, 3"));
	}
}

TEST_F(ZoneFactionOrderTest, ThreeZoneFirstHintCycleThrows)
{
	/*
	 * Asserts: a three-zone first-hint cycle throws.
	 * Pass: rmgException listing zones 1, 2 and 3.
	 * Fail: an order is returned.
	 */
	auto hints = emptyHints({1, 2, 3});
	hints[1] = {likeZone(2)};
	hints[2] = {likeZone(3)};
	hints[3] = {likeZone(1)};
	try
	{
		orderZonesByFirstTownHint(hints);
		FAIL() << "expected rmgException";
	}
	catch(const rmgException & e)
	{
		const std::string message = e.what();
		EXPECT_THAT(message, HasSubstr("cycle"));
		EXPECT_THAT(message, HasSubstr("zones 1, 2, 3"));
	}
}

}
