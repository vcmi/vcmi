/*
 * MapScriptTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/TinyH3MBuilder.h"
#include "../mock/mock_MapServiceTinyH3M.h"
#include "../mock/mock_Services.h"
#include "../mock/mock_IGameEventCallback.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapHeader.h"

#include <vcmi/ServerCallback.h>
#include <vcmi/scripting/MapEventDispatcher.h>

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

/// A map script whose init binds the handler `onVisit` to every Pandora's Box on the map,
/// discovering them by iterating the map objects and matching their instance-name prefix.
const std::string ATTACH_SCRIPT = R"lua(
local Map = {}

function Map:init(setup)
	for _, object in ipairs(setup:objects()) do
		if object:getInstanceName():sub(1, 10) == "pandoraBox" then
			setup:attachEventScript("onVisit", object:getInstanceName())
		end
	end
end

function Map:onVisit(game, server, object, hero)
end

return Map
)lua";

/// init that attaches to a name that does not exist - attachEventScript throws, init fails, but the
/// game must still load and no handler is bound.
const std::string BAD_NAME_SCRIPT = R"lua(
local Map = {}
function Map:init(setup)
	setup:attachEventScript("onVisit", "doesNotExist")
end
return Map
)lua";

/// Handler that pauses on a yes/no question: the branch grants gold (100 on yes, 5 on no), and a
/// sibling statement after the question grants 1 gem. The gem grant proves the sibling runs only
/// after the player answers - i.e. the blocking action really paused the coroutine.
const std::string QUESTION_SCRIPT = R"lua(
local Map = {}

function Map:init(setup)
	for _, object in ipairs(setup:objects()) do
		if object:getInstanceName():sub(1, 10) == "pandoraBox" then
			setup:attachEventScript("onVisit", object:getInstanceName())
		end
	end
end

function Map:onVisit(game, server, object, hero)
	local player = hero:getOwner()
	server:showQuestion{
		text = {append = {"question"}},
		mode = 1,
		images = {},
		onYes = function() server:giveResource(player, 6, 100) end,
		onNo  = function() server:giveResource(player, 6, 5) end,
	}
	server:giveResource(player, 5, 1) -- sibling after the question; must run only after the answer
end

return Map
)lua";

/// Handler that fights the visited pandora: startCombat replaces the box's garrison with 10 pikemen and
/// pauses; the post-combat resource grant must wait until the battle resolves.
const std::string COMBAT_SCRIPT = R"lua(
local Map = {}

function Map:init(setup)
	for _, object in ipairs(setup:objects()) do
		if object:getInstanceName():sub(1, 10) == "pandoraBox" then
			setup:attachEventScript("onVisit", object:getInstanceName())
		end
	end
end

function Map:onVisit(game, server, object, hero)
	local player = hero:getOwner()
	server:startCombat(hero, {
		{10, "core:pikeman"}, {0, ""}, {0, ""}, {0, ""}, {0, ""}, {0, ""}, {0, ""}
	})
	server:giveResource(player, 6, 777) -- post-combat; must not run until the battle resolves
end

return Map
)lua";

/// questEvents handler for a HOTA_SCRIPTED seer hut. Mirrors what the registerQuest helper (added to
/// the converter's helpersPrelude) does, but calls the primitives directly so the test doesn't depend
/// on the converter's generated text: first visit shows the proposal and marks/logs the quest, a
/// repeat visit shows progression instead.
const std::string SEER_HUT_QUEST_SCRIPT = R"lua(
local Map = {}

function Map:questEvents_7(game, server, object, hero)
	local player = hero:getOwner()
	if game:wasQuestProposed(object, player) then
		server:showMessage(player, {append = {"progression"}}, {})
	else
		server:markQuestProposed(object, player)
		server:addToQuestLog(object, player)
		server:showMessage(player, {append = {"proposal"}}, {})
	end
end

return Map
)lua";

/// questEvents handler that immediately finishes the quest.
const std::string SEER_HUT_FINISH_SCRIPT = R"lua(
local Map = {}

function Map:questEvents_7(game, server, object, hero)
	server:finishQuestOrRemoveObject(object)
end

return Map
)lua";
}

class MapScriptTest : public ::testing::Test, public ServerCallback, public MapListener
{
public:
	MapScriptTest()
		: gameEventCallback(std::make_shared<GameEventCallbackMock>(this))
	{
	}

	void SetUp() override
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(&services);
	}

	void TearDown() override
	{
		gameState.reset();
		mapService.reset();
		map = nullptr;
	}

	// ---- ServerCallback overrides ----
	bool describeChanges() const override { return true; }
	void apply(CPackForClient & pack) override { gameState->apply(pack); }
	void complain(const std::string & problem) override { FAIL() << "Server-side assertion: " << problem; }
	vstd::RNG * getRNG() override { return &randomGenerator; }
	void apply(BattleLogMessage &) override {}
	void apply(BattleStackMoved &) override {}
	void apply(BattleUnitsChanged &) override {}
	void apply(SetStackEffect &) override {}
	void apply(StacksInjured &) override {}
	void apply(BattleObstaclesChanged &) override {}
	void apply(CatapultAttack &) override {}

	void mapLoaded(CMap * loadedMap) override
	{
		EXPECT_EQ(map, nullptr);
		map = loadedMap;
	}

	void startWithMap(TinyH3M::TinyH3MBuilder builder)
	{
		std::string script = builder.script();
		auto bytes = builder.build();
		mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this, std::move(script));

		StartInfo si;
		si.mapname = "tiny";
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		auto header = mapService->loadMapHeader(ResourcePath(si.mapname));
		ASSERT_NE(header.get(), nullptr) << "TinyH3M scenario header failed to load";

		for(int i = 0; i < static_cast<int>(header->players.size()); ++i)
		{
			const PlayerInfo & pinfo = header->players[i];
			if(!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si.playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			pset.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			pset.name = "Player";
			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(mapService.get(), &si, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr) << "gameState init did not populate the CMap";
		gameEventCallback->setGameInfoCallback(gameState.get());
	}

	CGPandoraBox * findPandora() const
	{
		for(const auto & obj : map->objects)
			if(auto * pandora = dynamic_cast<CGPandoraBox *>(obj.get()))
				return pandora;
		return nullptr;
	}

	SeerHut * findSeerHut() const
	{
		for(const auto & obj : map->objects)
			if(auto * hut = dynamic_cast<SeerHut *>(obj.get()))
				return hut;
		return nullptr;
	}

	CGHeroInstance * findHero() const
	{
		for(const auto & obj : map->objects)
			if(auto * hero = dynamic_cast<CGHeroInstance *>(obj.get()))
				return hero;
		return nullptr;
	}

	int gold() const { return gameState->players.at(PLAYER).resources[GameResID::GOLD]; }
	int gems() const { return gameState->players.at(PLAYER).resources[GameResID::GEMS]; }

protected:
	std::shared_ptr<CGameState>            gameState;
	std::shared_ptr<GameEventCallbackMock> gameEventCallback;
	std::unique_ptr<MapServiceTinyH3M>     mapService;
	ServicesMock                           services;
	CMap *                                 map = nullptr;
	CRandomGenerator                       randomGenerator;
};

TEST_F(MapScriptTest, initBindsHandlerToPandoraByInstanceName)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.pandora({10, 10, 0})
		.withScript(ATTACH_SCRIPT);

	startWithMap(builder);

	CGPandoraBox * pandora = findPandora();
	ASSERT_NE(pandora, nullptr) << "the built map has no Pandora's Box";
	EXPECT_EQ(pandora->heroVisitScriptHandler, "onVisit")
		<< "init should have bound onVisit to the pandora via attachEventScript";
}

TEST_F(MapScriptTest, attachToUnknownObjectLeavesGameLoadable)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.pandora({10, 10, 0})
		.withScript(BAD_NAME_SCRIPT);

	startWithMap(builder);

	CGPandoraBox * pandora = findPandora();
	ASSERT_NE(pandora, nullptr);
	EXPECT_TRUE(pandora->heroVisitScriptHandler.empty())
		<< "a failed attach must not bind any handler";
}

TEST_F(MapScriptTest, showQuestionPausesUntilAnswered)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.pandora({10, 10, 0})
		.withScript(QUESTION_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	ASSERT_NE(dispatcher, nullptr);
	CGPandoraBox * pandora = findPandora();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(pandora, nullptr);
	ASSERT_NE(hero, nullptr);

	const int baseGold = gold();
	const int baseGems = gems();

	// Firing the handler must pause on the question: a dialog is shown and nothing after it has run.
	auto handle = dispatcher->onObjectVisit(*gameEventCallback, "onVisit", pandora, hero);
	ASSERT_TRUE(handle.has_value()) << "the handler must pause on showQuestion";
	EXPECT_EQ(gameEventCallback->blockingDialogs.size(), 1u) << "the question dialog must be shown";
	EXPECT_EQ(gold(), baseGold) << "no branch may run before the answer";
	EXPECT_EQ(gems(), baseGems) << "the sibling after showQuestion must not run before the answer";

	// Answering yes runs the onYes branch and then the sibling statement, in that order.
	bool finished = dispatcher->resumeCoroutine(*gameEventCallback, *handle, 1);
	EXPECT_TRUE(finished) << "the coroutine finishes once the reward is granted";
	EXPECT_EQ(gold(), baseGold + 100) << "onYes must grant 100 gold";
	EXPECT_EQ(gems(), baseGems + 1) << "the sibling after showQuestion must run after the answer";
}

TEST_F(MapScriptTest, showQuestionNoBranchRuns)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.pandora({10, 10, 0})
		.withScript(QUESTION_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGPandoraBox * pandora = findPandora();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(pandora, nullptr);
	ASSERT_NE(hero, nullptr);

	const int baseGold = gold();
	const int baseGems = gems();

	auto handle = dispatcher->onObjectVisit(*gameEventCallback, "onVisit", pandora, hero);
	ASSERT_TRUE(handle.has_value());

	bool finished = dispatcher->resumeCoroutine(*gameEventCallback, *handle, 0);
	EXPECT_TRUE(finished);
	EXPECT_EQ(gold(), baseGold + 5) << "onNo must grant 5 gold";
	EXPECT_EQ(gems(), baseGems + 1) << "the sibling after showQuestion must still run";
}

TEST_F(MapScriptTest, startCombatReplacesGarrisonAndPauses)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.pandora({10, 10, 0})
		.withScript(COMBAT_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGPandoraBox * pandora = findPandora();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(pandora, nullptr);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(pandora->stacksCount(), 0) << "the built pandora starts with no army";

	const int baseGold = gold();

	auto handle = dispatcher->onObjectVisit(*gameEventCallback, "onVisit", pandora, hero);
	ASSERT_TRUE(handle.has_value()) << "startCombat must pause the script";

	// The event army replaced the (empty) garrison, and the post-combat grant has not run yet.
	ASSERT_EQ(pandora->stacksCount(), 1) << "the event army must be placed into the pandora";
	ASSERT_NE(pandora->getCreature(SlotID(0)), nullptr);
	EXPECT_EQ(pandora->getCreature(SlotID(0))->getJsonKey(), "core:pikeman") << "the event creature must be placed";
	EXPECT_EQ(pandora->getStackCount(SlotID(0)), 10);
	EXPECT_EQ(gold(), baseGold) << "the post-combat grant must wait for the battle to resolve";
}

TEST_F(MapScriptTest, seerHutScriptedQuestTracksProposalThenProgression)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.seerHut({10, 10, 0}, TinyH3M::TinyH3MBuilder::missionScripted(7))
		.withScript(SEER_HUT_QUEST_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	SeerHut * hut = findSeerHut();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(hut, nullptr);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(hut->getActiveQuest(), nullptr) << "the built quest must classify as HOTA_SCRIPTED";
	EXPECT_EQ(hut->getActiveQuest()->missionKind, EQuestMission::HOTA_SCRIPTED);
	EXPECT_FALSE(hut->getActiveQuest()->isKnownTo(PLAYER));

	// First visit: not yet proposed -> proposal branch, marks the quest known and logs it.
	auto handle = dispatcher->onObjectVisit(*gameEventCallback, "questEvents_7", hut, hero);
	EXPECT_FALSE(handle.has_value()) << "this handler never blocks";
	EXPECT_TRUE(hut->getActiveQuest()->isKnownTo(PLAYER)) << "markQuestProposed must record the visit";
	EXPECT_EQ(gameState->players.at(PLAYER).quests.size(), 1u) << "addToQuestLog must add exactly one entry";
	ASSERT_EQ(gameEventCallback->infoWindows.size(), 1u);

	// Repeat visit: already proposed -> progression branch, no duplicate log entry.
	dispatcher->onObjectVisit(*gameEventCallback, "questEvents_7", hut, hero);
	EXPECT_EQ(gameState->players.at(PLAYER).quests.size(), 1u) << "addQuest is idempotent, no duplicate entries";
	ASSERT_EQ(gameEventCallback->infoWindows.size(), 2u);
}

TEST_F(MapScriptTest, finishQuestOrRemoveObjectEmptiesSeerHutWithoutRemovingIt)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.size(36, false)
		.name("MapScriptTest")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.seerHut({10, 10, 0}, TinyH3M::TinyH3MBuilder::missionScripted(7))
		.withScript(SEER_HUT_FINISH_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	SeerHut * hut = findSeerHut();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(hut, nullptr);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(hut->getActiveQuest(), nullptr);

	dispatcher->onObjectVisit(*gameEventCallback, "questEvents_7", hut, hero);

	EXPECT_EQ(hut->getActiveQuest(), nullptr) << "finishQuestOrRemoveObject must empty a seer hut's quest, not remove the hut";
	EXPECT_EQ(findSeerHut(), hut) << "the seer hut object itself must remain on the map";
}
