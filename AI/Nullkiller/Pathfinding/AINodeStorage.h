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
#define NKAI_TRACE_LEVEL 0

#include "../../../lib/pathfinder/CGPathNode.h"
#include "../../../lib/pathfinder/INodeStorage.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"
#include "../Engine/FuzzyHelper.h"
#include "../Goals/AbstractGoal.h"
#include "Actions/SpecialAction.h"
#include "Actors.h"

namespace NKAI
{
	const int SCOUT_TURN_DISTANCE_LIMIT = 3;
	const int MAIN_TURN_DISTANCE_LIMIT = 5;

namespace AIPathfinding
{
#ifdef ENVIRONMENT64
	const int BUCKET_COUNT = 7;
#else
	const int BUCKET_COUNT = 5;
#endif // ENVIRONMENT64

	const int BUCKET_SIZE = 5;
	const int NUM_CHAINS = BUCKET_COUNT * BUCKET_SIZE;
	const int THREAD_COUNT = 8;
	const int CHAIN_MAX_DEPTH = 4;
}

struct AIPathNode : public CGPathNode
{
	uint64_t danger;
	uint64_t armyLoss;
	uint32_t manaCost;
	const AIPathNode * chainOther;
	std::shared_ptr<const SpecialAction> specialAction;
	const ChainActor * actor;

	STRONG_INLINE
	bool blocked() const
	{
		return accessible == EPathAccessibility::NOT_SET
			|| accessible == EPathAccessibility::BLOCKED;
	}

	void addSpecialAction(std::shared_ptr<const SpecialAction> action);
};

struct AIPathNodeInfo
{
	float cost;
	uint8_t turns;
	int3 coord;
	uint64_t danger;
	const CGHeroInstance * targetHero;
	int parentIndex;
	uint64_t chainMask;
	std::shared_ptr<const SpecialAction> specialAction;
	bool actionIsBlocked;
};

struct AIPath
{
	std::vector<AIPathNodeInfo> nodes;
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
	AIPathNode * carrierParent;
	AIPathNode * otherParent;
};

enum EHeroChainPass
{
	INITIAL, // single heroes unlimited distance

	CHAIN, // chains with limited distance

	FINAL // same as SINGLE but for heroes from CHAIN pass
};

class AISharedStorage
{
	// 1 - layer (air, water, land)
	// 2-4 - position on map[z][x][y]
	// 5 - chain (normal, battle, spellcast and combinations)
	static std::shared_ptr<boost::multi_array<AIPathNode, 5>> shared;
	std::shared_ptr<boost::multi_array<AIPathNode, 5>> nodes;
public:
	static boost::mutex locker;

	AISharedStorage(int3 mapSize);
	~AISharedStorage();

	STRONG_INLINE
	boost::detail::multi_array::sub_array<AIPathNode, 1> get(int3 tile, EPathfindingLayer layer) const
	{
		return (*nodes)[layer][tile.z][tile.x][tile.y];
	}
};

class AINodeStorage : public INodeStorage
{
private:
	int3 sizes;

	const CPlayerSpecificInfoCallback * cb;
	const Nullkiller * ai;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
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

	void initialize(const PathfinderOptions & options, const CGameState * gs) override;

	bool increaseHeroChainTurnLimit();
	bool selectFirstActor();
	bool selectNextActor();

	virtual std::vector<CGPathNode *> getInitialNodes() override;

	virtual std::vector<CGPathNode *> calculateNeighbours(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) override;

	virtual std::vector<CGPathNode *> calculateTeleportations(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) override;

	virtual void commit(CDestinationNodeInfo & destination, const PathNodeInfo & source) override;

	void commit(
		AIPathNode * destination,
		const AIPathNode * source,
		EPathNodeAction action,
		int turn,
		int movementLeft,
		float cost) const;

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

	bool hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;

	bool isMovementIneficient(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
	{
		return hasBetterChain(source, destination);
	}

	bool isDistanceLimitReached(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;

	template<class NodeRange>
	bool hasBetterChain(
		const CGPathNode * source, 
		const AIPathNode * destinationNode,
		const NodeRange & chains) const;

	std::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, const ChainActor * actor);
	std::vector<AIPath> getChainInfo(const int3 & pos, bool isOnLand) const;
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

	inline uint64_t evaluateDanger(const int3 &  tile, const CGHeroInstance * hero, bool checkGuards) const
	{
		return dangerEvaluator->evaluateDanger(tile, hero, checkGuards);
	}

	inline uint64_t evaluateArmyLoss(const CGHeroInstance * hero, uint64_t armyValue, uint64_t danger) const
	{
		double ratio = (double)danger / (armyValue * hero->getFightingStrength());

		return (uint64_t)(armyValue * ratio * ratio * ratio);
	}

	STRONG_INLINE
	void resetTile(const int3 & tile, EPathfindingLayer layer, EPathAccessibility accessibility);

	STRONG_INLINE int getBucket(const ChainActor * actor) const
	{
		return ((uintptr_t)actor * 395) % AIPathfinding::BUCKET_COUNT;
	}

	void calculateTownPortalTeleportations(std::vector<CGPathNode *> & neighbours);
	void fillChainInfo(const AIPathNode * node, AIPath & path, int parentIndex) const;

private:
	template<class TVector>
	void calculateTownPortal(
		const ChainActor * actor,
		const std::map<const CGHeroInstance *, int> & maskMap,
		const std::vector<CGPathNode *> & initialNodes,
		TVector & output);
};

}
