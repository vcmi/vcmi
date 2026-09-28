/*
 * BattleMirrorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../server/battles/BattleMirrorServer.h"

#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/texts/MetaString.h"

class BattleMirrorTest : public BattleTestFixture
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
	}

protected:
	BattleMirrorController controller;
	std::vector<std::string> sinkFrames;

	void attachSink()
	{
		controller.setSink([this](std::string frame)
		{
			sinkFrames.push_back(std::move(frame));
		});
	}

	void apply(CPackForClient & pack)
	{
		controller.onPackApplied(pack, *server.gameState);
	}

	void applyStart(const BattleID & battleID)
	{
		BattleStart pack;
		pack.battleID = battleID;
		apply(pack);
	}

	void applyLog(const BattleID & battleID, const std::string & text)
	{
		BattleLogMessage pack;
		pack.battleID = battleID;

		MetaString line;
		line.appendRawString(text);
		pack.lines.push_back(std::move(line));
		apply(pack);
	}
};

TEST_F(BattleMirrorTest, LifecycleProducesFramesAndSummary)
{
	controller.setInterested(true);
	attachSink();

	applyStart(battle()->battleID);
	EXPECT_TRUE(controller.hasBattle());

	BattleSetActiveStack active;
	active.battleID = battle()->battleID;
	apply(active);

	applyLog(battle()->battleID, "phase one");

	BattleEnded ended;
	ended.battleID = battle()->battleID;
	ended.victor = PlayerColor(0);
	// terminal packs reach the mirror only after the game state applied them, which erases the
	// battle - the summary must still come out of the pack fields alone
	server.applyPack(ended);
	apply(ended);
	EXPECT_FALSE(controller.hasBattle());

	ASSERT_EQ(sinkFrames.size(), 4u) << "exactly one frame per applied pack";
	EXPECT_NE(sinkFrames[0].find("battle #0"), std::string::npos);
	EXPECT_NE(sinkFrames[2].find("phase one"), std::string::npos);
	EXPECT_NE(sinkFrames[3].find("Battle ended"), std::string::npos);
}

TEST_F(BattleMirrorTest, RingBoundedToEight)
{
	controller.setInterested(true);

	applyStart(battle()->battleID);

	for(const char * word : {"alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel", "india", "juliet", "kilo", "lima"})
		applyLog(battle()->battleID, word);

	const std::string frame = controller.snapshotFrame();
	EXPECT_NE(frame.find("lima"), std::string::npos);
	EXPECT_NE(frame.find("echo"), std::string::npos);
	EXPECT_EQ(frame.find("delta"), std::string::npos) << "overflowed entry leaked into the ring";
	EXPECT_EQ(frame.find("alpha"), std::string::npos) << "oldest entry leaked into the ring";
}

TEST_F(BattleMirrorTest, BattleSwitchResetsRing)
{
	controller.setInterested(true);
	attachSink();

	applyStart(battle()->battleID);
	applyLog(battle()->battleID, "alpha");
	ASSERT_EQ(sinkFrames.size(), 2u);

	const BattleID unknownBattle(1);
	applyStart(unknownBattle);
	EXPECT_EQ(sinkFrames.size(), 2u) << "battle missing from the game state is not rendered";
	EXPECT_TRUE(controller.snapshotFrame().empty());

	applyLog(unknownBattle, "bravo");

	applyStart(battle()->battleID);
	applyLog(battle()->battleID, "omega");
	ASSERT_EQ(sinkFrames.size(), 4u);

	const std::string & last = sinkFrames.back();
	EXPECT_NE(last.find("omega"), std::string::npos);
	EXPECT_EQ(last.find("alpha"), std::string::npos);
	EXPECT_EQ(last.find("bravo"), std::string::npos);
}

TEST_F(BattleMirrorTest, NotInterestedSkipsSinkButSnapshotWorks)
{
	attachSink();

	applyStart(battle()->battleID);
	applyLog(battle()->battleID, "silent");

	EXPECT_TRUE(sinkFrames.empty());
	EXPECT_TRUE(controller.hasBattle());
	EXPECT_NE(controller.snapshotFrame().find("silent"), std::string::npos);

	controller.setInterested(true);
	applyLog(battle()->battleID, "loud");

	ASSERT_EQ(sinkFrames.size(), 1u);
	EXPECT_NE(sinkFrames[0].find("loud"), std::string::npos);
}

TEST_F(BattleMirrorTest, NotInterestedTerminalPacksStillClearState)
{
	controller.setInterested(false);
	attachSink();

	applyStart(battle()->battleID);
	ASSERT_TRUE(controller.hasBattle());

	BattleEnded ended;
	ended.battleID = battle()->battleID;
	ended.victor = PlayerColor(0);
	server.applyPack(ended);
	apply(ended);
	EXPECT_TRUE(sinkFrames.empty());
	EXPECT_FALSE(controller.hasBattle());

	startBattle();

	applyStart(battle()->battleID);
	ASSERT_TRUE(controller.hasBattle());

	BattleCancelled cancelled;
	cancelled.battleID = battle()->battleID;
	server.applyPack(cancelled);
	apply(cancelled);
	EXPECT_TRUE(sinkFrames.empty());
	EXPECT_FALSE(controller.hasBattle());
}

TEST_F(BattleMirrorTest, NonBattlePacksIgnored)
{
	controller.setInterested(true);
	attachSink();

	MetaString text;
	text.appendRawString("server says");
	SystemMessage message(std::move(text));
	apply(message);

	EXPECT_TRUE(sinkFrames.empty());
	EXPECT_FALSE(controller.hasBattle());
	EXPECT_TRUE(controller.snapshotFrame().empty());
}

TEST_F(BattleMirrorTest, SnapshotWithoutBattle)
{
	EXPECT_EQ(controller.snapshotFrame(), "");
	EXPECT_FALSE(controller.hasBattle());
}

TEST_F(BattleMirrorTest, PortSettingIsRangeChecked)
{
	const JsonNode schema(JsonPath::builtin("config/schemas/settings.json"));
	const JsonNode & port = schema["properties"]["server"]["properties"]["battleMirror"]["properties"]["port"];

	EXPECT_TRUE(port["minimum"].isNumber());
	EXPECT_EQ(port["minimum"].Integer(), 0);
	EXPECT_TRUE(port["maximum"].isNumber());
	EXPECT_EQ(port["maximum"].Integer(), 65535);
}

TEST_F(BattleMirrorTest, ResetClearsMirroredState)
{
	controller.setInterested(true);
	attachSink();

	applyStart(battle()->battleID);
	applyLog(battle()->battleID, "stale");
	ASSERT_TRUE(controller.hasBattle());
	EXPECT_NE(controller.snapshotFrame().find("stale"), std::string::npos);

	controller.reset();

	EXPECT_FALSE(controller.hasBattle());
	EXPECT_TRUE(controller.snapshotFrame().empty());

	applyStart(battle()->battleID);
	EXPECT_EQ(controller.snapshotFrame().find("stale"), std::string::npos) << "log ring survived the reset";
}

TEST_F(BattleMirrorTest, CancelledBattleSendsTerminalFrameFromPackFields)
{
	controller.setInterested(true);
	attachSink();

	const BattleID id = battle()->battleID;
	applyStart(id);
	ASSERT_EQ(sinkFrames.size(), 1u);

	BattleCancelled cancelled;
	cancelled.battleID = id;
	// the game state erases the battle while applying the terminal pack, so the controller sees
	// the pack only after the battle it refers to is already gone
	server.applyPack(cancelled);
	apply(cancelled);

	ASSERT_EQ(sinkFrames.size(), 2u) << "one frame for the start, one terminal frame";
	EXPECT_NE(sinkFrames.back().find("Battle cancelled"), std::string::npos);
	EXPECT_FALSE(controller.hasBattle());

	applyLog(id, "ghost");
	EXPECT_EQ(sinkFrames.size(), 2u) << "no further frame once the battle is gone from the game state";
}

TEST_F(BattleMirrorTest, BattleEventPacksEachProduceAFrame)
{
	controller.setInterested(true);
	attachSink();

	applyStart(battle()->battleID);
	ASSERT_EQ(sinkFrames.size(), 1u);

	const auto applyEvent = [this](auto & pack)
	{
		pack.battleID = battle()->battleID;
		apply(pack);
	};

	BattleNextRound nextRound;
	applyEvent(nextRound);
	BattleStackMoved stackMoved;
	applyEvent(stackMoved);
	BattleUnitsChanged unitsChanged;
	applyEvent(unitsChanged);
	BattleAttack attackPack;
	applyEvent(attackPack);
	BattleSpellCast spellCast;
	applyEvent(spellCast);
	StacksInjured injured;
	applyEvent(injured);
	BattleObstaclesChanged obstaclesChanged;
	applyEvent(obstaclesChanged);
	CatapultAttack catapultAttack;
	applyEvent(catapultAttack);
	BattleSetStackProperty setStackProperty;
	applyEvent(setStackProperty);
	BattleTriggerEffect triggerEffect;
	applyEvent(triggerEffect);
	BattleUpdateGateState updateGateState;
	applyEvent(updateGateState);

	EXPECT_EQ(sinkFrames.size(), 12u) << "exactly one frame per event pack";
}
