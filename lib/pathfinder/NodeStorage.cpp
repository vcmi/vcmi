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
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"

void NodeStorage::initialize(const PathfinderOptions & options, const IGameInfoCallback & gameInfo)
{
	out.beginSearch();
	this->gameInfo = &gameInfo;
	player = out.hero->tempOwner;
	playerTeam = gameInfo.getPlayerTeam(player);
	useFlying = options.useFlying;
	useWaterWalking = options.useWaterWalking;
}

EPathAccessibility NodeStorage::evaluateAccessibility(
	const int3 & coord,
	const EPathfindingLayer & layer) const
{
	const TerrainTile * tile = gameInfo->getTile(coord);
	const bool isWater = tile->isWater();

	if((layer == ELayer::LAND && isWater)
		|| (layer == ELayer::SAIL && !isWater)
		|| (layer == ELayer::AIR && !useFlying)
		|| (layer == ELayer::WATER && (!isWater || !useWaterWalking)))
		return EPathAccessibility::NOT_SET;

	switch(layer.toEnum())
	{
	case ELayer::LAND:
		return PathfinderUtil::evaluateAccessibility<ELayer::LAND>(
			coord, *tile, playerTeam->fogOfWarMap, player, *gameInfo);
	case ELayer::SAIL:
		return PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(
			coord, *tile, playerTeam->fogOfWarMap, player, *gameInfo);
	case ELayer::WATER:
		return PathfinderUtil::evaluateAccessibility<ELayer::WATER>(
			coord, *tile, playerTeam->fogOfWarMap, player, *gameInfo);
	case ELayer::AIR:
		return PathfinderUtil::evaluateAccessibility<ELayer::AIR>(
			coord, *tile, playerTeam->fogOfWarMap, player, *gameInfo);
	case ELayer::AVIATE:
		return PathfinderUtil::evaluateAccessibility<ELayer::AVIATE>(
			coord, *tile, playerTeam->fogOfWarMap, player, *gameInfo);
	default:
		return EPathAccessibility::NOT_SET;
	}
}

CGPathNode * NodeStorage::getNode(const int3 & coord, const EPathfindingLayer & layer)
{
	auto * node = out.getNodeForWrite(coord, layer);
	if(!out.isCurrent(*node))
		resetTile(coord, layer, evaluateAccessibility(coord, layer));
	return node;
}

void NodeStorage::calculateNeighbours(
	std::vector<CGPathNode *> & result,
	const PathNodeInfo & source,
	EPathfindingLayer layer,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	NeighbourTilesVector accessibleNeighbourTiles;
	
	result.clear();
	
	pathfinderHelper->calculateNeighbourTiles(accessibleNeighbourTiles, source);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		auto * node = getNode(neighbour, layer);

		if(node->accessible == EPathAccessibility::NOT_SET)
			continue;

		result.push_back(node);
	}
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

		if(!node->coord.isValid())
		{
			logAi->debug("Teleportation exit is blocked " + neighbour.toString());
			continue;
		}

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
	auto * node = out.getNodeForWrite(tile, layer);
	node->update(tile, layer, accessibility);
	node->generation = out.currentGeneration;
}

std::vector<CGPathNode *> NodeStorage::getInitialNodes()
{
	auto * initialNode = getNode(out.hpos, out.hero->inBoat() ? out.hero->getBoat()->layer : EPathfindingLayer::LAND);

	initialNode->turns = 0;
	initialNode->moveRemains = out.hero->movementPointsRemaining();
	initialNode->setCost(0.0);

	if(!initialNode->coord.isValid())
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
