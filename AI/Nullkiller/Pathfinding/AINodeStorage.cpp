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
	nodes.resize(boost::extents[sizes.x][sizes.y][sizes.z][EPathfindingLayer::NUM_LAYERS][NUM_CHAINS]);
	dangerEvaluator.reset(new FuzzyHelper());
}

AINodeStorage::~AINodeStorage() = default;

void AINodeStorage::initialize(const PathfinderOptions & options, const CGameState * gs)
{
	//TODO: fix this code duplication with NodeStorage::initialize, problem is to keep `resetTile` inline

	int3 pos;
	const PlayerColor player = ai->playerID;
	const int3 sizes = gs->getMapSize();
	const auto & fow = static_cast<const CGameInfoCallback *>(gs)->getPlayerTeam(player)->fogOfWarMap;

	//make 200% sure that these are loop invariants (also a bit shorter code), let compiler do the rest(loop unswitching)
	const bool useFlying = options.useFlying;
	const bool useWaterWalking = options.useWaterWalking;

	for(pos.x=0; pos.x < sizes.x; ++pos.x)
	{
		for(pos.y=0; pos.y < sizes.y; ++pos.y)
		{
			for(pos.z=0; pos.z < sizes.z; ++pos.z)
			{
				const TerrainTile * tile = &gs->map->getTile(pos);
				switch(tile->terType)
				{
				case ETerrainType::ROCK:
					break;

				case ETerrainType::WATER:
					resetTile(pos, ELayer::SAIL, PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
					if(useWaterWalking)
						resetTile(pos, ELayer::WATER, PathfinderUtil::evaluateAccessibility<ELayer::WATER>(pos, tile, fow, player, gs));
					break;

				default:
					resetTile(pos, ELayer::LAND, PathfinderUtil::evaluateAccessibility<ELayer::LAND>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
					break;
				}
			}
		}
	}
}

void AINodeStorage::clear()
{
	actors.clear();
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

boost::optional<AIPathNode *> AINodeStorage::getOrCreateNode(
	const int3 & pos, 
	const EPathfindingLayer layer, 
	const ChainActor * actor)
{
	auto chains = nodes[pos.x][pos.y][pos.z][layer];

	for(AIPathNode & node : chains)
	{
		if(node.actor == actor)
		{
			return &node;
		}

		if(!node.actor)
		{
			node.actor = actor;

			return &node;
		}
	}

	return boost::none;
}

std::vector<CGPathNode *> AINodeStorage::getInitialNodes()
{
	std::vector<CGPathNode *> initialNodes;

	for(auto actorPtr : actors)
	{
		ChainActor * actor = actorPtr.get();
		AIPathNode * initialNode =
			getOrCreateNode(actor->initialPosition, actor->layer, actor)
			.get();

		initialNode->turns = actor->initialTurn;
		initialNode->moveRemains = actor->initialMovement;
		initialNode->danger = 0;
		initialNode->cost = 0.0;

		if(actor->isMovable)
		{
			initialNodes.push_back(initialNode);
		}
		else
		{
			initialNode->locked = true;
		}
	}

	return initialNodes;
}

void AINodeStorage::resetTile(const int3 & coord, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility)
{
	for(int i = 0; i < NUM_CHAINS; i++)
	{
		AIPathNode & heroNode = nodes[coord.x][coord.y][coord.z][layer][i];

		heroNode.actor = nullptr;
		heroNode.danger = 0;
		heroNode.manaCost = 0;
		heroNode.specialAction.reset();
		heroNode.armyLoss = 0;
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
		dstNode->cost = destination.cost;
		dstNode->danger = srcNode->danger;
		dstNode->action = destination.action;
		dstNode->theNodeBefore = srcNode->theNodeBefore;
		dstNode->manaCost = srcNode->manaCost;
		dstNode->armyLoss = srcNode->armyLoss;

		if(dstNode->specialAction && dstNode->actor)
		{
			dstNode->specialAction->applyOnDestination(dstNode->actor->hero, destination, source, dstNode, srcNode);
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
			auto nextNode = getOrCreateNode(neighbour, i, srcNode->actor);

			if(!nextNode || nextNode.get()->accessible == CGPathNode::NOT_SET)
				continue;

			neighbours.push_back(nextNode.get());
		}
	}

	if((source.node->layer == EPathfindingLayer::LAND || source.node->layer == EPathfindingLayer::SAIL)
		&& source.node->turns < 1)
	{
		addHeroChain(neighbours, srcNode);
	}
	
	return neighbours;
}

void AINodeStorage::addHeroChain(std::vector<CGPathNode *> & result, const AIPathNode * srcNode)
{
	auto chains = nodes[srcNode->coord.x][srcNode->coord.y][srcNode->coord.z][srcNode->layer];

	for(const AIPathNode & node : chains)
	{
		if(!node.locked || !node.actor || node.action == CGPathNode::ENodeAction::UNKNOWN && node.actor->hero)
		{
			continue;
		}

		addHeroChain(result, srcNode, &node);
		addHeroChain(result, &node, srcNode);
	}
}

void AINodeStorage::addHeroChain(
	std::vector<CGPathNode *> & result, 
	const AIPathNode * carrier, 
	const AIPathNode * other)
{
	if(carrier->actor->canExchange(other->actor))
	{
		bool hasLessMp = carrier->turns > other->turns || carrier->moveRemains < other->moveRemains;
		bool hasLessExperience = carrier->actor->hero->exp < other->actor->hero->exp;

		if(hasLessMp && hasLessExperience)
			return;

		auto newActor = carrier->actor->exchange(other->actor);
		auto chainNodeOptional = getOrCreateNode(carrier->coord, carrier->layer, newActor);

		if(!chainNodeOptional)
			return;

		auto chainNode = chainNodeOptional.get();

		if(chainNode->locked)
			return;

		chainNode->specialAction = newActor->getExchangeAction();

		result.push_back(chainNode);
	}
}

const CGHeroInstance * AINodeStorage::getHero(const CGPathNode * node) const
{
	auto aiNode = getAINode(node);

	return aiNode->actor->hero;
}

const std::set<const CGHeroInstance *> AINodeStorage::getAllHeroes() const
{
	std::set<const CGHeroInstance *> heroes;

	for(auto actor : actors)
	{
		if(actor->hero)
			heroes.insert(actor->hero);
	}

	return heroes;
}

void AINodeStorage::setHeroes(std::vector<HeroPtr> heroes, const VCAI * _ai)
{
	cb = _ai->myCb.get();
	ai = _ai;

	for(auto & hero : heroes)
	{
		uint64_t mask = 1 << actors.size();

		actors.push_back(std::make_shared<HeroActor>(hero.get(), mask));
	}
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
			auto node = getOrCreateNode(neighbour, source.node->layer, srcNode->actor);

			if(!node)
				continue;

			neighbours.push_back(node.get());
		}
	}

	if(source.isInitialPosition)
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
	auto hero = srcNode->actor->hero;

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

			auto nodeOptional = getOrCreateNode(targetTown->visitablePos(), EPathfindingLayer::LAND, srcNode->actor->castActor);

			if(nodeOptional)
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace("Adding town portal node at %s", targetTown->name);
#endif

				AIPathNode * node = nodeOptional.get();

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
	auto chains = nodes[pos.x][pos.y][pos.z][EPathfindingLayer::LAND];
	auto destinationNode = getAINode(destination.node);

	for(const AIPathNode & node : chains)
	{
		auto sameNode = node.actor == destinationNode->actor;

		if(sameNode	|| node.action == CGPathNode::ENodeAction::UNKNOWN)
		{
			continue;
		}

		if(node.danger <= destinationNode->danger && destinationNode->actor == node.actor->battleActor)
		{
			if(node.cost < destinationNode->cost)
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

bool AINodeStorage::isTileAccessible(const HeroPtr & hero, const int3 & pos, const EPathfindingLayer layer) const
{
	auto chains = nodes[pos.x][pos.y][pos.z][layer];

	for(const AIPathNode & node : chains)
	{
		if(node.action != CGPathNode::ENodeAction::UNKNOWN 
			&& node.actor && node.actor->hero == hero.h)
		{
			return true;
		}
	}

	return false;
}

std::vector<AIPath> AINodeStorage::getChainInfo(const int3 & pos, bool isOnLand) const
{
	std::vector<AIPath> paths;
	auto chains = nodes[pos.x][pos.y][pos.z][isOnLand ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL];

	for(const AIPathNode & node : chains)
	{
		if(node.action == CGPathNode::ENodeAction::UNKNOWN || !node.actor || !node.actor->hero)
		{
			continue;
		}

		AIPath path;
		const AIPathNode * current = &node;

		path.targetHero = node.actor->hero;
		auto initialPos = path.targetHero->visitablePos();

		while(current != nullptr && current->coord != initialPos)
		{
			AIPathNodeInfo pathNode;
			pathNode.cost = current->cost;
			pathNode.turns = current->turns;
			pathNode.danger = current->danger;
			pathNode.coord = current->coord;

			path.nodes.push_back(pathNode);
			path.specialAction = current->specialAction;

			current = getAINode(current->theNodeBefore);
		}

		path.targetObjectDanger = evaluateDanger(pos, path.targetHero);

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