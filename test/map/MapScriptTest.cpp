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
#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapHeader.h"

#include <vcmi/ServerCallback.h>
#include <vcmi/scripting/MapEventDispatcher.h>
#include <vcmi/scripting/Service.h>

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

/// A map script whose init binds the handler `onVisit` to every Pandora's Box on the map,
/// discovering them by iterating the map objects and matching their instance-name prefix.
constexpr auto ATTACH_SCRIPT = R"lua(
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
constexpr auto BAD_NAME_SCRIPT = R"lua(
local Map = {}
function Map:init(setup)
	setup:attachEventScript("onVisit", "doesNotExist")
end
return Map
)lua";

/// Handler that pauses on a yes/no question: the branch grants gold (100 on yes, 5 on no), and a
/// sibling statement after the question grants 1 gem. The gem grant proves the sibling runs only
/// after the player answers - i.e. the blocking action really paused the coroutine.
constexpr auto QUESTION_SCRIPT = R"lua(
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
		onYes = function() server:giveResource(player, LIBRARY:getResourceByName("gold"), 100) end,
		onNo  = function() server:giveResource(player, LIBRARY:getResourceByName("gold"), 5) end,
	}
	server:giveResource(player, LIBRARY:getResourceByName("gems"), 1) -- sibling after the question; must run only after the answer
end

return Map
)lua";

/// Handler that fights the visited pandora: startCombat replaces the box's garrison with 10 pikemen and
/// pauses; the post-combat resource grant must wait until the battle resolves.
constexpr auto COMBAT_SCRIPT = R"lua(
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
	server:giveResource(player, LIBRARY:getResourceByName("gold"), 777) -- post-combat; must not run until the battle resolves
end

return Map
)lua";

/// questEvents handler for a HOTA_SCRIPTED seer hut. Mirrors what the registerQuest helper (added to
/// the converter's helpersPrelude) does, but calls the primitives directly so the test doesn't depend
/// on the converter's generated text: first visit shows the proposal and marks/logs the quest, a
/// repeat visit shows progression instead.
constexpr auto SEER_HUT_QUEST_SCRIPT = R"lua(
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
constexpr auto SEER_HUT_FINISH_SCRIPT = R"lua(
local Map = {}

function Map:questEvents_7(game, server, object, hero)
	server:finishQuestOrRemoveObject(object)
end

return Map
)lua";

/// Object-identity predicates. Each handler is fired with the object of interest as `object`, so the
/// object's own instance name feeds the predicate - exercising getObjectByName resolution end to end.
/// Grants 100 gold on the true branch, 5 on the false branch (gems for the difficulty check).
constexpr auto PREDICATE_SCRIPT = R"lua(
local Map = {}

function Map:checkOwnsTown(game, server, object, hero)
	local player = hero:getOwner()
	local town = game:getObjectByName(object:getInstanceName())
	local owns = town ~= nil and town:getOwner() == player
	server:giveResource(player, LIBRARY:getResourceByName("gold"), owns and 100 or 5)
end

function Map:checkDefeatedMonster(game, server, object, hero)
	local player = hero:getOwner()
	local killed = game:playerDestroyedObject(player, object)
	server:giveResource(player, LIBRARY:getResourceByName("gold"), killed and 100 or 5)
end

return Map
)lua";

/// Spell/movement-point grants in each of the three modes. Fired with the hero as `object`.
constexpr auto GRANT_POINTS_SCRIPT = R"lua(
local Map = {}

function Map:grantPoints(game, server, object, hero)
	server:grantSpellPoints(hero, 5, 0)      -- add
	server:grantSpellPoints(hero, 3, 2)      -- set
	server:grantMovementPoints(hero, 100, 0) -- add
end

return Map
)lua";

/// townEvents handler that grows the tier-0 hire pool of the town it runs on.
constexpr auto HIRE_SCRIPT = R"lua(
local Map = {}

function Map:hire(game, server, town)
	server:grantCreaturesToHire(town, 0, 7)
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
	bool rollCombatAbility(const IBattleInfoCallback &, const battle::Unit &, int) override { return false; }
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

	/// Minimal playable map every test starts from: one active player with a hero, plus the script.
	/// Tests add the object under test on top.
	static TinyH3M::TinyH3MBuilder baseMap(EMapFormat format, const std::string & script)
	{
		TinyH3M::TinyH3MBuilder builder(format);
		builder
			.size(36, false)
			.name("MapScriptTest")
			.playerActive(PLAYER)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.withScript(script);
		return builder;
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

	CGTownInstance * findTown() const
	{
		for(const auto & obj : map->objects)
			if(auto * town = dynamic_cast<CGTownInstance *>(obj.get()))
				return town;
		return nullptr;
	}

	CGObjectInstance * findMonster() const
	{
		for(const auto & obj : map->objects)
			if(auto * monster = dynamic_cast<CGCreature *>(obj.get()))
				return monster;
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
	auto builder = baseMap(EMapFormat::SOD, ATTACH_SCRIPT);
	builder.pandora({10, 10, 0});

	startWithMap(builder);

	CGPandoraBox * pandora = findPandora();
	ASSERT_NE(pandora, nullptr) << "the built map has no Pandora's Box";
	EXPECT_EQ(pandora->heroVisitScriptHandler, "onVisit")
		<< "init should have bound onVisit to the pandora via attachEventScript";
}

TEST_F(MapScriptTest, attachToUnknownObjectLeavesGameLoadable)
{
	auto builder = baseMap(EMapFormat::SOD, BAD_NAME_SCRIPT);
	builder.pandora({10, 10, 0});

	startWithMap(builder);

	CGPandoraBox * pandora = findPandora();
	ASSERT_NE(pandora, nullptr);
	EXPECT_TRUE(pandora->heroVisitScriptHandler.empty())
		<< "a failed attach must not bind any handler";
}

TEST_F(MapScriptTest, showQuestionPausesUntilAnswered)
{
	auto builder = baseMap(EMapFormat::SOD, QUESTION_SCRIPT);
	builder.pandora({10, 10, 0});

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

TEST_F(MapScriptTest, startCombatReplacesGarrisonAndPauses)
{
	auto builder = baseMap(EMapFormat::SOD, COMBAT_SCRIPT);
	builder.pandora({10, 10, 0});

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
	auto builder = baseMap(EMapFormat::HOTA, SEER_HUT_QUEST_SCRIPT);
	builder.seerHut({10, 10, 0}, TinyH3M::TinyH3MBuilder::missionScripted(7));

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
	EXPECT_EQ(gameState->players.at(PLAYER).quests.size(), 1u) << "a repeat visit must not add a duplicate quest log entry";
	ASSERT_EQ(gameEventCallback->infoWindows.size(), 2u);
}

TEST_F(MapScriptTest, finishQuestOrRemoveObjectEmptiesSeerHutWithoutRemovingIt)
{
	auto builder = baseMap(EMapFormat::HOTA, SEER_HUT_FINISH_SCRIPT);
	builder.seerHut({10, 10, 0}, TinyH3M::TinyH3MBuilder::missionScripted(7));

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

TEST_F(MapScriptTest, playerOwnsTownResolvesObjectByName)
{
	auto builder = baseMap(EMapFormat::SOD, PREDICATE_SCRIPT);
	builder.randomTown({10, 10, 0}, PLAYER);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGTownInstance * town = findTown();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(town, nullptr) << "the built map has no town";
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(town->getOwner(), PLAYER);

	const int baseGold = gold();
	dispatcher->onObjectVisit(*gameEventCallback, "checkOwnsTown", town, hero);
	EXPECT_EQ(gold(), baseGold + 100) << "playerOwnsTown must resolve the town by name and see PLAYER owns it";
}

TEST_F(MapScriptTest, playerDefeatedMonsterReadsDestroyedObjects)
{
	auto builder = baseMap(EMapFormat::SOD, PREDICATE_SCRIPT);
	builder.monster({10, 10, 0}, CreatureID(0), 10);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGObjectInstance * monster = findMonster();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(monster, nullptr) << "the built map has no monster";
	ASSERT_NE(hero, nullptr);

	const int baseGold = gold();

	// Not defeated yet -> false branch grants 5 gold.
	dispatcher->onObjectVisit(*gameEventCallback, "checkDefeatedMonster", monster, hero);
	EXPECT_EQ(gold(), baseGold + 5) << "an undefeated monster must read as not-defeated";

	// Record the defeat and fire again -> true branch grants 100 gold.
	gameState->players.at(PLAYER).destroyedObjects.insert(monster->id);
	dispatcher->onObjectVisit(*gameEventCallback, "checkDefeatedMonster", monster, hero);
	EXPECT_EQ(gold(), baseGold + 5 + 100) << "a monster in the player's destroyedObjects must read as defeated";
}

TEST_F(MapScriptTest, grantPointsRespectMode)
{
	auto builder = baseMap(EMapFormat::SOD, GRANT_POINTS_SCRIPT);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGHeroInstance * hero = findHero();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(hero, nullptr);

	const int baseMana = hero->mana;
	const int baseMove = hero->movementPointsRemaining();

	dispatcher->onObjectVisit(*gameEventCallback, "grantPoints", hero, hero);

	ASSERT_EQ(gameEventCallback->manaPointsSet.size(), 2u);
	EXPECT_EQ(gameEventCallback->manaPointsSet[0].second, baseMana + 5) << "mode 0 adds to the current mana";
	EXPECT_EQ(gameEventCallback->manaPointsSet[1].second, 3)            << "mode 2 sets the total directly";
	ASSERT_EQ(gameEventCallback->movePointsSet.size(), 1u);
	EXPECT_EQ(gameEventCallback->movePointsSet[0].second, baseMove + 100) << "mode 0 adds to the current movement";
}

TEST_F(MapScriptTest, grantCreaturesToHireChangesTownPool)
{
	auto builder = baseMap(EMapFormat::SOD, HIRE_SCRIPT);
	builder.randomTown({10, 10, 0}, PLAYER);

	startWithMap(builder);

	auto * dispatcher = gameState->getMapEventDispatcher();
	CGTownInstance * town = findTown();
	ASSERT_NE(dispatcher, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_FALSE(town->creatures.empty());

	const int base = town->creatures[0].first;

	dispatcher->onTownTurnStart(*gameEventCallback, "hire", town);

	EXPECT_EQ(town->creatures[0].first, base + 7) << "grantCreaturesToHire adds to the town's tier-0 pool";
}
