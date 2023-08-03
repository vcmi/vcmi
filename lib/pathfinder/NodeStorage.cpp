/*
 * NodeStorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NodeStorage.h"

#include "CPathfinder.h"
#include "PathfinderUtil.h"
#include "PathfinderOptions.h"

#include "../CPlayerState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

void NodeStorage::initialize(const PathfinderOptions & options, const CGameState * gs)
{
	//TODO: fix this code duplication with AINodeStorage::initialize, problem is to keep `resetTile` inline

	int3 pos;
	const PlayerColor player = out.hero->tempOwner;
	const int3 sizes = gs->getMapSize();
	const auto fow = static_cast<const CGameInfoCallback *>(gs)->getPlayerTeam(player)->fogOfWarMap;

	//make 200% sure that these are loop invariants (also a bit shorter code), let compiler do the rest(loop unswitching)
	const bool useFlying = options.useFlying;
	const bool useWaterWalking = options.useWaterWalking;

	for(pos.z=0; pos.z < sizes.z; ++pos.z)
	{
		for(pos.x=0; pos.x < sizes.x; ++pos.x)
		{
			for(pos.y=0; pos.y < sizes.y; ++pos.y)
			{
				const TerrainTile tile = gs->map->getTile(pos);
				if(tile.terType->isWater())
				{
					resetTile(pos, ELayer::SAIL, PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
					if(useWaterWalking)
						resetTile(pos, ELayer::WATER, PathfinderUtil::evaluateAccessibility<ELayer::WATER>(pos, tile, fow, player, gs));
				}
				if(tile.terType->isLand())
				{
					resetTile(pos, ELayer::LAND, PathfinderUtil::evaluateAccessibility<ELayer::LAND>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
				}
			}
		}
	}
}

std::vector<CGPathNode *> NodeStorage::calculateNeighbours(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;
	neighbours.reserve(16);
	auto accessibleNeighbourTiles = pathfinderHelper->getNeighbourTiles(source);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		for(EPathfindingLayer i = EPathfindingLayer::LAND; i < EPathfindingLayer::NUM_LAYERS; i.advance(1))
		{
			auto * node = getNode(neighbour, i);

			if(node->accessible == EPathAccessibility::NOT_SET)
				continue;

			neighbours.push_back(node);
		}
	}

	return neighbours;
}

std::vector<CGPathNode *> NodeStorage::calculateTeleportations(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;

	if(!source.isNodeObjectVisitable())
		return neighbours;

	auto accessibleExits = pathfinderHelper->getTeleportExits(source);

	for(auto & neighbour : accessibleExits)
	{
		auto * node = getNode(neighbour, source.node->layer);

		neighbours.push_back(node);
	}

	return neighbours;
}

NodeStorage::NodeStorage(CPathsInfo & pathsInfo, const CGHeroInstance * hero)
	:out(pathsInfo)
{
	out.hero = hero;
	out.hpos = hero->visitablePos();
}

void NodeStorage::resetTile(const int3 & tile, const EPathfindingLayer & layer, EPathAccessibility accessibility)
{
	getNode(tile, layer)->update(tile, layer, accessibility);
}

std::vector<CGPathNode *> NodeStorage::getInitialNodes()
{
	auto * initialNode = getNode(out.hpos, out.hero->boat ? out.hero->boat->layer : EPathfindingLayer::LAND);

	initialNode->turns = 0;
	initialNode->moveRemains = out.hero->movementPointsRemaining();
	initialNode->setCost(0.0);

	if(!initialNode->coord.valid())
	{
		initialNode->coord = out.hpos;
	}

	return std::vector<CGPathNode *> { initialNode };
}

void NodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	assert(destination.node != source.node->theNodeBefore); //two tiles can't point to each other
	destination.node->setCost(destination.cost);
	destination.node->moveRemains = destination.movementLeft;
	destination.node->turns = destination.turn;
	destination.node->theNodeBefore = source.node;
	destination.node->action = destination.action;
}

VCMI_LIB_NAMESPACE_END
