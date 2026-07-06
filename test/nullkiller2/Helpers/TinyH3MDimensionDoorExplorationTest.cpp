/*
 * TinyH3MDimensionDoorExplorationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/AdventureSpellCast.h"
#include "AI/Nullkiller2/Goals/Composition.h"
#include "AI/Nullkiller2/Helpers/ExplorationHelper.h"

#include "mock/TinyH3MBuilder.h"
#include "mock/mock_MapServiceTinyH3M.h"
#include "mock/mock_Services.h"

#include "lib/CPlayerState.h"
#include "lib/IGameSettings.h"
#include "lib/StartInfo.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/GameRandomizer.h"
#include "lib/filesystem/ResourcePath.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"
#include "lib/mapping/CMapHeader.h"
#include "lib/spells/CSpell.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const SpellID DIMENSION_DOOR = SpellID(8);

TinyH3M::TinyH3MBuilder makeDimensionDoorExplorationMap(bool withDimensionDoor)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("DDNullkillerExploration")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.heroPrimary(10, 10, 10, 50)
		.heroSecondarySkills({{SecondarySkill::AIR_MAGIC, 3}})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroSpells(withDimensionDoor ? std::vector<SpellID>{DIMENSION_DOOR} : std::vector<SpellID>{});

	return builder;
}

bool containsDimensionDoorCast(const NK2AI::Goals::TGoalVec & goals)
{
	for(const auto & goal : goals)
	{
		const auto * spellCast = dynamic_cast<const NK2AI::Goals::AdventureSpellCast *>(goal.get());
		if(spellCast && spellCast->getSpell()->getId() == DIMENSION_DOOR)
			return true;
	}

	return false;
}

class TinyH3MDimensionDoorExplorationTest : public ::testing::Test, public MapListener
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
		pendingOverrides.clear();
	}

	void mapLoaded(CMap * loadedMap) override
	{
		EXPECT_EQ(map, nullptr);
		map = loadedMap;

		for(const auto & [option, value] : pendingOverrides)
		{
			JsonNode node;
			node.Bool() = value;
			loadedMap->overrideGameSetting(option, node);
		}
	}

	void startWithMap(TinyH3M::TinyH3MBuilder builder)
	{
		auto bytes = builder.build();
		mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

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

	void overrideSettingBeforeInit(EGameSettings option, bool value)
	{
		pendingOverrides.push_back({option, value});
	}

	CGHeroInstance * findHeroByOwner(PlayerColor owner) const
	{
		for(const auto & obj : map->objects)
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(obj.get());
			if(hero && hero->getOwner() == owner)
				return hero;
		}
		return nullptr;
	}

	bool prepareForAdventureSpellPlanning(CGHeroInstance & hero) const
	{
		const CSpell * dimensionDoor = DIMENSION_DOOR.toSpell();
		if(!dimensionDoor)
			return false;

		const int spellCost = hero.getSpellCost(dimensionDoor);
		if(spellCost <= 0)
			return false;

		hero.mana = spellCost;
		hero.setMovementPoints(500);
		return true;
	}

	void setAllTilesVisible(PlayerColor player, bool visible)
	{
		auto & fow = teamState(player).fogOfWarMap;

		for(int z = 0; z < map->levels(); ++z)
		{
			for(int x = 0; x < map->width; ++x)
			{
				for(int y = 0; y < map->height; ++y)
					fow[int3(x, y, z)] = visible ? 1 : 0;
			}
		}
	}

	void setTileVisible(PlayerColor player, const int3 & tile, bool visible)
	{
		ASSERT_TRUE(map->isInTheMap(tile)) << tile.toString();
		teamState(player).fogOfWarMap[tile] = visible ? 1 : 0;
	}

	std::shared_ptr<CCallback> makeCallback(PlayerColor player)
	{
		return std::make_shared<CCallback>(gameState, std::optional<PlayerColor>{player}, nullptr);
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway(const std::shared_ptr<CCallback> & callback)
	{
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		return gateway;
	}

private:
	TeamState & teamState(PlayerColor player)
	{
		return gameState->teams.at(gameState->players.at(player).team);
	}

	struct PendingOverride
	{
		EGameSettings option;
		bool value;
	};

	std::vector<PendingOverride> pendingOverrides;

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	ServicesMock services;
	CMap * map = nullptr;
};
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedDimensionDoorHeroCastsForHiddenExploration)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, false);
	startWithMap(makeDimensionDoorExplorationMap(true));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_TRUE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setAllTilesVisible(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_TRUE(helper.canUseDimensionDoor());
	ASSERT_TRUE(helper.considerDimensionDoorExplorationTargets());
	const auto goals = helper.makeComposition()->decompose(gateway->nullkiller.get());

	EXPECT_TRUE(containsDimensionDoorCast(goals));
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedHeroWithoutDimensionDoorDoesNotCast)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, false);
	startWithMap(makeDimensionDoorExplorationMap(false));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_FALSE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setAllTilesVisible(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_FALSE(helper.canUseDimensionDoor());
	EXPECT_FALSE(helper.considerDimensionDoorExplorationTargets());
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedDimensionDoorHeroDoesNotProbeHiddenTilesWhenGuardedLandingsTrigger)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, true);
	startWithMap(makeDimensionDoorExplorationMap(true));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_TRUE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setAllTilesVisible(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_TRUE(helper.canUseDimensionDoor());
	EXPECT_FALSE(helper.considerDimensionDoorExplorationTargets());
}
