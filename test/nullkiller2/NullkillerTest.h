/*
 * NullkillerTest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../mock/TinyMapGameTest.h"

#include "../../AI/Nullkiller2/AIGateway.h"
#include "../../AI/Nullkiller2/Engine/Nullkiller.h"

namespace NK2AI
{
/// Exposes private Nullkiller steps of makeTurn to tests
class NullkillerTestAccess
{
public:
	static bool executeTask(Nullkiller & ai, const Goals::TTask & task)
	{
		return ai.executeTask(task);
	}

	static void prepareState(Nullkiller & ai)
	{
		ai.resetState();
		ai.updateState();
	}
};
}

class NullkillerTest : public TinyMapGameTest
{
protected:
	static NK2AI::AIPathNodeInfo pathNode(const CGHeroInstance & hero, const int3 & coordinate)
	{
		NK2AI::AIPathNodeInfo node{};
		node.coord = coordinate;
		node.layer = EPathfindingLayer::LAND;
		node.targetHero = &hero;
		node.parentIndex = -1;
		node.chainMask = 1;
		node.turns = 0;
		return node;
	}

	/// Refreshes hero roles and single-hero paths, as a planning pass does before decomposing behaviors
	void preparePlanning(NK2AI::AIGateway & gateway)
	{
		auto & ai = *gateway.nullkiller;
		ai.heroManager->update();

		NK2AI::PathfinderSettings settings;
		settings.useHeroChain = false;
		settings.useDimensionDoor = false;
		ai.pathfinder->updatePaths(ai.getHeroesForPathfinding(), settings);
		ai.decomposer->reset();
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway(const std::shared_ptr<CCallback> & callback)
	{
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		return gateway;
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway(PlayerColor player, IClient * client = nullptr)
	{
		return makeGateway(makeCallback(player, client));
	}
};
