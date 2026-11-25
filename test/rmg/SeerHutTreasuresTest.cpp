/*
 * SeerHutTreasuresTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/rewardable/Info.h"
#include "../../lib/rmg/ObjectInfo.h"
#include "../../lib/rmg/SeerHutTreasures.h"
#include "../mock/mock_IGameInfoCallback.h"

#include <limits>

namespace test
{
using namespace ::testing;

TEST(SeerHutTreasuresTest, AddSeerHutQuestStoresArtifactAndReward)
{
	/*
	 * Asserts: addQuest gives the hut one quest whose mission is the artifact and whose
	 * reward holds the given experience.
	 * Pass: allQuests size 1, mission.artifacts is that artifact, reward experience is 7000.
	 * Fail: no quest is added, the artifact is missing, or the reward is left unset.
	 */

	NiceMock<IGameInfoCallbackMock> gameInfo;
	auto hut = std::dynamic_pointer_cast<SeerHut>(LIBRARY->objtypeh->getHandlerFor(Obj::SEER_HUT, 0).create(&gameInfo, nullptr));
	ASSERT_NE(hut, nullptr);

	Rewardable::VisitInfo reward;
	reward.reward.heroExperience = 7000;
	reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	const ArtifactID artifact(ArtifactID::decode("centaurAxe"));
	SeerHutTreasures::addQuest(*hut, artifact, reward);

	ASSERT_EQ(hut->allQuests().size(), 1u);
	EXPECT_THAT(hut->getQuest().mission.artifacts, ElementsAre(artifact));
	ASSERT_TRUE(hut->getQuest().reward.has_value());
	EXPECT_EQ(hut->getQuest().reward->reward.heroExperience, 7000);
}

TEST(SeerHutTreasuresTest, PickSeerHutSlotsRespectsBudgetAndZoneLimit)
{
	/*
	 * Asserts: pickSlots never exceeds the slot budget, never picks an index more than
	 * its maxPerZone, and stops when every candidate is used up.
	 * Pass: 5 slots of two maxPerZone-1 candidates is exactly 2 picks; no candidates is empty.
	 * Fail: a candidate is over-used, or unused capacity is filled with extra picks.
	 */

	ObjectInfo first;
	first.probability = 1;
	first.maxPerZone = 1;
	ObjectInfo second;
	second.probability = 1;
	second.maxPerZone = 1;
	CRandomGenerator rng(1);

	auto picks = SeerHutTreasures::pickSlots({first, second}, 5, rng);
	EXPECT_LE(picks.size(), 5u);
	EXPECT_EQ(picks.size(), 2u);
	EXPECT_THAT(picks, UnorderedElementsAre(0u, 1u));

	EXPECT_THAT(SeerHutTreasures::pickSlots({}, 5, rng), IsEmpty());
}

TEST(SeerHutTreasuresTest, PickSeerHutSlotsNeverPicksZeroProbability)
{
	/*
	 * Asserts: a candidate with probability 0 is never picked when another candidate has weight.
	 * Pass: every pick of unlimited 0 and 1 is the second index.
	 * Fail: the zero-weight candidate appears in the result.
	 */

	ObjectInfo zero;
	zero.probability = 0;
	zero.maxPerZone = std::numeric_limits<ui32>::max();
	ObjectInfo one;
	one.probability = 1;
	one.maxPerZone = std::numeric_limits<ui32>::max();
	CRandomGenerator rng(1);

	auto picks = SeerHutTreasures::pickSlots({zero, one}, 8, rng);
	ASSERT_EQ(picks.size(), 8u);
	EXPECT_THAT(picks, Each(Eq(1u)));
}


}
