/*
* AINodeStorage.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AINodeStorage.h"


AINodeStorage::AINodeStorage(const int3 & Sizes)
	: sizes(Sizes)
{
	nodes.resize(boost::extents[sizes.x][sizes.y][sizes.z][EPathfindingLayer::NUM_LAYERS][NUM_CHAINS]);
}

AINodeStorage::~AINodeStorage()
{
}

const AIPathNode * AINodeStorage::getAINode(const CGPathNode * node) const
{
	return static_cast<const AIPathNode *>(node);
}

void AINodeStorage::updateAINode(CGPathNode * node, std::function<void(AIPathNode *)> updater)
{
	auto aiNode = static_cast<AIPathNode *>(node);

	updater(aiNode);
}

bool AINodeStorage::isBattleNode(const CGPathNode * node) const
{
	return (getAINode(node)->chainMask & BATTLE_CHAIN) > 0;
}

boost::optional<AIPathNode *> AINodeStorage::getOrCreateNode(const int3 & pos, const EPathfindingLayer layer, int chainNumber)
{
	auto chains = nodes[pos.x][pos.y][pos.z][layer];

	for(AIPathNode & node : chains)
	{
		if(node.chainMask == chainNumber)
		{
			return &node;
		}

		if(node.chainMask == 0)
		{
			node.chainMask = chainNumber;

			return &node;
		}
	}

	return boost::none;
}

CGPathNode * AINodeStorage::getInitialNode()
{
	auto hpos = hero->getPosition(false);
	auto initialNode = 
		getOrCreateNode(hpos, hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND, NORMAL_CHAIN)
		.get();

	initialNode->turns = 0;
	initialNode->moveRemains = hero->movement;
	initialNode->danger = 0;

	return initialNode;
}

void AINodeStorage::resetTile(const int3 & coord, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility)
{
	for(int i = 0; i < NUM_CHAINS; i++)
	{
		AIPathNode & heroNode = nodes[coord.x][coord.y][coord.z][layer][i];

		heroNode.chainMask = 0;
		heroNode.danger = 0;
		heroNode.manaCost = 0;
		heroNode.specialAction.reset();
		heroNode.update(coord, layer, accessibility);
	}
}

void AINodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	const AIPathNode * srcNode = getAINode(source.node);

	updateAINode(destination.node, [&](AIPathNode * dstNode) {
		dstNode->moveRemains = destination.movementLeft;
		dstNode->turns = destination.turn;
		dstNode->danger = srcNode->danger;
		dstNode->action = destination.action;
		dstNode->theNodeBefore = srcNode->theNodeBefore;
		dstNode->manaCost = srcNode->manaCost;
		
		if(dstNode->specialAction)
		{
			dstNode->specialAction->applyOnDestination(getHero(), destination, source, dstNode, srcNode);
		}
	});
}

std::vector<CGPathNode *> AINodeStorage::calculateNeighbours(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;
	neighbours.reserve(16);
	const AIPathNode * srcNode = getAINode(source.node);
	auto accessibleNeighbourTiles = pathfinderHelper->getNeighbourTiles(source);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		for(EPathfindingLayer i = EPathfindingLayer::LAND; i <= EPathfindingLayer::AIR; i.advance(1))
		{
			auto nextNode = getOrCreateNode(neighbour, i, srcNode->chainMask);

			if(!nextNode || nextNode.get()->accessible == CGPathNode::NOT_SET)
				continue;

			neighbours.push_back(nextNode.get());
		}
	}

	return neighbours;
}

std::vector<CGPathNode *> AINodeStorage::calculateTeleportations(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;
	auto accessibleExits = pathfinderHelper->getTeleportExits(source);
	auto srcNode = getAINode(source.node);

	for(auto & neighbour : accessibleExits)
	{
		auto node = getOrCreateNode(neighbour, source.node->layer, srcNode->chainMask);

		if(!node)
			continue;

		neighbours.push_back(node.get());
	}

	return neighbours;
}

bool AINodeStorage::hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
{
	auto pos = destination.coord;
	auto chains = nodes[pos.x][pos.y][pos.z][EPathfindingLayer::LAND];
	auto destinationNode = getAINode(destination.node);

	for(const AIPathNode & node : chains)
	{
		auto sameNode = node.chainMask == destinationNode->chainMask;
		if(sameNode	|| node.action == CGPathNode::ENodeAction::UNKNOWN)
		{
			continue;
		}

		if(node.danger <= destinationNode->danger && destinationNode->chainMask == 1 && node.chainMask == 0)
		{
			if(node.turns < destinationNode->turns
				|| (node.turns == destinationNode->turns && node.moveRemains >= destinationNode->moveRemains))
			{
				logAi->trace(
					"Block ineficient move %s:->%s, mask=%i, mp diff: %i",
					source.coord.toString(),
					destination.coord.toString(),
					destinationNode->chainMask,
					node.moveRemains - destinationNode->moveRemains);

				return true;
			}
		}
	}

	return false;
}

std::vector<AIPath> AINodeStorage::getChainInfo(int3 pos, bool isOnLand) const
{
	std::vector<AIPath> paths;
	auto chains = nodes[pos.x][pos.y][pos.z][isOnLand ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL];
	auto initialPos = hero->visitablePos();

	for(const AIPathNode & node : chains)
	{
		if(node.action == CGPathNode::ENodeAction::UNKNOWN)
		{
			continue;
		}

		AIPath path;
		const AIPathNode * current = &node;

		while(current != nullptr && current->coord != initialPos)
		{
			AIPathNodeInfo pathNode;

			pathNode.movementPointsLeft = current->moveRemains;
			pathNode.movementPointsUsed = (int)(current->turns * hero->maxMovePoints(true) + hero->movement) - (int)current->moveRemains;
			pathNode.turns = current->turns;
			pathNode.danger = current->danger;
			pathNode.coord = current->coord;

			path.nodes.push_back(pathNode);
			path.specialAction = current->specialAction;

			current = getAINode(current->theNodeBefore);
		}

		paths.push_back(path);
	}

	return paths;
}

AIPath::AIPath()
	: nodes({})
{
}

int3 AIPath::firstTileToGet() const
{
	if(nodes.size())
	{
		return nodes.back().coord;
	}

	return int3(-1, -1, -1);
}

uint64_t AIPath::getPathDanger() const
{
	if(nodes.size())
	{
		return nodes.front().danger;
	}

	return 0;
}

uint32_t AIPath::movementCost() const
{
	if(nodes.size())
	{
		return nodes.front().movementPointsUsed;
	}

	// TODO: boost:optional?
	return 0;
}

uint64_t AIPath::getTotalDanger(HeroPtr hero) const
{
	uint64_t pathDanger = getPathDanger();
	uint64_t objDanger = evaluateDanger(nodes.front().coord, hero.get()); // bank danger is not checked by pathfinder
	uint64_t danger = pathDanger > objDanger ? pathDanger : objDanger;

	return danger;
}
