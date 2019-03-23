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
	if(heroChainPass)
		return;

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
	heroChainPass = false;
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
	if(heroChainPass)
		return heroChain;

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
		initialNode->cost = actor->initialTurn;
		initialNode->action = CGPathNode::ENodeAction::NORMAL;

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
		heroNode.chainOther = nullptr;
		heroNode.update(coord, layer, accessibility);
	}
}

void AINodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	const AIPathNode * srcNode = getAINode(source.node);

	updateAINode(destination.node, [&](AIPathNode * dstNode)
	{
		commit(dstNode, srcNode, destination.action, destination.turn, destination.movementLeft, destination.cost);

		if(srcNode->specialAction || srcNode->chainOther)
		{
			// there is some action on source tile which should be performed before we can bypass it
			destination.node->theNodeBefore = source.node;
		}

		if(dstNode->specialAction && dstNode->actor)
		{
			dstNode->specialAction->applyOnDestination(dstNode->actor->hero, destination, source, dstNode, srcNode);
		}

#ifdef VCMI_TRACE_PATHFINDER_EX
		logAi->trace(
			"Commited %s -> %s, cost: %f, hero: %s, mask: %i", 
			source.coord.toString(),
			destination.coord.toString(),
			destination.cost,
			dstNode->actor->toString(),
			dstNode->actor->chainMask);
#endif
	});
}

void AINodeStorage::commit(
	AIPathNode * destination, 
	const AIPathNode * source, 
	CGPathNode::ENodeAction action, 
	int turn, 
	int movementLeft, 
	float cost) const
{
	destination->action = action;
	destination->cost = cost;
	destination->moveRemains = movementLeft;
	destination->turns = turn;
	destination->armyLoss = source->armyLoss;
	destination->manaCost = source->manaCost;
	destination->danger = source->danger;
	destination->theNodeBefore = source->theNodeBefore;
	destination->chainOther = nullptr;
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
	
	return neighbours;
}

bool AINodeStorage::calculateHeroChain()
{
	heroChainPass = true;
	heroChain.resize(0);

	std::vector<AIPathNode *> existingChains;
	std::vector<ExchangeCandidate> newChains;

	existingChains.reserve(NUM_CHAINS);
	newChains.reserve(NUM_CHAINS);

	foreach_tile_pos([&](const int3 & pos) {
		auto layer = EPathfindingLayer::LAND;
		auto chains = nodes[pos.x][pos.y][pos.z][layer];

		existingChains.resize(0);
		newChains.resize(0);

		for(AIPathNode & node : chains)
		{
			if(node.coord.x == 60 && node.coord.y == 56 && node.actor)
				logAi->trace(node.actor->toString());

			if(node.turns <= heroChainMaxTurns && node.action != CGPathNode::ENodeAction::UNKNOWN)
				existingChains.push_back(&node);
		}

		for(AIPathNode * node : existingChains)
		{
			if(node->actor->hero)
			{
				calculateHeroChain(node, existingChains, newChains);
			}
		}

		cleanupInefectiveChains(newChains);
		addHeroChain(newChains);
	});

	return heroChain.size();
}

void AINodeStorage::cleanupInefectiveChains(std::vector<ExchangeCandidate> & result) const
{
	vstd::erase_if(result, [&](ExchangeCandidate & chainInfo) -> bool
	{
		auto pos = chainInfo.coord;
		auto chains = nodes[pos.x][pos.y][pos.z][EPathfindingLayer::LAND];

		return hasBetterChain(chainInfo.carrierParent, &chainInfo, chains)
			|| hasBetterChain(chainInfo.carrierParent, &chainInfo, result);
	});
}

void AINodeStorage::calculateHeroChain(
	AIPathNode * srcNode, 
	const std::vector<AIPathNode *> & variants, 
	std::vector<ExchangeCandidate> & result) const
{
	for(AIPathNode * node : variants)
	{
		if(node == srcNode || !node->actor || node->turns > heroChainMaxTurns 
			|| node->action == CGPathNode::ENodeAction::UNKNOWN && node->actor->hero)
		{
			continue;
		}

#ifdef VCMI_TRACE_PATHFINDER_EX
		logAi->trace(
			"Thy exchange %s[%i] -> %s[%i] at %s",
			node->actor->toString(),
			node->actor->chainMask,
			srcNode->actor->toString(),
			srcNode->actor->chainMask,
			srcNode->coord.toString());
#endif

		calculateHeroChain(srcNode, node, result);
	}
}

void AINodeStorage::calculateHeroChain(
	AIPathNode * carrier, 
	AIPathNode * other, 
	std::vector<ExchangeCandidate> & result) const
{
	if(!carrier->actor->isMovable)
		return;
	
	if(carrier->actor->canExchange(other->actor))
	{
#ifdef VCMI_TRACE_PATHFINDER_EX
		logAi->trace(
			"Exchange allowed %s[%i] -> %s[%i] at %s",
			other->actor->toString(),
			other->actor->chainMask,
			carrier->actor->toString(),
			carrier->actor->chainMask,
			carrier->coord.toString());
#endif

		bool hasLessMp = carrier->turns > other->turns || carrier->moveRemains < other->moveRemains;
		bool hasLessExperience = carrier->actor->hero->exp < other->actor->hero->exp;

		if(hasLessMp && hasLessExperience)
		{
#ifdef VCMI_TRACE_PATHFINDER_EX
			logAi->trace("Exchange at %s is ineficient. Blocked.", carrier->coord.toString());
#endif
			return;
		}

		auto newActor = carrier->actor->exchange(other->actor);
		
		result.push_back(calculateExchange(newActor, carrier, other));
	}
}

void AINodeStorage::addHeroChain(const std::vector<ExchangeCandidate> & result)
{
	for(const ExchangeCandidate & chainInfo : result)
	{
		auto carrier = chainInfo.carrierParent;
		auto newActor = chainInfo.actor;
		auto other = chainInfo.otherParent;
		auto chainNodeOptional = getOrCreateNode(carrier->coord, carrier->layer, newActor);

		if(!chainNodeOptional)
		{
#ifdef VCMI_TRACE_PATHFINDER_EX
			logAi->trace("Exchange at %s can not allocate node. Blocked.", carrier->coord.toString());
#endif
			continue;
		}

		auto exchangeNode = chainNodeOptional.get();

		if(exchangeNode->action != CGPathNode::ENodeAction::UNKNOWN)
		{
#ifdef VCMI_TRACE_PATHFINDER_EX
			logAi->trace("Exchange at %s node is already in use. Blocked.", carrier->coord.toString());
#endif
			continue;
		}
		
		if(exchangeNode->turns != 0xFF && exchangeNode->cost < chainInfo.cost)
		{
#ifdef VCMI_TRACE_PATHFINDER_EX
			logAi->trace(
				"Exchange at %s is is not effective enough. %f < %f", 
				exchangeNode->coord.toString(), 
				exchangeNode->cost, 
				chainInfo.cost);
#endif
			continue;
		}

		commit(exchangeNode, carrier, carrier->action, chainInfo.turns, chainInfo.moveRemains, chainInfo.cost);

		exchangeNode->chainOther = other;
		exchangeNode->armyLoss = chainInfo.armyLoss;

#ifdef VCMI_TRACE_PATHFINDER_EX
		logAi->trace(
			"Chain accepted at %s %s -> %s, mask %i, cost %f", 
			exchangeNode->coord.toString(), 
			other->actor->toString(), 
			exchangeNode->actor->toString(),
			exchangeNode->actor->chainMask,
			exchangeNode->cost);
#endif
		heroChain.push_back(exchangeNode);
	}
}

ExchangeCandidate AINodeStorage::calculateExchange(
	ChainActor * exchangeActor, 
	AIPathNode * carrierParentNode, 
	AIPathNode * otherParentNode) const
{
	ExchangeCandidate candidate;

	auto carrierActor = carrierParentNode->actor;
	auto otherActor = otherParentNode->actor;

	candidate.layer = carrierParentNode->layer;
	candidate.coord = carrierParentNode->coord;
	candidate.carrierParent = carrierParentNode;
	candidate.otherParent = otherParentNode;
	candidate.actor = exchangeActor;
	candidate.armyLoss = carrierParentNode->armyLoss + otherParentNode->armyLoss;
	candidate.turns = carrierParentNode->turns;
	candidate.cost = carrierParentNode->cost + otherParentNode->cost / 1000.0;
	candidate.moveRemains = carrierParentNode->moveRemains;

	if(carrierParentNode->turns < otherParentNode->turns)
	{
		int moveRemains = exchangeActor->hero->maxMovePoints(carrierParentNode->layer);
		float waitingCost = otherParentNode->turns - carrierParentNode->turns - 1
			+ carrierParentNode->moveRemains / (float)moveRemains;

		candidate.turns = otherParentNode->turns;
		candidate.cost += waitingCost;
		candidate.moveRemains = moveRemains;
	}

	return candidate;
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

		actors.push_back(std::make_shared<HeroActor>(hero.get(), mask, ai));
	}
}

void AINodeStorage::setTownsAndDwellings(
	const std::vector<const CGTownInstance *> & towns,
	const std::set<const CGObjectInstance *> & visitableObjs)
{
	for(auto town : towns)
	{
		uint64_t mask = 1 << actors.size();

		if(town->getUpperArmy()->getArmyStrength())
		{
			actors.push_back(std::make_shared<TownGarrisonActor>(town, mask));
		}
	}

	auto dayOfWeek = cb->getDate(Date::DAY_OF_WEEK);
	auto waitForGrowth = dayOfWeek > 4;

	for(auto obj: visitableObjs)
	{
		const CGDwelling * dwelling = dynamic_cast<const CGDwelling *>(obj);

		if(dwelling)
		{
			uint64_t mask = 1 << actors.size();
			auto dwellingActor = std::make_shared<DwellingActor>(dwelling, mask, false, dayOfWeek);

			if(dwellingActor->creatureSet->getArmyStrength())
			{
				actors.push_back(dwellingActor);
			}

			if(waitForGrowth)
			{
				mask = 1 << actors.size();
				dwellingActor = std::make_shared<DwellingActor>(dwelling, mask, waitForGrowth, dayOfWeek);

				if(dwellingActor->creatureSet->getArmyStrength())
				{
					actors.push_back(dwellingActor);
				}
			}
		}
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

	return hasBetterChain(source.node, getAINode(destination.node), chains);
}

template<class NodeRange>
bool AINodeStorage::hasBetterChain(
	const CGPathNode * source, 
	const AIPathNode * destinationNode,
	const NodeRange & chains) const
{
	auto dstActor = destinationNode->actor;

	for(const AIPathNode & node : chains)
	{
		auto sameNode = node.actor == destinationNode->actor;

		if(sameNode	|| node.action == CGPathNode::ENodeAction::UNKNOWN || !node.actor->hero)
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
					source->coord.toString(),
					destinationNode->coord.toString(),
					destinationNode->actor->chainMask,
					node.moveRemains - destinationNode->moveRemains);
#endif
				return true;
			}
		}

		if(dstActor->actorExchangeCount == 1)
			continue;

		auto nodeActor = node.actor;

		if(nodeActor->armyValue - node.armyLoss >= dstActor->armyValue - destinationNode->armyLoss
			&& nodeActor->heroFightingStrength >= dstActor->heroFightingStrength
			&& node.cost >= destinationNode->cost)
		{
			return true;
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

	paths.reserve(NUM_CHAINS / 4);

	auto chains = nodes[pos.x][pos.y][pos.z][isOnLand ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL];

	for(const AIPathNode & node : chains)
	{
		if(node.action == CGPathNode::ENodeAction::UNKNOWN || !node.actor || !node.actor->hero)
		{
			continue;
		}

		AIPath path;

		path.targetHero = node.actor->hero;
		path.heroArmy = node.actor->creatureSet;
		path.armyLoss = node.armyLoss;
		path.targetObjectDanger = evaluateDanger(pos, path.targetHero);
		path.chainMask = node.actor->chainMask;
		
		fillChainInfo(&node, path);

		paths.push_back(path);
	}

	return paths;
}

void AINodeStorage::fillChainInfo(const AIPathNode * node, AIPath & path) const
{
	while(node != nullptr)
	{
		if(!node->actor->hero)
			return;

		if(node->chainOther)
			fillChainInfo(node->chainOther, path);

		if(node->actor->hero->visitablePos() != node->coord)
		{
			AIPathNodeInfo pathNode;
			pathNode.cost = node->cost;
			pathNode.targetHero = node->actor->hero;
			pathNode.turns = node->turns;
			pathNode.danger = node->danger;
			pathNode.coord = node->coord;

			path.nodes.push_back(pathNode);
		}

		path.specialAction = node->specialAction;

		node = getAINode(node->theNodeBefore);
	}
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

const AIPathNodeInfo & AIPath::firstNode() const
{
	return nodes.back();
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

uint64_t AIPath::getHeroStrength() const
{
	return targetHero->getFightingStrength() * heroArmy->getArmyStrength();
}

uint64_t AIPath::getTotalDanger(HeroPtr hero) const
{
	uint64_t pathDanger = getPathDanger();
	uint64_t danger = pathDanger > targetObjectDanger ? pathDanger : targetObjectDanger;

	return danger;
}