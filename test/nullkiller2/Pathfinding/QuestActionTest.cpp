/*
 * QuestActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"
#include "AI/Nullkiller2/Pathfinding/Actions/QuestAction.h"

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/callback/CCallback.h"
#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const int3 HERO_POS(4, 5, 0);
const int3 GATE_POS(4, 6, 0);
const int3 TARGET_POS(4, 8, 0);

TinyH3M::TinyH3MBuilder makeGateMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.hotaVersion(3)
		.size(36, false)
		.name("NK2QuestGate")
		.playerActive(PLAYER)
		.hero(HERO_POS + int3(1, 0, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.questGate(GATE_POS + int3(1, 0, 0), TinyH3M::TinyH3MBuilder::missionLevel(1));

	return builder;
}

class Nullkiller2_Pathfinding_QuestAction : public QuestTest
{
public:
	void revealMapAndEncloseHero()
	{
		revealMap(PLAYER);

		for(int dx = -1; dx <= 1; ++dx)
		{
			for(int dy = -1; dy <= 1; ++dy)
			{
				const int3 tile = HERO_POS + int3(dx, dy, 0);
				if(tile != HERO_POS && tile != GATE_POS)
					map->getTile(tile).terrainType = TerrainId(ETerrainId::ROCK);
			}
		}
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway()
	{
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), makeCallback(PLAYER));
		return gateway;
	}
};
}

TEST_F(Nullkiller2_Pathfinding_QuestAction, addsInitialVisitBeforeCrossingUnopenedGate)
{
	startWithMap(makeGateMap());
	revealMapAndEncloseHero();

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), HERO_POS);
	const auto gateObject = std::find_if(
		map->objects.begin(),
		map->objects.end(),
		[](const std::shared_ptr<CGObjectInstance> & object)
		{
			return object && object->visitablePos() == GATE_POS;
		});
	ASSERT_NE(gateObject, map->objects.end());
	ASSERT_TRUE((*gateObject)->passableFor(hero));
	const auto gateway = makeGateway();

	NK2AI::HeroMap<NK2AI::HeroRole> heroes;
	heroes[hero] = NK2AI::HeroRole::MAIN;
	NK2AI::PathfinderSettings settings;
	settings.allowBypassObjects = true;
	gateway->nullkiller->pathfinder->updatePaths(heroes, settings);

	const auto gatePaths = gateway->nullkiller->pathfinder->getPathInfo(GATE_POS);
	ASSERT_FALSE(gatePaths.empty()) << "the hero must be able to reach the gate";
	const auto paths = gateway->nullkiller->pathfinder->getPathInfo(TARGET_POS);
	ASSERT_FALSE(paths.empty()) << "the satisfiable gate should not block planning";

	const auto & path = paths.front();
	const auto actionNode = std::find_if(
		path.nodes.begin(),
		path.nodes.end(),
		[](const NK2AI::AIPathNodeInfo & node)
		{
			return dynamic_cast<const NK2AI::AIPathfinding::QuestAction *>(node.specialAction.get());
		});

	ASSERT_NE(actionNode, path.nodes.end())
		<< "an unopened gate must be visited before planning movement through it";
	EXPECT_FALSE(actionNode->actionIsBlocked);
}

TEST_F(Nullkiller2_Pathfinding_QuestAction, rejectsActionAfterQuestObjectWasRemoved)
{
	startWithMap(makeGateMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	const auto gateObject = std::find_if(
		map->objects.begin(),
		map->objects.end(),
		[](const std::shared_ptr<CGObjectInstance> & object)
		{
			return object && object->visitablePos() == GATE_POS;
		});
	ASSERT_NE(gateObject, map->objects.end());

	NK2AI::AIPathfinding::QuestAction questAction(QuestInfo((*gateObject)->id));
	const auto gateway = makeGateway();
	(*gateObject).reset();

	EXPECT_THROW(
		questAction.execute(gateway.get(), hero),
		NK2AI::cannotFulfillGoalException);
}
