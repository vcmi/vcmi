/*
 * TinyMapGameTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "TinyMapGameTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/networkPacks/PacksForClient.h"

void TinyMapGameTest::SetUp()
{
	currentGameState = std::make_shared<CGameState>();
	currentGameState->preInit(gameServices());
}

void TinyMapGameTest::TearDown()
{
	currentGameState.reset();
	mapService.reset();
	loadedMap = nullptr;
}

void TinyMapGameTest::complain(const std::string & problem)
{
	FAIL() << "Server-side assertion: " << problem;
}

void TinyMapGameTest::mapLoaded(CMap * loadedMap)
{
	EXPECT_EQ(this->loadedMap, nullptr);
	this->loadedMap = loadedMap;
	for(const auto & overrideValue : pendingOverrides)
	{
		JsonNode node;
		node.Bool() = overrideValue.value;
		loadedMap->overrideGameSetting(overrideValue.option, node);
	}
}

void TinyMapGameTest::startWithMap(TinyH3M::TinyH3MBuilder builder)
{
	startWithMap(std::move(builder), EMapDifficulty::EASY);
}

void TinyMapGameTest::startWithMap(TinyH3M::TinyH3MBuilder builder, EMapDifficulty difficulty)
{
	const auto * info = ::testing::UnitTest::GetInstance()->current_test_info();
	std::string fixtureName = std::string(info->test_suite_name()) + "_" + info->name();
	std::replace(fixtureName.begin(), fixtureName.end(), '/', '_');
	auto bytes = builder.buildAndDump(fixtureName);
	mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

	StartInfo startInfo;
	startInfo.mapname = "tiny";
	startInfo.difficulty = static_cast<ui8>(difficulty);
	startInfo.mode = EStartMode::NEW_GAME;

	auto header = mapService->loadMapHeader(ResourcePath(startInfo.mapname));
	ASSERT_NE(header.get(), nullptr) << "TinyH3M scenario header failed to load";

	for(int index = 0; index < static_cast<int>(header->players.size()); ++index)
	{
		const PlayerInfo & playerInfo = header->players[index];
		if(!(playerInfo.canHumanPlay || playerInfo.canComputerPlay))
			continue;

		PlayerSettings & settings = startInfo.playerInfos[PlayerColor(index)];
		settings.color = PlayerColor(index);
		settings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(index));
		settings.name = "Player";
		settings.castle = playerInfo.defaultCastle();
		settings.hero = playerInfo.defaultHero();
		configurePlayer(settings);
	}

	GameRandomizer randomizer(*currentGameState);
	Load::ProgressAccumulator progressTracker;
	currentGameState->init(mapService.get(), &startInfo, randomizer, progressTracker, false);

	ASSERT_NE(loadedMap, nullptr) << "gameState init did not populate the CMap";
	onMapStarted();
}

CGObjectInstance * TinyMapGameTest::findObjectAt(const int3 & pos) const
{
	for(const auto & object : loadedMap->objects)
	{
		if(object && object->anchorPos() == pos)
			return object.get();
	}
	return nullptr;
}

CGHeroInstance * TinyMapGameTest::findHeroAt(const int3 & pos) const
{
	for(const auto & object : loadedMap->objects)
	{
		auto * hero = dynamic_cast<CGHeroInstance *>(object.get());
		if(hero && hero->anchorPos() == pos)
			return hero;
	}
	return nullptr;
}

CGHeroInstance * TinyMapGameTest::findHeroByOwner(PlayerColor owner) const
{
	for(const auto & object : loadedMap->objects)
	{
		auto * hero = dynamic_cast<CGHeroInstance *>(object.get());
		if(hero && hero->getOwner() == owner)
			return hero;
	}
	return nullptr;
}

void TinyMapGameTest::revealMap(PlayerColor player)
{
	setMapVisibility(player, true);
}

void TinyMapGameTest::setMapVisibility(PlayerColor player, bool visible)
{
	auto * team = currentGameState->getPlayerTeam(player);
	ASSERT_NE(team, nullptr);

	for(int z = 0; z < loadedMap->levels(); ++z)
		for(int x = 0; x < loadedMap->width; ++x)
			for(int y = 0; y < loadedMap->height; ++y)
				team->fogOfWarMap[int3(x, y, z)] = visible ? 1 : 0;
}

std::shared_ptr<CCallback> TinyMapGameTest::makeCallback(PlayerColor player, IClient * client) const
{
	return std::make_shared<CCallback>(currentGameState, std::optional<PlayerColor>{ player }, client);
}

void TinyMapGameTest::grantResources(PlayerColor player, GameResID resource, int amount)
{
	SetResources resources;
	resources.mode = ChangeValueMode::RELATIVE;
	resources.player = player;
	resources.res[resource] = amount;
	currentGameState->apply(resources);
}

void TinyMapGameTest::overrideSettingBeforeInit(EGameSettings option, bool value)
{
	pendingOverrides.push_back({ option, value });
}
