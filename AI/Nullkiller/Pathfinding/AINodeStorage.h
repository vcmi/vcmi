/*
* AINodeStorage.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#define NKAI_PATHFINDER_TRACE_LEVEL 0
constexpr int NKAI_GRAPH_TRACE_LEVEL = 0; // To actually enable graph visualization, enter `/vslog graph` in game chat
#define NKAI_TRACE_LEVEL 0

#include "../../../lib/pathfinder/CGPathNode.h"
#include "../../../lib/pathfinder/INodeStorage.h"
#include "Actions/SpecialAction.h"
#include "Actors.h"

namespace NKAI
{
namespace AIPathfinding
{
	const int CHAIN_MAX_DEPTH = 4;
}

enum DayFlags : ui8
{
	NONE = 0,
	FLY_CAST = 1,
	WATER_WALK_CAST = 2
};

struct AIPathNode : public CGPathNode
{
	std::shared_ptr<const SpecialAction> specialAction;

	const AIPathNode * chainOther = nullptr;
	const ChainActor * actor = nullptr;

	uint64_t danger;
	uint64_t armyLoss;
	uint32_t version;

	int16_t manaCost;
	DayFlags dayFlags;

	void addSpecialAction(std::shared_ptr<const SpecialAction> action);

	inline void reset(EPathfindingLayer layer, EPathAccessibility accessibility)
	{
		CGPathNode::reset();

		actor = nullptr;
		danger = 0;
		manaCost = 0;
		specialAction.reset();
		armyLoss = 0;
		chainOther = nullptr;
		dayFlags = DayFlags::NONE;
		this->layer = layer;
		accessible = accessibility;
	}
};

struct AIPathNodeInfo
{
	float cost;
	uint8_t turns;
	int3 coord;
	EPathfindingLayer layer;
	uint64_t danger;
	const CGHeroInstance * targetHero;
	int parentIndex;
	uint64_t chainMask;
	std::shared_ptr<const SpecialAction> specialAction;
	bool actionIsBlocked;
};

struct AIPath
{
	using NodesVector = boost::container::small_vector<AIPathNodeInfo, 16>;

	NodesVector nodes;
	uint64_t targetObjectDanger;
	uint64_t armyLoss;
	uint64_t targetObjectArmyLoss;
	const CGHeroInstance * targetHero;
	const CCreatureSet * heroArmy;
	uint64_t chainMask;
	uint8_t exchangeCount;

	AIPath();

	/// Gets danger of path excluding danger of visiting the target object like creature bank
	uint64_t getPathDanger() const;

	/// Gets danger of path including danger of visiting the target object like creature bank
	uint64_t getTotalDanger() const;

	/// Gets danger of path including danger of visiting the target object like creature bank
	uint64_t getTotalArmyLoss() const;

	int3 firstTileToGet() const;
	int3 targetTile() const;

	const AIPathNodeInfo & firstNode() const;

	const AIPathNodeInfo & targetNode() const;

	float movementCost() const;

	uint8_t turn() const;

	uint64_t getHeroStrength() const;

	std::string toString() const;

	std::shared_ptr<const SpecialAction> getFirstBlockedAction() const;

	bool containsHero(const CGHeroInstance * hero) const;
};

struct ExchangeCandidate : public AIPathNode
{
	AIPathNode * carrierParent = nullptr;
	AIPathNode * otherParent = nullptr;
};

enum EHeroChainPass
{
	INITIAL, // single heroes unlimited distance

	CHAIN, // chains with limited distance

	FINAL // same as SINGLE but for heroes from CHAIN pass
};

class AISharedStorage
{
	// 1-3 - position on map[z][x][y]
	// 4 - chain + layer (normal, battle, spellcast and combinations, water, air)
	static std::shared_ptr<boost::multi_array<AIPathNode, 4>> shared;
	std::shared_ptr<boost::multi_array<AIPathNode, 4>> nodes;
public:
	static std::mutex locker;
	static uint32_t version;

	AISharedStorage(int3 sizes, int numChains);
	~AISharedStorage();

	STRONG_INLINE
	boost::detail::multi_array::sub_array<AIPathNode, 1> get(int3 tile) const
	{
		return (*nodes)[tile.z][tile.x][tile.y];
	}
};

class AINodeStorage : public INodeStorage
{
private:
	int3 sizes;

	std::unique_ptr<boost::multi_array<EPathAccessibility, 4>> accessibility;

	const CPlayerSpecificInfoCallback * cb;
	const Nullkiller * ai;
	AISharedStorage nodes;
	std::vector<std::shared_ptr<ChainActor>> actors;
	std::vector<CGPathNode *> heroChain;
	EHeroChainPass heroChainPass; // true if we need to calculate hero chain
	uint64_t chainMask;
	int heroChainTurn;
	int heroChainMaxTurns;
	PlayerColor playerID;
	uint8_t turnDistanceLimit[2];

public:
	/// more than 1 chain layer for each hero allows us to have more than 1 path to each tile so we can chose more optimal one.	
	AINodeStorage(const Nullkiller * ai, const int3 & sizes);
	~AINodeStorage();

	void initialize(const PathfinderOptions & options, const IGameInfoCallback & gameInfo) override;

	bool increaseHeroChainTurnLimit();
	bool selectFirstActor();
	bool selectNextActor();

	int getBucketCount() const;
	int getBucketSize() const;

	std::vector<CGPathNode *> getInitialNodes() override;

	virtual void calculateNeighbours(
		std::vector<CGPathNode *> & result,
		const PathNodeInfo & source,
		EPathfindingLayer layer,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) override;

	virtual std::vector<CGPathNode *> calculateTeleportations(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) override;

	void commit(CDestinationNodeInfo & destination, const PathNodeInfo & source) override;

	void commit(
		AIPathNode * destination,
		const AIPathNode * source,
		EPathNodeAction action,
		int turn,
		int movementLeft,
		float cost,
		bool saveToCommitted = true) const;

	inline const AIPathNode * getAINode(const CGPathNode * node) const
	{
		return static_cast<const AIPathNode *>(node);
	}

	inline void updateAINode(CGPathNode * node, std::function<void (AIPathNode *)> updater)
	{
		auto * aiNode = static_cast<AIPathNode *>(node);

		updater(aiNode);
	}

	inline const CGHeroInstance * getHero(const CGPathNode * node) const
	{
		const auto * aiNode = getAINode(node);

		return aiNode->actor->hero;
	}

	inline bool blocked(const int3 & tile, EPathfindingLayer layer) const
	{
		EPathAccessibility accessible = getAccessibility(tile, layer);

		return accessible == EPathAccessibility::NOT_SET
			|| accessible == EPathAccessibility::BLOCKED;
	}

	bool hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;
	bool hasBetterChain(const CGPathNode * source, const AIPathNode & candidateNode) const;

	template<class NodeRange>
	bool hasBetterChain(
		const CGPathNode * source, 
		const AIPathNode & destinationNode,
		const NodeRange & chains) const;

	bool isOtherChainBetter(
		const CGPathNode * source,
		const AIPathNode & candidateNode,
		const AIPathNode & other) const;

	bool isMovementInefficient(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
	{
		return hasBetterChain(source, destination);
	}

	bool isDistanceLimitReached(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;

	std::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, const ChainActor * actor);
	void calculateChainInfo(std::vector<AIPath> & result, const int3 & pos, bool isOnLand) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & pos, const EPathfindingLayer layer) const;
	void setHeroes(std::map<const CGHeroInstance *, HeroRole> heroes);
	void setScoutTurnDistanceLimit(uint8_t distanceLimit) { turnDistanceLimit[HeroRole::SCOUT] = distanceLimit; }
	void setMainTurnDistanceLimit(uint8_t distanceLimit) { turnDistanceLimit[HeroRole::MAIN] = distanceLimit; }
	void setTownsAndDwellings(
		const std::vector<const CGTownInstance *> & towns,
		const std::set<const CGObjectInstance *> & visitableObjs);
	const std::set<const CGHeroInstance *> getAllHeroes() const;
	void clear();
	bool calculateHeroChain();
	bool calculateHeroChainFinal();

	uint64_t evaluateArmyLoss(const CGHeroInstance * hero, uint64_t armyValue, uint64_t danger) const;

	inline EPathAccessibility getAccessibility(const int3 & tile, EPathfindingLayer layer) const
	{
		return (*this->accessibility)[tile.z][tile.x][tile.y][layer.getNum()];
	}

	inline void resetTile(const int3 & tile, EPathfindingLayer layer, EPathAccessibility tileAccessibility)
	{
		(*this->accessibility)[tile.z][tile.x][tile.y][layer.getNum()] = tileAccessibility;
	}

	inline int getBucket(const ChainActor * actor) const
	{
		return ((uintptr_t)actor * 395) % getBucketCount();
	}

	void calculateTownPortalTeleportations(std::vector<CGPathNode *> & neighbours);
	void fillChainInfo(const AIPathNode * node, AIPath & path, int parentIndex) const;

	template<typename Fn>
	void iterateValidNodes(const int3 & pos, EPathfindingLayer layer, Fn fn)
	{
		if(blocked(pos, layer))
			return;

		auto chains = nodes.get(pos);

		for(AIPathNode & node : chains)
		{
			if(node.version != AISharedStorage::version || node.layer != layer)
				continue;

			fn(node);
		}
	}

	template<typename Fn>
	bool iterateValidNodesUntil(const int3 & pos, EPathfindingLayer layer, Fn predicate) const
	{
		if(blocked(pos, layer))
			return false;

		auto chains = nodes.get(pos);

		for(AIPathNode & node : chains)
		{
			if(node.version != AISharedStorage::version || node.layer != layer)
				continue;

			if(predicate(node))
				return true;
		}

		return false;
	}


private:
	template<class TVector>
	void calculateTownPortal(
		const ChainActor * actor,
		const std::map<const CGHeroInstance *, int> & maskMap,
		const std::vector<CGPathNode *> & initialNodes,
		TVector & output);
};

}
