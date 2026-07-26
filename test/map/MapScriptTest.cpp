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

#include "../../lib/StartInfo.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapHeader.h"

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
}

class MapScriptTest : public ::testing::Test, public MapListener
{
public:
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
	}

	CGPandoraBox * findPandora() const
	{
		for(const auto & obj : map->objects)
			if(auto * pandora = dynamic_cast<CGPandoraBox *>(obj.get()))
				return pandora;
		return nullptr;
	}

protected:
	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	ServicesMock services;
	CMap * map = nullptr;
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
