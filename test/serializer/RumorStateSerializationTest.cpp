/*
 * RumorStateSerializationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/gameState/RumorState.h"
#include "../../lib/serializer/CMemorySerializer.h"

TEST(RumorStateSerializationTest, modRumorStateRoundTrips)
{
	RumorState original;
	original.type = RumorState::TYPE_MOD;
	ASSERT_TRUE(original.updateModRumor("sample-mod", "rumor-1"));

	CMemorySerializer writer;
	writer.oser & original;

	CMemorySerializer reader(writer.extractBuffer());
	RumorState restored;
	reader.iser & restored;

	EXPECT_TRUE(reader.isFullyRead());
	EXPECT_EQ(restored.type, RumorState::TYPE_MOD);
	EXPECT_TRUE(restored.last.contains(RumorState::TYPE_MOD));
	EXPECT_EQ(restored.lastModRumor.first, "sample-mod");
	EXPECT_EQ(restored.lastModRumor.second, "rumor-1");
}

TEST(RumorStateSerializationTest, oldSaveWithoutRumorModIdDoesNotReadNewField)
{
	RumorState oldState;
	oldState.type = RumorState::TYPE_RAND;
	oldState.last[RumorState::TYPE_RAND] = {17, -1};
	oldState.lastModRumor = {"not-saved", "not-saved"};
	const std::string sentinel = "following-save-field";

	CMemorySerializer writer;
	writer.oser.version = ESerializationVersion::NEUTRAL_TOWN_EVENTS;
	writer.oser & oldState;
	writer.oser & sentinel;

	CMemorySerializer reader(writer.extractBuffer());
	reader.iser.version = ESerializationVersion::NEUTRAL_TOWN_EVENTS;
	RumorState restored;
	restored.lastModRumor = {"stale", "stale"};
	std::string restoredSentinel;
	reader.iser & restored;
	reader.iser & restoredSentinel;

	EXPECT_TRUE(reader.isFullyRead());
	EXPECT_EQ(restored.type, RumorState::TYPE_RAND);
	EXPECT_EQ(restored.last.at(RumorState::TYPE_RAND).first, 17);
	EXPECT_TRUE(restored.lastModRumor.first.empty());
	EXPECT_TRUE(restored.lastModRumor.second.empty());
	EXPECT_EQ(restoredSentinel, sentinel);
}

TEST(RumorStateSerializationTest, oldReplayBlobKeepsFollowingFieldAligned)
{
	RumorState oldState;
	oldState.type = RumorState::TYPE_MAP;
	oldState.last[RumorState::TYPE_MAP] = {3, -1};
	const std::string sentinel = "following-replay-field";

	CMemorySerializer writer;
	writer.oser.version = ESerializationVersion::NEUTRAL_TOWN_EVENTS;
	writer.oser & oldState;
	writer.oser & sentinel;

	CMemorySerializer reader(writer.extractBuffer());
	reader.iser.version = ESerializationVersion::CURRENT;
	RumorState restored;
	std::string restoredSentinel;
	reader.iser & restored;
	reader.iser & restoredSentinel;

	EXPECT_TRUE(reader.isFullyRead());
	EXPECT_EQ(restored.type, RumorState::TYPE_MAP);
	EXPECT_EQ(restored.last.at(RumorState::TYPE_MAP).first, 3);
	EXPECT_TRUE(restored.lastModRumor.first.empty());
	EXPECT_TRUE(restored.lastModRumor.second.empty());
	EXPECT_EQ(restoredSentinel, sentinel);
}
