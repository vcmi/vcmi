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
#include "Actions/TownPortalAction.h"
#include "../Goals/Goals.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/PathfinderUtil.h"
#include "../../../lib/CPlayerState.h"

AINodeStorage::AINodeStorage(const int3 & Sizes)
	: sizes(Sizes)
{
	nodes.resize(boost::extents[EPathfindingLayer::NUM_LAYERS][sizes.z][sizes.x][sizes.y][NUM_CHAINS]);
	dangerEvaluator.reset(new FuzzyHelper());
}

AINodeStorage::~AINodeStorage() = default;

void AINodeStorage::initialize(const PathfinderOptions & options, const CGameState * gs)
{
	int3 pos;
	const int3 sizes = gs->getMapSize();
	const auto & fow = static_cast<const CGameInfoCallback *>(gs)->getPlayerTeam(hero->tempOwner)->fogOfWarMap;
	const PlayerColor player = hero->tempOwner;

	//make 200% sure that these are loop invariants (also a bit shorter code), let compiler do the rest(loop unswitching)
	const bool useFlying = options.useFlying;
	const bool useWaterWalking = options.useWaterWalking;

	for(pos.z=0; pos.z < sizes.z; ++pos.z)
	{
		for(pos.x=0; pos.x < sizes.x; ++pos.x)
		{
			for(pos.y=0; pos.y < sizes.y; ++pos.y)
			{
				const TerrainTile & tile = gs->map->getTile(pos);
				if(!tile.terType->isPassable())
					continue;
				
				if(tile.terType->isWater())
				{
					resetTile(pos, ELayer::SAIL, PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
					if(useWaterWalking)
						resetTile(pos, ELayer::WATER, PathfinderUtil::evaluateAccessibility<ELayer::WATER>(pos, tile, fow, player, gs));
				}
				else
				{
					resetTile(pos, ELayer::LAND, PathfinderUtil::evaluateAccessibility<ELayer::LAND>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
				}
			}
		}
	}
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

std::optional<AIPathNode *> AINodeStorage::getOrCreateNode(const int3 & pos, const EPathfindingLayer layer, int chainNumber)
{
	auto chains = nodes[layer][pos.z][pos.x][pos.y];

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

	return std::nullopt;
}

std::vector<CGPathNode *> AINodeStorage::getInitialNodes()
{
	auto hpos = hero->visitablePos();
	auto initialNode = getOrCreateNode(hpos, hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND, NORMAL_CHAIN).value();

	initialNode->turns = 0;
	initialNode->moveRemains = hero->movement;
	initialNode->danger = 0;
	initialNode->setCost(0.0);

	return {initialNode};
}

void AINodeStorage::resetTile(const int3 & coord, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility)
{
	for(int i = 0; i < NUM_CHAINS; i++)
	{
		AIPathNode & heroNode = nodes[layer][coord.z][coord.x][coord.y][i];

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

	updateAINode(destination.node, [&](AIPathNode * dstNode)
	{
		dstNode->moveRemains = destination.movementLeft;
		dstNode->turns = destination.turn;
		dstNode->setCost(destination.cost);
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
		for(EPathfindingLayer i = EPathfindingLayer::LAND; i < EPathfindingLayer::NUM_LAYERS; i.advance(1))
		{
			auto nextNode = getOrCreateNode(neighbour, i, srcNode->chainMask);

			if(!nextNode || nextNode.value()->accessible == CGPathNode::NOT_SET)
				continue;

			neighbours.push_back(nextNode.value());
		}
	}

	return neighbours;
}

void AINodeStorage::setHero(HeroPtr heroPtr, const VCAI * _ai)
{
	hero = heroPtr.get();
	cb = _ai->myCb.get();
	ai = _ai;
}

std::vector<CGPathNode *> AINodeStorage::calculateTeleportations(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;

	if(source.isNodeObjectVisitable())
	{
		auto accessibleExits = pathfinderHelper->getTeleportExits(source);
		auto srcNode = getAINode(source.node);

		for(auto & neighbour : accessibleExits)
		{
			auto node = getOrCreateNode(neighbour, source.node->layer, srcNode->chainMask);

			if(!node)
				continue;

			neighbours.push_back(node.value());
		}
	}

	if(hero->visitablePos() == source.coord)
	{
		calculateTownPortalTeleportations(source, neighbours);
	}

	return neighbours;
}

void AINodeStorage::calculateTownPortalTeleportations(
	const PathNodeInfo & source,
	std::vector<CGPathNode *> & neighbours)
{
	SpellID spellID = SpellID::TOWN_PORTAL;
	const CSpell * townPortal = spellID.toSpell();
	auto srcNode = getAINode(source.node);

	if(hero->canCastThisSpell(townPortal) && hero->mana >= hero->getSpellCost(townPortal))
	{
		auto towns = cb->getTownsInfo(false);

		vstd::erase_if(towns, [&](const CGTownInstance * t) -> bool
		{
			return cb->getPlayerRelations(hero->tempOwner, t->tempOwner) == PlayerRelations::ENEMIES;
		});

		if(!towns.size())
		{
			return;
		}

		// TODO: Copy/Paste from TownPortalMechanics
		auto skillLevel = hero->getSpellSchoolLevel(townPortal);
		auto movementCost = GameConstants::BASE_MOVEMENT_COST * (skillLevel >= 3 ? 2 : 3);

		if(hero->movement < movementCost)
		{
			return;
		}

		if(skillLevel < SecSkillLevel::ADVANCED)
		{
			const CGTownInstance * nearestTown = *vstd::minElementByFun(towns, [&](const CGTownInstance * t) -> int
			{
				return hero->visitablePos().dist2dSQ(t->visitablePos());
			});

			towns = std::vector<const CGTownInstance *>{ nearestTown };
		}

		for(const CGTownInstance * targetTown : towns)
		{
			if(targetTown->visitingHero)
				continue;

			auto nodeOptional = getOrCreateNode(targetTown->visitablePos(), EPathfindingLayer::LAND, srcNode->chainMask | CAST_CHAIN);

			if(nodeOptional)
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace("Adding town portal node at %s", targetTown->name);
#endif

				AIPathNode * node = nodeOptional.value();

				node->theNodeBefore = source.node;
				node->specialAction.reset(new AIPathfinding::TownPortalAction(targetTown));
				node->moveRemains = source.node->moveRemains;
				
				neighbours.push_back(node);
			}
		}
	}
}

bool AINodeStorage::hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
{
	auto pos = destination.coord;
	auto chains = nodes[EPathfindingLayer::LAND][pos.z][pos.x][pos.y];
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
			if(node.getCost() < destinationNode->getCost())
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace(
					"Block ineficient move %s:->%s, mask=%i, mp diff: %i",
					source.coord.toString(),
					destination.coord.toString(),
					destinationNode->chainMask,
					node.moveRemains - destinationNode->moveRemains);
#endif
				return true;
			}
		}
	}

	return false;
}

bool AINodeStorage::isTileAccessible(const int3 & pos, const EPathfindingLayer layer) const
{
	const AIPathNode & node = nodes[layer][pos.z][pos.x][pos.y][0];

	return node.action != CGPathNode::ENodeAction::UNKNOWN;
}

std::vector<AIPath> AINodeStorage::getChainInfo(const int3 & pos, bool isOnLand) const
{
	std::vector<AIPath> paths;
	auto chains = nodes[isOnLand ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL][pos.z][pos.x][pos.y];
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
			pathNode.cost = current->getCost();
			pathNode.turns = current->turns;
			pathNode.danger = current->danger;
			pathNode.coord = current->coord;

			path.nodes.push_back(pathNode);
			path.specialAction = current->specialAction;

			current = getAINode(current->theNodeBefore);
		}

		path.targetObjectDanger = evaluateDanger(pos);

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

float AIPath::movementCost() const
{
	if(nodes.size())
	{
		return nodes.front().cost;
	}

	// TODO: boost:optional?
	return 0.0;
}

uint64_t AIPath::getTotalDanger(HeroPtr hero) const
{
	uint64_t pathDanger = getPathDanger();
	uint64_t danger = pathDanger > targetObjectDanger ? pathDanger : targetObjectDanger;

	return danger;
}
