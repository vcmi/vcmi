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
#include "Actions/WhirlpoolAction.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/callback/IGameInfoCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/pathfinder/CPathfinder.h"
#include "../../../lib/pathfinder/PathfinderUtil.h"
#include "../../../lib/pathfinder/PathfinderOptions.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/adventure/TownPortalEffect.h"
#include "../../../lib/IGameSettings.h"
#include "../../../lib/CPlayerState.h"

namespace NKAI
{

std::shared_ptr<boost::multi_array<AIPathNode, 4>> AISharedStorage::shared;
uint32_t AISharedStorage::version = 0;
std::mutex AISharedStorage::locker;
std::set<int3> committedTiles;
std::set<int3> committedTilesInitial;


const uint64_t FirstActorMask = 1;
const uint64_t MIN_ARMY_STRENGTH_FOR_CHAIN = 5000;
const uint64_t MIN_ARMY_STRENGTH_FOR_NEXT_ACTOR = 1000;
const uint64_t CHAIN_MAX_DEPTH = 4;

const bool DO_NOT_SAVE_TO_COMMITTED_TILES = false;

AISharedStorage::AISharedStorage(int3 sizes, int numChains)
{
	if(!shared){
		shared.reset(new boost::multi_array<AIPathNode, 4>(
			boost::extents[sizes.z][sizes.x][sizes.y][numChains]));

		nodes = shared;

		foreach_tile_pos([&](const int3 & pos)
			{
				for(auto i = 0; i < numChains; i++)
				{
					auto & node = get(pos)[i];
						
					node.version = -1;
					node.coord = pos;
				}
			});
	}
	else
		nodes = shared;
}

AISharedStorage::~AISharedStorage()
{
	nodes.reset();
	if(shared && shared.use_count() == 1)
	{
		shared.reset();
	}
}

void AIPathNode::addSpecialAction(std::shared_ptr<const SpecialAction> action)
{
	if(!specialAction)
	{
		specialAction = action;
	}
	else
	{
		auto parts = specialAction->getParts();

		if(parts.empty())
		{
			parts.push_back(specialAction);
		}

		parts.push_back(action);

		specialAction = std::make_shared<CompositeAction>(parts);
	}
}

int AINodeStorage::getBucketCount() const
{
	return ai->settings->getPathfinderBucketsCount();
}

int AINodeStorage::getBucketSize() const
{
	return ai->settings->getPathfinderBucketSize();
}

AINodeStorage::AINodeStorage(const Nullkiller * ai, const int3 & Sizes)
	: sizes(Sizes), ai(ai), cb(ai->cb.get()), nodes(Sizes, ai->settings->getPathfinderBucketSize() * ai->settings->getPathfinderBucketsCount())
{
	accessibility = std::make_unique<boost::multi_array<EPathAccessibility, 4>>(
		boost::extents[sizes.z][sizes.x][sizes.y][EPathfindingLayer::NUM_LAYERS]);
}

AINodeStorage::~AINodeStorage() = default;

void AINodeStorage::initialize(const PathfinderOptions & options, const IGameInfoCallback & gameInfo)
{
	if(heroChainPass != EHeroChainPass::INITIAL)
		return;

	AISharedStorage::version++;

	//TODO: fix this code duplication with NodeStorage::initialize, problem is to keep `resetTile` inline
	const PlayerColor fowPlayer = ai->playerID;
	const auto & fow = gameInfo.getPlayerTeam(fowPlayer)->fogOfWarMap;
	const int3 sizes = gameInfo.getMapSize();

	//Each thread gets different x, but an array of y located next to each other in memory

	tbb::parallel_for(tbb::blocked_range<size_t>(0, sizes.x), [&](const tbb::blocked_range<size_t>& r)
	{
		int3 pos;

		for(pos.z = 0; pos.z < sizes.z; ++pos.z)
		{
			const bool useFlying = options.useFlying;
			const bool useWaterWalking = options.useWaterWalking;
			const PlayerColor player = playerID;

			for(pos.x = r.begin(); pos.x != r.end(); ++pos.x)
			{
				for(pos.y = 0; pos.y < sizes.y; ++pos.y)
				{
					const TerrainTile * tile = gameInfo.getTile(pos);
					if (!tile->getTerrain()->isPassable())
						continue;

					if (tile->isWater())
					{
						resetTile(pos, ELayer::SAIL, PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(pos, *tile, fow, player, gameInfo));
						if (useFlying)
							resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, *tile, fow, player, gameInfo));
						if (useWaterWalking)
							resetTile(pos, ELayer::WATER, PathfinderUtil::evaluateAccessibility<ELayer::WATER>(pos, *tile, fow, player, gameInfo));
					}
					else
					{
						resetTile(pos, ELayer::LAND, PathfinderUtil::evaluateAccessibility<ELayer::LAND>(pos, *tile, fow, player, gameInfo));
						if (useFlying)
							resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, *tile, fow, player, gameInfo));
					}
				}
			}
		}
	});
}

void AINodeStorage::clear()
{
	actors.clear();
	committedTiles.clear();
	heroChainPass = EHeroChainPass::INITIAL;
	heroChainTurn = 0;
	heroChainMaxTurns = 1;
	turnDistanceLimit[HeroRole::MAIN] = 255;
	turnDistanceLimit[HeroRole::SCOUT] = 255;
}

std::optional<AIPathNode *> AINodeStorage::getOrCreateNode(
	const int3 & pos, 
	const EPathfindingLayer layer, 
	const ChainActor * actor)
{
	int bucketIndex = ((uintptr_t)actor + layer.getNum()) % ai->settings->getPathfinderBucketsCount();
	int bucketOffset = bucketIndex * ai->settings->getPathfinderBucketSize();
	auto chains = nodes.get(pos);

	if(blocked(pos, layer))
	{
		return std::nullopt;
	}

	for(auto i = ai->settings->getPathfinderBucketSize() - 1; i >= 0; i--)
	{
		AIPathNode & node = chains[i + bucketOffset];

		if(node.version != AISharedStorage::version)
		{
			node.reset(layer, getAccessibility(pos, layer));
			node.version = AISharedStorage::version;
			node.actor = actor;

			return &node;
		}

		if(node.actor == actor && node.layer == layer)
		{
			return &node;
		}
	}

	return std::nullopt;
}

std::vector<CGPathNode *> AINodeStorage::getInitialNodes()
{
	if(heroChainPass)
	{
		if(heroChainTurn == 0)
			calculateTownPortalTeleportations(heroChain);

		return heroChain;
	}

	std::vector<CGPathNode *> initialNodes;

	for(auto actorPtr : actors)
	{
		ChainActor * actor = actorPtr.get();

		auto allocated = getOrCreateNode(actor->initialPosition, actor->layer, actor);

		if(!allocated)
			continue;

		AIPathNode * initialNode = allocated.value();

		initialNode->pq = nullptr;
		initialNode->turns = actor->initialTurn;
		initialNode->moveRemains = actor->initialMovement;
		initialNode->danger = 0;
		initialNode->setCost(actor->initialTurn);
		initialNode->action = EPathNodeAction::NORMAL;

		if(actor->isMovable)
		{
			initialNodes.push_back(initialNode);
		}
		else
		{
			initialNode->locked = true;
		}
	}

	if(heroChainTurn == 0)
		calculateTownPortalTeleportations(initialNodes);

	return initialNodes;
}

void AINodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	const AIPathNode * srcNode = getAINode(source.node);

	updateAINode(destination.node, [&](AIPathNode * dstNode)
	{
		commit(dstNode, srcNode, destination.action, destination.turn, destination.movementLeft, destination.cost);

		// regular pathfinder can not go directly through whirlpool
		bool isWhirlpoolTeleport = destination.nodeObject
			&& destination.nodeObject->ID == Obj::WHIRLPOOL;

		if(srcNode->specialAction
			|| srcNode->chainOther
			|| isWhirlpoolTeleport)
		{
			// there is some action on source tile which should be performed before we can bypass it
			dstNode->theNodeBefore = source.node;

			if(isWhirlpoolTeleport)
			{
				if(dstNode->actor->creatureSet->Slots().size() == 1
					&& dstNode->actor->creatureSet->Slots().begin()->second->getCount() == 1)
				{
					return;
				}

				const auto & weakest = vstd::minElementByFun(dstNode->actor->creatureSet->Slots(), [](const auto & pair) -> int
					{
						return pair.second->getCount() * pair.second->getCreatureID().toCreature()->getAIValue();
					});

				if(weakest == dstNode->actor->creatureSet->Slots().end())
				{
					logAi->debug("Empty army entering whirlpool detected at tile %s", dstNode->coord.toString());
					destination.blocked = true;

					return;
				}

				if(dstNode->actor->creatureSet->getFreeSlots().size())
					dstNode->armyLoss += weakest->second->getCreatureID().toCreature()->getAIValue();
				else
					dstNode->armyLoss += (weakest->second->getCount() + 1) / 2 * weakest->second->getCreatureID().toCreature()->getAIValue();

				dstNode->specialAction = AIPathfinding::WhirlpoolAction::instance;
			}
		}

		if(dstNode->specialAction && dstNode->actor)
		{
			dstNode->specialAction->applyOnDestination(dstNode->actor->hero, destination, source, dstNode, srcNode);
		}
	});
}

void AINodeStorage::commit(
	AIPathNode * destination, 
	const AIPathNode * source, 
	EPathNodeAction action, 
	int turn, 
	int movementLeft, 
	float cost,
	bool saveToCommitted) const
{
	destination->action = action;
	destination->setCost(cost);
	destination->moveRemains = movementLeft;
	destination->turns = turn;
	destination->armyLoss = source->armyLoss;
	destination->manaCost = source->manaCost;
	destination->danger = source->danger;
	destination->theNodeBefore = source->theNodeBefore;
	destination->chainOther = nullptr;

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
	logAi->trace(
		"Committed %s -> %s, layer: %d, cost: %f, turn: %s, mp: %d, hero: %s, mask: %x, army: %lld",
		source->coord.toString(),
		destination->coord.toString(),
		destination->layer,
		destination->getCost(),
		std::to_string(destination->turns),
		destination->moveRemains,
		destination->actor->toString(),
		destination->actor->chainMask,
		destination->actor->armyValue);
#endif

	if(saveToCommitted && destination->turns <= heroChainTurn)
	{
		committedTiles.insert(destination->coord);
	}

	if(destination->turns == source->turns)
	{
		destination->dayFlags = source->dayFlags;
	}
}

void AINodeStorage::calculateNeighbours(
	std::vector<CGPathNode *> & result,
	const PathNodeInfo & source,
	EPathfindingLayer layer,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	NeighbourTilesVector accessibleNeighbourTiles;

	result.clear();
	pathfinderHelper->calculateNeighbourTiles(accessibleNeighbourTiles, source);

	const AIPathNode * srcNode = getAINode(source.node);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		if(getAccessibility(neighbour, layer) == EPathAccessibility::NOT_SET)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Node %s rejected for %s, layer %d because of inaccessibility",
				neighbour.toString(),
				source.coord.toString(),
				static_cast<int32_t>(layer));
#endif
			continue;
		}

		auto nextNode = getOrCreateNode(neighbour, layer, srcNode->actor);

		if(!nextNode)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Failed to allocate node at %s[%d]",
				neighbour.toString(),
				static_cast<int32_t>(layer));
#endif
			continue;
		}

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Node %s added to neighbors of %s, layer %d",
			neighbour.toString(),
			source.coord.toString(),
			static_cast<int32_t>(layer));
#endif

		result.push_back(nextNode.value());
	}
}

constexpr std::array phisycalLayers = {EPathfindingLayer::LAND, EPathfindingLayer::SAIL};

bool AINodeStorage::increaseHeroChainTurnLimit()
{
	if(heroChainTurn >= heroChainMaxTurns)
		return false;

	heroChainTurn++;
	committedTiles.clear();

	for(auto layer : phisycalLayers)
	{
		foreach_tile_pos([&](const int3 & pos)
		{
			iterateValidNodesUntil(pos, layer, [&](AIPathNode & node)
				{
					if(node.turns <= heroChainTurn && node.action != EPathNodeAction::UNKNOWN)
					{
						committedTiles.insert(pos);
						return true;
					}

					return false;
				});
		});
	}

	return true;
}

bool AINodeStorage::calculateHeroChainFinal()
{
	heroChainPass = EHeroChainPass::FINAL;
	heroChain.resize(0);

	for(auto layer : phisycalLayers)
	{
		foreach_tile_pos([&](const int3 & pos)
		{
			iterateValidNodes(pos, layer, [&](AIPathNode & node)
				{
					if(node.turns > heroChainTurn
						&& !node.locked
						&& node.action != EPathNodeAction::UNKNOWN
						&& node.actor->actorExchangeCount > 1
						&& !hasBetterChain(&node, node))
					{
						heroChain.push_back(&node);
					}
				});
		});
	}

	return heroChain.size();
}

struct DelayedWork
{
	AIPathNode * carrier;
	AIPathNode * other;

	DelayedWork()
	{
	}
	
	DelayedWork(AIPathNode * carrier, AIPathNode * other) : carrier(carrier), other(other)
	{
	}
};

class HeroChainCalculationTask
{
private:
	AINodeStorage & storage;
	std::vector<AIPathNode *> existingChains;
	std::vector<ExchangeCandidate> newChains;
	uint64_t chainMask;
	int heroChainTurn;
	std::vector<CGPathNode *> heroChain;
	const std::vector<int3> & tiles;
	std::vector<DelayedWork> delayedWork;

public:
	HeroChainCalculationTask(
		AINodeStorage & storage, const std::vector<int3> & tiles, uint64_t chainMask, int heroChainTurn)
		:existingChains(), newChains(), delayedWork(), storage(storage), chainMask(chainMask), heroChainTurn(heroChainTurn), heroChain(), tiles(tiles)
	{
		existingChains.reserve(storage.getBucketCount() * storage.getBucketSize());
		newChains.reserve(storage.getBucketCount() * storage.getBucketSize());
	}

	void execute(const tbb::blocked_range<size_t>& r)
	{
		std::random_device randomDevice;
		std::mt19937 randomEngine(randomDevice());

		for(int i = r.begin(); i != r.end(); i++)
		{
			auto & pos = tiles[i];

			for(auto layer : phisycalLayers)
			{
				existingChains.clear();

				storage.iterateValidNodes(pos, layer, [this](AIPathNode & node)
					{
						if(node.turns <= heroChainTurn && node.action != EPathNodeAction::UNKNOWN)
							existingChains.push_back(&node);
					});

				if(existingChains.empty())
					continue;

				newChains.clear();

				std::shuffle(existingChains.begin(), existingChains.end(), randomEngine);

				for(AIPathNode * node : existingChains)
				{
					if(node->actor->isMovable)
					{
						calculateHeroChain(node, existingChains, newChains);
					}
				}

				for(auto delayed = delayedWork.begin(); delayed != delayedWork.end();)
				{
					auto newActor = delayed->carrier->actor->tryExchangeNoLock(delayed->other->actor);

					if(!newActor.lockAcquired) continue;
					
					if(newActor.actor)
					{
						newChains.push_back(calculateExchange(newActor.actor, delayed->carrier, delayed->other));
					}
					
					delayed++;
				}

				delayedWork.clear();

				cleanupInefectiveChains(newChains);
				addHeroChain(newChains);
			}
		}
	}

	void calculateHeroChain(
		AIPathNode * srcNode,
		const std::vector<AIPathNode *> & variants,
		std::vector<ExchangeCandidate> & result);

	void calculateHeroChain(
		AIPathNode * carrier,
		AIPathNode * other,
		std::vector<ExchangeCandidate> & result);

	void cleanupInefectiveChains(std::vector<ExchangeCandidate> & result) const;
	void addHeroChain(const std::vector<ExchangeCandidate> & result);

	ExchangeCandidate calculateExchange(
		ChainActor * exchangeActor,
		AIPathNode * carrierParentNode,
		AIPathNode * otherParentNode) const;

	void flushResult(std::vector<CGPathNode *> & result)
	{
		vstd::concatenate(result, heroChain);
	}
};

bool AINodeStorage::calculateHeroChain()
{
	heroChainPass = EHeroChainPass::CHAIN;
	heroChain.clear();

	std::vector<int3> data(committedTiles.begin(), committedTiles.end());

	int maxConcurrency = tbb::this_task_arena::max_concurrency();
	std::vector<std::vector<CGPathNode *>> results(maxConcurrency);

	logAi->trace("Caculating hero chain for %d items", data.size());

	tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()), [&](const tbb::blocked_range<size_t>& r)
	{
		HeroChainCalculationTask task(*this, data, chainMask, heroChainTurn);

		int ourThread = tbb::this_task_arena::current_thread_index();
		task.execute(r);
		task.flushResult(results.at(ourThread));
	});

	// FIXME: potentially non-deterministic behavior due to parallel_for
	for (const auto & result : results)
		vstd::concatenate(heroChain, result);

	committedTiles.clear();

	return !heroChain.empty();
}

bool AINodeStorage::selectFirstActor()
{
	if(actors.empty())
		return false;

	auto strongest = *vstd::maxElementByFun(actors, [](std::shared_ptr<ChainActor> actor) -> uint64_t
	{
		return actor->armyValue;
	});

	chainMask = strongest->chainMask;
	committedTilesInitial = committedTiles;

	return true;
}

bool AINodeStorage::selectNextActor()
{
	auto currentActor = std::find_if(actors.begin(), actors.end(), [&](std::shared_ptr<ChainActor> actor)-> bool
	{
		return actor->chainMask == chainMask;
	});

	auto nextActor = actors.end();

	for(auto actor = actors.begin(); actor != actors.end(); actor++)
	{
		if(actor->get()->armyValue > currentActor->get()->armyValue
			|| (actor->get()->armyValue == currentActor->get()->armyValue && actor <= currentActor))
		{
			continue;
		}

		if(nextActor == actors.end()
			|| actor->get()->armyValue > nextActor->get()->armyValue)
		{
			nextActor = actor;
		}
	}

	if(nextActor != actors.end())
	{
		if(nextActor->get()->armyValue < MIN_ARMY_STRENGTH_FOR_NEXT_ACTOR)
			return false;

		chainMask = nextActor->get()->chainMask;
		committedTiles = committedTilesInitial;

		return true;
	}

	return false;
}

uint64_t AINodeStorage::evaluateArmyLoss(const CGHeroInstance * hero, uint64_t armyValue, uint64_t danger) const
{
	float fightingStrength = ai->heroManager->getFightingStrengthCached(hero);
	double ratio = (double)danger / (armyValue * fightingStrength);

	return (uint64_t)(armyValue * ratio * ratio);
}

void HeroChainCalculationTask::cleanupInefectiveChains(std::vector<ExchangeCandidate> & result) const
{
	vstd::erase_if(result, [&](const ExchangeCandidate & chainInfo) -> bool
	{
		auto isNotEffective = storage.hasBetterChain(chainInfo.carrierParent, chainInfo)
			|| storage.hasBetterChain(chainInfo.carrierParent, chainInfo, result);

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		if(isNotEffective)
		{
			logAi->trace(
				"Skip exchange %s[%x] -> %s[%x] at %s is inefficient",
				chainInfo.otherParent->actor->toString(), 
				chainInfo.otherParent->actor->chainMask,
				chainInfo.carrierParent->actor->toString(),
				chainInfo.carrierParent->actor->chainMask,
				chainInfo.carrierParent->coord.toString());
		}
#endif

		return isNotEffective;
	});
}

void HeroChainCalculationTask::calculateHeroChain(
	AIPathNode * srcNode, 
	const std::vector<AIPathNode *> & variants, 
	std::vector<ExchangeCandidate> & result)
{
	for(AIPathNode * node : variants)
	{
		if(node == srcNode || !node->actor || node->version != AISharedStorage::version)
			continue;

		if((node->actor->chainMask & chainMask) == 0 && (srcNode->actor->chainMask & chainMask) == 0)
			continue;

		if(node->actor->actorExchangeCount + srcNode->actor->actorExchangeCount > CHAIN_MAX_DEPTH)
			continue;

		if(node->action == EPathNodeAction::BATTLE
			|| node->action == EPathNodeAction::TELEPORT_BATTLE
			|| node->action == EPathNodeAction::TELEPORT_NORMAL
			|| node->action == EPathNodeAction::DISEMBARK
			|| node->action == EPathNodeAction::TELEPORT_BLOCKING_VISIT)
		{
			continue;
		}

		if(node->turns > heroChainTurn 
			|| (node->action == EPathNodeAction::UNKNOWN && node->actor->hero)
			|| (node->actor->chainMask & srcNode->actor->chainMask) != 0)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Skip exchange %s[%x] -> %s[%x] at %s because of %s",
				node->actor->toString(),
				node->actor->chainMask,
				srcNode->actor->toString(),
				srcNode->actor->chainMask,
				srcNode->coord.toString(),
				(node->turns > heroChainTurn 
					? "turn limit" 
					: (node->action == EPathNodeAction::UNKNOWN && node->actor->hero)
						? "action unknown"
						: "chain mask"));
#endif
			continue;
		}

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Thy exchange %s[%x] -> %s[%x] at %s",
			node->actor->toString(),
			node->actor->chainMask,
			srcNode->actor->toString(),
			srcNode->actor->chainMask,
			srcNode->coord.toString());
#endif

		calculateHeroChain(srcNode, node, result);
	}
}

void HeroChainCalculationTask::calculateHeroChain(
	AIPathNode * carrier, 
	AIPathNode * other, 
	std::vector<ExchangeCandidate> & result)
{	
	if(carrier->armyLoss < carrier->actor->armyValue
		&& (carrier->action != EPathNodeAction::BATTLE || (carrier->actor->allowBattle && carrier->specialAction))
		&& carrier->action != EPathNodeAction::BLOCKING_VISIT
		&& (other->armyLoss == 0 || other->armyLoss < other->actor->armyValue))
	{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Exchange allowed %s[%x] -> %s[%x] at %s",
			other->actor->toString(),
			other->actor->chainMask,
			carrier->actor->toString(),
			carrier->actor->chainMask,
			carrier->coord.toString());
#endif

		if(other->actor->isMovable)
		{
			bool hasLessMp = carrier->turns > other->turns || (carrier->turns == other->turns && carrier->moveRemains < other->moveRemains);
			bool hasLessExperience = carrier->actor->hero->exp < other->actor->hero->exp;

			if(hasLessMp && hasLessExperience)
			{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace("Exchange at %s is inefficient. Blocked.", carrier->coord.toString());
#endif
				return;
			}
		}

		auto newActor = carrier->actor->tryExchangeNoLock(other->actor);
		
		if(!newActor.lockAcquired) delayedWork.push_back(DelayedWork(carrier, other));
		if(newActor.actor) result.push_back(calculateExchange(newActor.actor, carrier, other));
	}
}

void HeroChainCalculationTask::addHeroChain(const std::vector<ExchangeCandidate> & result)
{
	for(const ExchangeCandidate & chainInfo : result)
	{
		auto carrier = chainInfo.carrierParent;
		auto newActor = chainInfo.actor;
		auto other = chainInfo.otherParent;
		auto chainNodeOptional = storage.getOrCreateNode(carrier->coord, carrier->layer, newActor);

		if(!chainNodeOptional)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace("Exchange at %s can not allocate node. Blocked.", carrier->coord.toString());
#endif
			continue;
		}

		auto exchangeNode = chainNodeOptional.value();

		if(exchangeNode->action != EPathNodeAction::UNKNOWN)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Skip exchange %s[%x] -> %s[%x] at %s because node is in use",
				other->actor->toString(),
				other->actor->chainMask,
				carrier->actor->toString(),
				carrier->actor->chainMask,
				carrier->coord.toString());
#endif
			continue;
		}
		
		if(exchangeNode->turns != 0xFF && exchangeNode->getCost() < chainInfo.getCost())
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Skip exchange %s[%x] -> %s[%x] at %s because not effective enough. %f < %f",
				other->actor->toString(),
				other->actor->chainMask,
				carrier->actor->toString(),
				carrier->actor->chainMask,
				carrier->coord.toString(),
				exchangeNode->getCost(),
				chainInfo.getCost());
#endif
			continue;
		}

		storage.commit(
			exchangeNode,
			carrier,
			carrier->action,
			chainInfo.turns,
			chainInfo.moveRemains, 
			chainInfo.getCost(),
			DO_NOT_SAVE_TO_COMMITTED_TILES);

		if(carrier->specialAction || carrier->chainOther)
		{
			// there is some action on source tile which should be performed before we can bypass it
			exchangeNode->theNodeBefore = carrier;
		}

		if(exchangeNode->actor->actorAction)
		{
			exchangeNode->theNodeBefore = carrier;
			exchangeNode->addSpecialAction(exchangeNode->actor->actorAction);
		}

		exchangeNode->chainOther = other;
		exchangeNode->armyLoss = chainInfo.armyLoss;

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Chain accepted at %s %s -> %s, mask %x, cost %f, turn: %s, mp: %d, army %i", 
			exchangeNode->coord.toString(), 
			other->actor->toString(), 
			exchangeNode->actor->toString(),
			exchangeNode->actor->chainMask,
			exchangeNode->getCost(),
			std::to_string(exchangeNode->turns),
			exchangeNode->moveRemains,
			exchangeNode->actor->armyValue);
#endif
		heroChain.push_back(exchangeNode);
	}
}

ExchangeCandidate HeroChainCalculationTask::calculateExchange(
	ChainActor * exchangeActor, 
	AIPathNode * carrierParentNode, 
	AIPathNode * otherParentNode) const
{
	ExchangeCandidate candidate;
	
	candidate.layer = carrierParentNode->layer;
	candidate.coord = carrierParentNode->coord;
	candidate.carrierParent = carrierParentNode;
	candidate.otherParent = otherParentNode;
	candidate.actor = exchangeActor;
	candidate.armyLoss = carrierParentNode->armyLoss + otherParentNode->armyLoss;
	candidate.turns = carrierParentNode->turns;
	candidate.setCost(carrierParentNode->getCost() + otherParentNode->getCost() / 1000.0);
	candidate.moveRemains = carrierParentNode->moveRemains;
	candidate.danger = carrierParentNode->danger;
	candidate.version = carrierParentNode->version;

	if(carrierParentNode->turns < otherParentNode->turns)
	{
		int moveRemains = exchangeActor->maxMovePoints(carrierParentNode->layer);
		float waitingCost = otherParentNode->turns - carrierParentNode->turns - 1
			+ carrierParentNode->moveRemains / (float)moveRemains;

		candidate.turns = otherParentNode->turns;
		candidate.setCost(candidate.getCost() + waitingCost);
		candidate.moveRemains = moveRemains;
	}

	return candidate;
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

bool AINodeStorage::isDistanceLimitReached(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
{
	if(heroChainPass == EHeroChainPass::CHAIN && destination.node->turns > heroChainTurn)
	{
		return true;
	}
	
	auto aiNode = getAINode(destination.node);
	
	if(heroChainPass != EHeroChainPass::CHAIN
		&& destination.node->turns > turnDistanceLimit[aiNode->actor->heroRole])
	{
		return true;
	}

	return false;
}

void AINodeStorage::setHeroes(std::map<const CGHeroInstance *, HeroRole> heroes)
{
	playerID = ai->playerID;

	for(auto & hero : heroes)
	{
		// do not allow our own heroes in garrison to act on map
		if(hero.first->getOwner() == ai->playerID
			&& hero.first->isGarrisoned()
			&& (ai->isHeroLocked(hero.first) || ai->heroManager->heroCapReached(false)))
		{
			continue;
		}

		uint64_t mask = FirstActorMask << actors.size();
		auto actor = std::make_shared<HeroActor>(hero.first, hero.second, mask, ai);

		if(actor->hero->tempOwner != ai->playerID)
		{
			bool onLand = !actor->hero->inBoat() || actor->hero->getBoat()->layer != EPathfindingLayer::SAIL;
			actor->initialMovement = actor->hero->movementPointsLimit(onLand);
		}

		playerID = actor->hero->tempOwner;

		actors.push_back(actor);
	}
}

void AINodeStorage::setTownsAndDwellings(
	const std::vector<const CGTownInstance *> & towns,
	const std::set<const CGObjectInstance *> & visitableObjs)
{
	for(auto town : towns)
	{
		uint64_t mask = FirstActorMask << actors.size();

		// TODO: investigate logix of second condition || ai->nullkiller->getHeroLockedReason(town->getGarrisonHero()) != HeroLockedReason::DEFENCE
		// check defence imrove
		if(!town->getGarrisonHero())
		{
			actors.push_back(std::make_shared<TownGarrisonActor>(town, mask));
		}
	}

	/*auto dayOfWeek = cb->getDate(Date::DAY_OF_WEEK);
	auto waitForGrowth = dayOfWeek > 4;*/

	for(auto obj: visitableObjs)
	{
		if(obj->ID == Obj::HILL_FORT)
		{
			uint64_t mask = FirstActorMask << actors.size();

			actors.push_back(std::make_shared<HillFortActor>(obj, mask));
		}
		/*const CGDwelling * dwelling = dynamic_cast<const CGDwelling *>(obj);

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
		}*/
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
			std::optional<AIPathNode *> node = getOrCreateNode(neighbour, source.node->layer, srcNode->actor);
			
			if(!node)
				continue;

			neighbours.push_back(node.value());
		}
	}

	return neighbours;
}

struct TownPortalFinder
{
	const std::vector<CGPathNode *> & initialNodes;
	const ChainActor * actor;
	const CGHeroInstance * hero;
	std::vector<const CGTownInstance *> targetTowns;
	AINodeStorage * nodeStorage;
	const CSpell * townPortal;
	uint64_t movementNeeded;
	SpellID spellID;
	bool townSelectionAllowed;

	TownPortalFinder(const ChainActor * actor, const std::vector<CGPathNode *> & initialNodes, const std::vector<const CGTownInstance *> & targetTowns, AINodeStorage * nodeStorage, SpellID spellID)
		: initialNodes(initialNodes)
		, actor(actor)
		, hero(actor->hero)
		, targetTowns(targetTowns)
		, nodeStorage(nodeStorage)
		, townPortal(spellID.toSpell())
		, spellID(spellID)
	{
		auto townPortalEffect = townPortal->getAdventureMechanics().getEffectAs<TownPortalEffect>(hero);
		movementNeeded = townPortalEffect->getMovementPointsRequired();
		townSelectionAllowed = townPortalEffect->townSelectionAllowed();
	}

	bool actorCanCastTownPortal()
	{
		return hero->canCastThisSpell(townPortal) && hero->mana >= hero->getSpellCost(townPortal);
	}

	CGPathNode * getBestInitialNodeForTownPortal(const CGTownInstance * targetTown)
	{
		for(CGPathNode * node : initialNodes)
		{
			auto aiNode = nodeStorage->getAINode(node);

			if(aiNode->actor->baseActor != actor
				|| node->layer != EPathfindingLayer::LAND
				|| node->moveRemains < movementNeeded)
			{
				continue;
			}

			if(!townSelectionAllowed)
			{
				const CGTownInstance * nearestTown = *vstd::minElementByFun(targetTowns, [&](const CGTownInstance * t) -> int
				{
					return node->coord.dist2dSQ(t->visitablePos());
				});

				if(targetTown != nearestTown)
					continue;
			}

			return node;
		}

		return nullptr;
	}

	std::optional<AIPathNode *> createTownPortalNode(const CGTownInstance * targetTown)
	{
		auto bestNode = getBestInitialNodeForTownPortal(targetTown);

		if(!bestNode)
			return std::nullopt;

		auto nodeOptional = nodeStorage->getOrCreateNode(targetTown->visitablePos(), EPathfindingLayer::LAND, actor->castActor);

		if(!nodeOptional)
			return std::nullopt;

		AIPathNode * node = nodeOptional.value();
		float movementCost = (float)movementNeeded / (float)hero->movementPointsLimit(EPathfindingLayer::LAND);

		movementCost += bestNode->getCost();

		if(node->action == EPathNodeAction::UNKNOWN || node->getCost() > movementCost)
		{
			nodeStorage->commit(
				node,
				nodeStorage->getAINode(bestNode),
				EPathNodeAction::TELEPORT_NORMAL,
				bestNode->turns,
				bestNode->moveRemains - movementNeeded,
				movementCost,
				DO_NOT_SAVE_TO_COMMITTED_TILES);

			node->theNodeBefore = bestNode;
			node->addSpecialAction(std::make_shared<AIPathfinding::TownPortalAction>(targetTown, spellID));
		}

		return nodeOptional;
	}
};

template<class TVector>
void AINodeStorage::calculateTownPortal(
	const ChainActor * actor,
	const std::map<const CGHeroInstance *, int> & maskMap,
	const std::vector<CGPathNode *> & initialNodes,
	TVector & output)
{
	auto towns = cb->getTownsInfo(false);

	vstd::erase_if(towns, [&](const CGTownInstance * t) -> bool
		{
			return cb->getPlayerRelations(actor->hero->tempOwner, t->tempOwner) == PlayerRelations::ENEMIES;
		});

	if(!towns.size())
	{
		return; // no towns no need to run loop further
	}

	for (const auto & spell : LIBRARY->spellh->objects)
	{
		if (!spell || !spell->isAdventure())
			continue;

		auto townPortalEffect = spell->getAdventureMechanics().getEffectAs<TownPortalEffect>(actor->hero);

		if (!townPortalEffect)
			continue;

		TownPortalFinder townPortalFinder(actor, initialNodes, towns, this, spell->id);

		if(!townPortalFinder.actorCanCastTownPortal())
			continue;

		for(const CGTownInstance * targetTown : towns)
		{
			if(targetTown->getVisitingHero()
				&& targetTown->getUpperArmy()->stacksCount()
				&& maskMap.find(targetTown->getVisitingHero()) != maskMap.end())
			{
				auto basicMask = maskMap.at(targetTown->getVisitingHero());
				bool sameActorInTown = actor->chainMask == basicMask;

				if(!sameActorInTown)
					continue;
			}

			if (targetTown->getVisitingHero()
				&& (targetTown->getVisitingHero()->getFactionID() != actor->hero->getFactionID()
					|| targetTown->getUpperArmy()->stacksCount()))
				continue;

			auto nodeOptional = townPortalFinder.createTownPortalNode(targetTown);

			if(nodeOptional)
			{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 1
				logAi->trace("Adding town portal node at %s", targetTown->getObjectName());
#endif
				output.push_back(nodeOptional.value());
			}
		}
	}
}

void AINodeStorage::calculateTownPortalTeleportations(std::vector<CGPathNode *> & initialNodes)
{
	std::set<const ChainActor *> actorsOfInitial;

	for(const CGPathNode * node : initialNodes)
	{
		auto aiNode = getAINode(node);

		if(aiNode->actor->hero)
			actorsOfInitial.insert(aiNode->actor->baseActor);
	}

	std::map<const CGHeroInstance *, int> maskMap;

	for(std::shared_ptr<ChainActor> basicActor : actors)
	{
		if(basicActor->hero)
			maskMap[basicActor->hero] = basicActor->chainMask;
	}

	boost::sort(initialNodes, NodeComparer<CGPathNode>());

	std::vector<const ChainActor *> actorsVector(actorsOfInitial.begin(), actorsOfInitial.end());
	tbb::concurrent_vector<CGPathNode *> output;

// TODO: re-enable after fixing thread races. See issue for details:
// https://github.com/vcmi/vcmi/pull/4130
#if 0
	if (actorsVector.size() * initialNodes.size() > 1000)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(0, actorsVector.size()), [&](const tbb::blocked_range<size_t> & r)
			{
				for(int i = r.begin(); i != r.end(); i++)
				{
					calculateTownPortal(actorsVector[i], maskMap, initialNodes, output);
				}
			});

		std::copy(output.begin(), output.end(), std::back_inserter(initialNodes));
	}
	else
#endif
	{
		for(auto actor : actorsVector)
		{
			calculateTownPortal(actor, maskMap, initialNodes, initialNodes);
		}
	}
}

bool AINodeStorage::hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
{
	auto candidateNode = getAINode(destination.node);

	return hasBetterChain(source.node, *candidateNode);
}

bool AINodeStorage::hasBetterChain(
	const CGPathNode * source,
	const AIPathNode & candidateNode) const
{
	return iterateValidNodesUntil(
		candidateNode.coord,
		candidateNode.layer,
		[this, &source, candidateNode](const AIPathNode & node) -> bool
		{
			return isOtherChainBetter(source, candidateNode, node);
		});
}

template<class NodeRange>
bool AINodeStorage::hasBetterChain(
	const CGPathNode * source,
	const AIPathNode & candidateNode,
	const NodeRange & nodes) const
{
	for(const AIPathNode & node : nodes)
	{
		if(isOtherChainBetter(source, candidateNode, node))
			return true;
	}

	return false;
}

bool AINodeStorage::isOtherChainBetter(
	const CGPathNode * source,
	const AIPathNode & candidateNode,
	const AIPathNode & other) const
{
	auto sameNode = other.actor == candidateNode.actor;

	if(sameNode || other.action == EPathNodeAction::UNKNOWN || !other.actor || !other.actor->hero)
	{
		return false;
	}

	if(other.danger <= candidateNode.danger && candidateNode.actor == other.actor->battleActor)
	{
		if(other.getCost() < candidateNode.getCost())
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Block inefficient battle move %s->%s, hero: %s[%X], army %lld, mp diff: %i",
				source->coord.toString(),
				candidateNode.coord.toString(),
				candidateNode.actor->hero->getNameTranslated(),
				candidateNode.actor->chainMask,
				candidateNode.actor->armyValue,
				other.moveRemains - candidateNode.moveRemains);
#endif
			return true;
		}
	}

	if(candidateNode.actor->chainMask != other.actor->chainMask && heroChainPass != EHeroChainPass::FINAL)
		return false;

	auto nodeActor = other.actor;
	auto nodeArmyValue = nodeActor->armyValue - other.armyLoss;
	auto candidateArmyValue = candidateNode.actor->armyValue - candidateNode.armyLoss;

	if(nodeArmyValue > candidateArmyValue
		&& other.getCost() <= candidateNode.getCost())
	{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Block inefficient move because of stronger army %s->%s, hero: %s[%X], army %lld, mp diff: %i",
			source->coord.toString(),
			candidateNode.coord.toString(),
			candidateNode.actor->hero->getNameTranslated(),
			candidateNode.actor->chainMask,
			candidateNode.actor->armyValue,
			other.moveRemains - candidateNode.moveRemains);
#endif
		return true;
	}

	if(heroChainPass == EHeroChainPass::FINAL)
	{
		if(nodeArmyValue == candidateArmyValue
			&& nodeActor->heroFightingStrength >= candidateNode.actor->heroFightingStrength
			&& other.getCost() <= candidateNode.getCost())
		{
			if(vstd::isAlmostEqual(nodeActor->heroFightingStrength, candidateNode.actor->heroFightingStrength)
				&& vstd::isAlmostEqual(other.getCost(), candidateNode.getCost())
				&& &other < &candidateNode)
			{
				return false;
			}

#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Block inefficient move because of stronger hero %s->%s, hero: %s[%X], army %lld, mp diff: %i",
				source->coord.toString(),
				candidateNode.coord.toString(),
				candidateNode.actor->hero->getNameTranslated(),
				candidateNode.actor->chainMask,
				candidateNode.actor->armyValue,
				other.moveRemains - candidateNode.moveRemains);
#endif
			return true;
		}
	}

	return false;
}

bool AINodeStorage::isTileAccessible(const HeroPtr & hero, const int3 & pos, const EPathfindingLayer layer) const
{
	auto chains = nodes.get(pos);

	for(const AIPathNode & node : chains)
	{
		if(node.version == AISharedStorage::version
			&& node.layer == layer
			&& node.action != EPathNodeAction::UNKNOWN 
			&& node.actor
			&& node.actor->hero == hero.h)
		{
			return true;
		}
	}

	return false;
}

void AINodeStorage::calculateChainInfo(std::vector<AIPath> & paths, const int3 & pos, bool isOnLand) const
{
	auto layer = isOnLand ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL;
	auto chains = nodes.get(pos);

	for(const AIPathNode & node : chains)
	{
		if(node.version != AISharedStorage::version
			|| node.layer != layer
			|| node.action == EPathNodeAction::UNKNOWN
			|| !node.actor
			|| !node.actor->hero)
		{
			continue;
		}

		AIPath & path = paths.emplace_back();

		path.targetHero = node.actor->hero;
		path.heroArmy = node.actor->creatureSet;
		path.armyLoss = node.armyLoss;
		path.targetObjectDanger = ai->dangerEvaluator->evaluateDanger(pos, path.targetHero, !node.actor->allowBattle);
		for (auto pathNode : path.nodes)
		{
			path.targetObjectDanger = std::max(ai->dangerEvaluator->evaluateDanger(pathNode.coord, path.targetHero, !node.actor->allowBattle), path.targetObjectDanger);
		}

		if(path.targetObjectDanger > 0)
		{
			if(node.theNodeBefore)
			{
				auto prevNode = getAINode(node.theNodeBefore);

				if(node.coord == prevNode->coord && node.actor->hero == prevNode->actor->hero)
				{
					paths.pop_back();
					continue;
				}
				else
				{
					path.armyLoss = prevNode->armyLoss;
				}
			}
			else
			{
				path.armyLoss = 0;
			}
		}

		int fortLevel = 0;
		auto visitableObjects = cb->getVisitableObjs(pos);
		for (auto obj : visitableObjects)
		{
			if (objWithID<Obj::TOWN>(obj))
			{
				auto town = dynamic_cast<const CGTownInstance*>(obj);
				fortLevel = town->fortLevel();
			}
		}

		path.targetObjectArmyLoss = evaluateArmyLoss(
			path.targetHero,
			getHeroArmyStrengthWithCommander(path.targetHero, path.heroArmy, fortLevel),
			path.targetObjectDanger);

		path.chainMask = node.actor->chainMask;
		path.exchangeCount = node.actor->actorExchangeCount;
		
		fillChainInfo(&node, path, -1);
	}
}

void AINodeStorage::fillChainInfo(const AIPathNode * node, AIPath & path, int parentIndex) const
{
	while(node != nullptr)
	{
		if(!node->actor->hero)
			return;

		if(node->chainOther)
			fillChainInfo(node->chainOther, path, parentIndex);

		AIPathNodeInfo pathNode;

		pathNode.cost = node->getCost();
		pathNode.targetHero = node->actor->hero;
		pathNode.chainMask = node->actor->chainMask;
		pathNode.specialAction = node->specialAction;
		pathNode.turns = node->turns;
		pathNode.danger = node->danger;
		pathNode.coord = node->coord;
		pathNode.parentIndex = parentIndex;
		pathNode.actionIsBlocked = false;
		pathNode.layer = node->layer;

		if(pathNode.specialAction)
		{
			auto targetNode =node->theNodeBefore ?  getAINode(node->theNodeBefore) : node;

			pathNode.actionIsBlocked = !pathNode.specialAction->canAct(ai, targetNode);
		}

		parentIndex = path.nodes.size();

		path.nodes.push_back(pathNode);
		node = getAINode(node->theNodeBefore);
	}
}

AIPath::AIPath()
	: nodes({})
{
}

std::shared_ptr<const SpecialAction> AIPath::getFirstBlockedAction() const
{
	for(auto node = nodes.rbegin(); node != nodes.rend(); node++)
	{
		if(node->specialAction && node->actionIsBlocked)
			return node->specialAction;
	}

	return std::shared_ptr<const SpecialAction>();
}

int3 AIPath::firstTileToGet() const
{
	if(nodes.size())
	{
		return nodes.back().coord;
	}

	return int3(-1, -1, -1);
}

int3 AIPath::targetTile() const
{
	if(nodes.size())
	{
		return targetNode().coord;
	}

	return int3(-1, -1, -1);
}

const AIPathNodeInfo & AIPath::firstNode() const
{
	return nodes.back();
}

const AIPathNodeInfo & AIPath::targetNode() const
{
	auto & node = nodes.front();

	return targetHero == node.targetHero ? node : nodes.at(1);
}

uint64_t AIPath::getPathDanger() const
{
	if(nodes.empty())
		return 0;

	return targetNode().danger;
}

float AIPath::movementCost() const
{
	if(nodes.empty())
		return 0.0f;

	return targetNode().cost;
}

uint8_t AIPath::turn() const
{
	if(nodes.empty())
		return 0;

	return targetNode().turns;
}

uint64_t AIPath::getHeroStrength() const
{
	return targetHero->getHeroStrength() * getHeroArmyStrengthWithCommander(targetHero, heroArmy);
}

uint64_t AIPath::getTotalDanger() const
{
	uint64_t pathDanger = getPathDanger();
	uint64_t danger = pathDanger > targetObjectDanger ? pathDanger : targetObjectDanger;

	return danger;
}

bool AIPath::containsHero(const CGHeroInstance * hero) const
{
	if(targetHero == hero)
		return true;

	for(auto node : nodes)
	{
		if(node.targetHero == hero)
			return true;
	}

	return false;
}

uint64_t AIPath::getTotalArmyLoss() const
{
	return armyLoss + targetObjectArmyLoss;
}

std::string AIPath::toString() const
{
	std::stringstream str;

	str << targetHero->getNameTranslated() << "[" << std::hex << chainMask << std::dec << "]" << ", turn " << (int)(turn()) << ": ";

	for(auto node : nodes)
		str << node.targetHero->getNameTranslated() << "[" << std::hex << node.chainMask << std::dec << "]" << "->" << node.coord.toString() << "; ";

	return str.str();
}

}
