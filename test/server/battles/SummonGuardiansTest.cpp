/*
 * SummonGuardiansTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/CCreatureHandler.h"

namespace
{

constexpr int32_t bearerCount = 10;
/// The fixture summoners raise stacks half their own size.
constexpr int32_t guardianCount = bearerCount / 2;

/// One scenario: which summoner stands where, and the hexes its guardians end up on.
struct SummonGuardiansCase
{
	const char * name;
	const char * bearer;
	const char * guardian; ///< creature the bearer is declared to summon
	BattleSide side;
	int bearerHex;
	std::vector<int> expectedHexes;
};

}

/// Summoned guardians surround their bearer when the battle starts. Where they fit depends on
/// how wide the bearer is, how wide the guardians are, and which side of the field they are on -
/// single-hex guardians simply take every surrounding hex, while double-wide ones need a layout
/// that covers the same ground with as few stacks as possible.
class SummonGuardiansTest : public BattleTestFixture, public ::testing::WithParamInterface<SummonGuardiansCase>
{
public:
	/// Hexes held by guardians, in ascending order.
	std::vector<int> guardianHexes(const CStack * bearer) const
	{
		std::vector<int> result;

		for(const auto & unit : battle()->stacks)
			if(unit->summoned && unit->unitSide() == bearer->unitSide())
				result.push_back(unit->getPosition().toInt());

		std::sort(result.begin(), result.end());
		return result;
	}
};

TEST_P(SummonGuardiansTest, PlacesGuardiansAroundTheBearer)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * bearer = addStack(scenario.side, creatureByName(scenario.bearer), BattleHex(scenario.bearerHex), bearerCount);
	ASSERT_NE(bearer, nullptr);

	beginCombat();

	EXPECT_EQ(guardianHexes(bearer), scenario.expectedHexes) << scenario.name;

	for(const auto & unit : battle()->stacks)
	{
		if(!unit->summoned)
			continue;

		EXPECT_EQ(unit->unitType()->getId(), creatureByName(scenario.guardian)) << scenario.name;
		EXPECT_EQ(unit->getCount(), guardianCount) << scenario.name;
		EXPECT_EQ(unit->unitSide(), scenario.side) << scenario.name;
	}
}

namespace
{
// creatures, named by how wide the bearer is and how wide what it summons is
constexpr auto narrowNarrow = "vcmi-test:testSummonerNarrowNarrow";
constexpr auto narrowWide = "vcmi-test:testSummonerNarrowWide";
constexpr auto wideNarrow = "vcmi-test:testSummonerWideNarrow";
constexpr auto wideWide = "vcmi-test:testSummonerWideWide";

constexpr auto pikeman = "core:pikeman";
constexpr auto wyvern = "core:wyvern";

// middle of the field, clear of the armies the layout places against either edge
constexpr int attackerHex = SummonGuardiansTest::leftHex;
constexpr int defenderHex = SummonGuardiansTest::rightHex;

// against the bearer's own edge of the field, where there is no room behind it
constexpr int attackerEdgeHex = 7 * GameConstants::BFIELD_WIDTH + 1;
constexpr int defenderEdgeHex = 6 * GameConstants::BFIELD_WIDTH + 15;
}

// The expected hexes come from the placement rules themselves: single-hex guardians take every
// hex around the bearer that is free and on the field, double-wide ones take the front, the two
// flanks and - where it fits - the back, each of them needing a second free hex for its body.
INSTANTIATE_TEST_SUITE_P(Scenarios, SummonGuardiansTest, ::testing::Values(
	SummonGuardiansCase{"attackerNarrowBearerNarrowGuardians", narrowNarrow, pikeman, BattleSide::ATTACKER, attackerHex, {74, 75, 91, 93, 108, 109}},
	SummonGuardiansCase{"attackerNarrowBearerWideGuardians",   narrowWide,   wyvern,  BattleSide::ATTACKER, attackerHex, {75, 91, 94, 109}},
	SummonGuardiansCase{"attackerWideBearerNarrowGuardians",   wideNarrow,   pikeman, BattleSide::ATTACKER, attackerHex, {73, 74, 75, 90, 93, 107, 108, 109}},
	SummonGuardiansCase{"attackerWideBearerWideGuardians",     wideWide,     wyvern,  BattleSide::ATTACKER, attackerHex, {74, 76, 90, 94, 108, 110}},

	SummonGuardiansCase{"defenderNarrowBearerNarrowGuardians", narrowNarrow, pikeman, BattleSide::DEFENDER, defenderHex, {75, 76, 92, 94, 109, 110}},
	SummonGuardiansCase{"defenderNarrowBearerWideGuardians",   narrowWide,   wyvern,  BattleSide::DEFENDER, defenderHex, {75, 91, 94, 109}},
	SummonGuardiansCase{"defenderWideBearerNarrowGuardians",   wideNarrow,   pikeman, BattleSide::DEFENDER, defenderHex, {75, 76, 77, 92, 95, 109, 110, 111}},
	SummonGuardiansCase{"defenderWideBearerWideGuardians",     wideWide,     wyvern,  BattleSide::DEFENDER, defenderHex, {74, 76, 91, 95, 108, 110}},

	// a bearer with its back to the edge cannot be guarded from behind, so both flanking
	// guardians move to the front instead
	SummonGuardiansCase{"attackerAtOwnEdge", narrowWide, wyvern, BattleSide::ATTACKER, attackerEdgeHex, {104, 122, 138}},
	SummonGuardiansCase{"defenderAtOwnEdge", narrowWide, wyvern, BattleSide::DEFENDER, defenderEdgeHex, {115, 133}}
),
	[](const ::testing::TestParamInfo<SummonGuardiansCase> & info) { return info.param.name; });
