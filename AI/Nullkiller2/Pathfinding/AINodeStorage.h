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

#include "../../../lib/pathfinder/CGPathNode.h"
#include "../../../lib/pathfinder/INodeStorage.h"
#include "Actions/SpecialAction.h"
#include "Actors.h"
#include "../Helpers/HeroMap.h"

#include <tbb/concurrent_vector.h>

#define NK2AI_PATHFINDER_TRACE_LEVEL 0
constexpr int NK2AI_GRAPH_TRACE_LEVEL = 0; // To actually enable graph visualization, enter `/vslog graph` in game chat
#define NK2AI_TRACE_LEVEL 0

class CSpell;
class DimensionDoorEffect;
struct TeamState;

namespace NK2AI
{
namespace AIPathfinding
{
	const int CHAIN_MAX_DEPTH = 4;
}

uint64_t evaluateArmyLossValue(uint64_t armyValue, uint64_t danger, double fightingStrength);

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

	uint64_t danger = 0;
	uint64_t armyLoss = 0;
	uint32_t version = 0;
	uint64_t storageOrder = std::numeric_limits<uint64_t>::max();

	int16_t manaCost = 0;
	DayFlags dayFlags = DayFlags::NONE;
	uint8_t dimensionDoorCasts = 0;

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
		dimensionDoorCasts = 0;
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

struct AIPathSummary
{
	const AIPathNode * node = nullptr;
	const CGHeroInstance * targetHero = nullptr;
	float cost = 0.f;
	uint8_t exchangeCount = 0;
	uint32_t generation = 0;
	uint64_t stableOrder = 0;
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

class AIPathNodePool
{
	using NodeList = std::vector<AIPathNode *>;

	struct TileNodes
	{
		NodeList nodes;
		uint32_t generation = 0;
	};

	int3 sizes;
	size_t capacity;
	uint32_t generation = 0;
	std::vector<TileNodes> tiles;
	tbb::concurrent_vector<AIPathNode> nodes;

	size_t tileIndex(const int3 & tile) const;
	TileNodes & getCurrentTile(const int3 & tile);

public:
	AIPathNodePool(int3 sizes, size_t capacity);

	bool beginGeneration();
	AIPathNode * allocate(const int3 & tile);
	const NodeList & get(const int3 & tile) const;
	uint64_t nextStorageOrder(const int3 & tile, size_t offset = 0) const;
	uint32_t getGeneration() const { return generation; }
};

class AINodeStorage : public INodeStorage
{
private:
	struct AccessibilityInfo
	{
		EPathAccessibility value = EPathAccessibility::NOT_SET;
		uint32_t generation = 0;
	};

	int3 sizes;
	mutable std::unique_ptr<boost::multi_array<AccessibilityInfo, 4>> accessibility;
	const IGameInfoCallback * gameInfo = nullptr;
	const TeamState * playerTeam = nullptr;
	bool useFlying = false;
	bool useWaterWalking = false;
	Nullkiller * aiNk; // TODO: Mircea: Replace with &
	AIPathNodePool nodes;
	std::vector<std::shared_ptr<ChainActor>> actors;
	std::vector<CGPathNode *> heroChain;
	std::set<int3> committedTiles;
	std::set<int3> committedTilesInitial;
	EHeroChainPass heroChainPass; // true if we need to calculate hero chain
	uint64_t chainMask;
	int heroChainTurn;
	int heroChainMaxTurns;
	PlayerColor playerID;
	uint8_t turnDistanceLimit[2];

public:
	/// more than 1 chain layer for each hero allows us to have more than 1 path to each tile so we can chose more optimal one.
	AINodeStorage(Nullkiller * aiNk, const int3 & sizes);
	~AINodeStorage() override;

	void initialize(const PathfinderOptions & options, const IGameInfoCallback & gameInfo) override;

	bool increaseHeroChainTurnLimit();
	bool selectFirstActor();
	bool selectNextActor();

	int getBucketCount() const;
	int getBucketSize() const;
	bool isCurrentNode(const AIPathNode * node) const { return node->version == nodes.getGeneration(); }
	uint64_t nextStorageOrder(const int3 & tile, size_t offset = 0) const
	{
		return nodes.nextStorageOrder(tile, offset);
	}

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
		bool saveToCommitted = true);

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
	bool hasCurrentNodes(const int3 & pos) const;
	void calculateChainInfo(std::vector<AIPath> & paths, const int3 & pos, bool isOnLand) const;
	void calculatePathSummaries(std::vector<AIPathSummary> & summaries, const int3 & pos, bool isOnLand) const;
	bool calculatePathInfo(AIPath & path, const AIPathSummary & summary) const;
	bool isTileAccessible(const HeroPtr & heroPtr, const int3 & pos, const EPathfindingLayer layer) const;
	void setHeroes(HeroMap<HeroRole> heroes);
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

	EPathAccessibility getAccessibility(const int3 & tile, EPathfindingLayer layer) const;

	void calculateTownPortalTeleportations(std::vector<CGPathNode *> & neighbours);

	using RealMoveMasksByHero = std::map<const CGHeroInstance *, uint64_t>;

	inline bool isRealMovementNode(const AIPathNode * node) const
	{
		return node && node->actor && node->actor->hero && node->coord != node->actor->hero->visitablePos();
	}

	// Reconstructs an AIPath by walking theNodeBefore / chainOther, appending branch nodes first and linking them via parentIndex
	// Returns false when reconstruction would assign conflicting real-move chainMasks to the same hero
	bool tryReconstructChainInfo(const AIPathNode * node, AIPath & path, int & parentIndex, RealMoveMasksByHero & realMoveMasks) const;
	bool calculatePathInfo(AIPath & path, const AIPathNode * node) const;

	template<typename Fn>
	void iterateValidNodes(const int3 & pos, EPathfindingLayer layer, Fn fn)
	{
		if(blocked(pos, layer))
			return;

		const auto & chains = nodes.get(pos);
		for(AIPathNode * node : chains)
		{
			if(node->version != nodes.getGeneration() || node->layer != layer)
				continue;

			fn(*node);
		}
	}

	template<typename Fn>
	bool iterateValidNodesUntil(const int3 & pos, EPathfindingLayer layer, Fn predicate) const
	{
		if(blocked(pos, layer))
			return false;

		const auto & chains = nodes.get(pos);
		for(AIPathNode * node : chains)
		{
			if(node->version != nodes.getGeneration() || node->layer != layer)
				continue;

			if(predicate(*node))
				return true;
		}

		return false;
	}


private:
	struct DimensionDoorCapability
	{
		const CSpell * spell = nullptr;
		const DimensionDoorEffect * effect = nullptr;
		int manaCost = 0;
		int castsLimit = 0;
		int castsAlreadyPerformed = 0;
	};

	struct DimensionDoorSpellPlan
	{
		const CSpell * spell = nullptr;
		const DimensionDoorEffect * effect = nullptr;
		int manaCost = 0;
		int plannedSourceTurn = 0;
		int plannedSourceMoveLimit = 1;
		int plannedSourceMoveRemains = 0;
		int plannedDimensionDoorCasts = 0;
		float destinationCost = 0.f;
	};

	struct DimensionDoorLandingInfo
	{
		const ChainActor * destinationActor = nullptr;
		uint64_t guardedLandingDanger = 0;
		uint64_t guardedLandingArmyLoss = 0;
		bool canLand = true;
	};

	HeroMap<std::vector<DimensionDoorCapability>> dimensionDoorCapabilities;

	const std::vector<DimensionDoorCapability> & getDimensionDoorCapabilities(const CGHeroInstance * hero);

	void calculateObjectTeleportations(
		std::vector<CGPathNode *> & neighbours,
		const PathNodeInfo & source,
		const CPathfinderHelper * pathfinderHelper,
		const AIPathNode * srcNode);

	bool canCalculateDimensionDoorTeleportations(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const AIPathNode * srcNode) const;

	void calculateDimensionDoorTeleportations(
		std::vector<CGPathNode *> & neighbours,
		const PathNodeInfo & source,
		const CPathfinderHelper * pathfinderHelper,
		const AIPathNode * srcNode);

	std::optional<DimensionDoorSpellPlan> getDimensionDoorSpellPlan(
		const PathNodeInfo & source,
		const CPathfinderHelper * pathfinderHelper,
		const AIPathNode * srcNode,
		const CGHeroInstance * hero,
		const DimensionDoorCapability & capability) const;

	void calculateDimensionDoorTeleportationsForSpell(
		std::vector<CGPathNode *> & neighbours,
		const PathNodeInfo & source,
		const AIPathNode * srcNode,
		const DimensionDoorSpellPlan & plan);

	bool isDimensionDoorDestinationValid(
		const PathNodeInfo & source,
		const CGHeroInstance * hero,
		const int3 & destination,
		const DimensionDoorSpellPlan & plan) const;

	DimensionDoorLandingInfo getDimensionDoorLandingInfo(
		const AIPathNode * srcNode,
		const CGHeroInstance * hero,
		const int3 & destination) const;

	void addDimensionDoorTeleportation(
		std::vector<CGPathNode *> & neighbours,
		const PathNodeInfo & source,
		const AIPathNode * srcNode,
		const int3 & destination,
		const DimensionDoorSpellPlan & plan,
		const DimensionDoorLandingInfo & landing);

	template<class TVector>
	void calculateTownPortal(
		const ChainActor * actor,
		const HeroMap<int> & maskMap,
		const std::vector<CGPathNode *> & initialNodes,
		TVector & output);
};

}
