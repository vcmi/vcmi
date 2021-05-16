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

#define PATHFINDER_TRACE_LEVEL 1
#define AI_TRACE_LEVEL 2

#include "../../../lib/CPathfinder.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"
#include "../FuzzyHelper.h"
#include "../Goals/AbstractGoal.h"
#include "Actions/SpecialAction.h"
#include "Actors.h"

struct AIPathNode : public CGPathNode
{
	uint64_t danger;
	uint64_t armyLoss;
	uint32_t manaCost;
	const AIPathNode * chainOther;
	std::shared_ptr<const SpecialAction> specialAction;
	const ChainActor * actor;
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

class AINodeStorage : public INodeStorage
{
private:
	int3 sizes;

	const CPlayerSpecificInfoCallback * cb;
	const VCAI * ai;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
	std::vector<std::shared_ptr<ChainActor>> actors;
	std::vector<CGPathNode *> heroChain;
	EHeroChainPass heroChainPass; // true if we need to calculate hero chain
	uint64_t chainMask;
	int heroChainTurn;
	int heroChainMaxTurns;
	PlayerColor playerID;

public:
	/// more than 1 chain layer for each hero allows us to have more than 1 path to each tile so we can chose more optimal one.
	static const int NUM_CHAINS = 10 * GameConstants::MAX_HEROES_PER_PLAYER;
	
	AINodeStorage(const int3 & sizes);
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
		CGPathNode::ENodeAction action,
		int turn,
		int movementLeft,
		float cost) const;

	const AIPathNode * getAINode(const CGPathNode * node) const;
	void updateAINode(CGPathNode * node, std::function<void (AIPathNode *)> updater);

	bool hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;

	bool isMovementIneficient(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
	{
		return hasBetterChain(source, destination);
	}

	bool isDistanceLimitReached(const PathNodeInfo & source, CDestinationNodeInfo & destination) const
	{
		return heroChainPass == EHeroChainPass::CHAIN && destination.node->turns > heroChainTurn;
	}

	template<class NodeRange>
	bool hasBetterChain(
		const CGPathNode * source, 
		const AIPathNode * destinationNode,
		const NodeRange & chains) const;

	boost::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, const ChainActor * actor);
	std::vector<AIPath> getChainInfo(const int3 & pos, bool isOnLand) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & pos, const EPathfindingLayer layer) const;
	void setHeroes(std::vector<HeroPtr> heroes, const VCAI * ai);
	void setTownsAndDwellings(
		const std::vector<const CGTownInstance *> & towns,
		const std::set<const CGObjectInstance *> & visitableObjs);
	const CGHeroInstance * getHero(const CGPathNode * node) const;
	const std::set<const CGHeroInstance *> getAllHeroes() const;
	void clear();
	bool calculateHeroChain();
	bool calculateHeroChainFinal();

	uint64_t evaluateDanger(const int3 &  tile, const CGHeroInstance * hero, bool checkGuards) const
	{
		return dangerEvaluator->evaluateDanger(tile, hero, ai, checkGuards);
	}

	uint64_t evaluateArmyLoss(const CGHeroInstance * hero, uint64_t armyValue, uint64_t danger) const
	{
		double ratio = (double)danger / (armyValue * hero->getFightingStrength());

		return (uint64_t)(armyValue * ratio * ratio * ratio);
	}

private:
	STRONG_INLINE
	void resetTile(const int3 & tile, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility);

	void calculateHeroChain(
		AIPathNode * srcNode, 
		const std::vector<AIPathNode *> & variants, 
		std::vector<ExchangeCandidate> & result) const;

	void calculateHeroChain(
		AIPathNode * carrier, 
		AIPathNode * other, 
		std::vector<ExchangeCandidate> & result) const;
	
	void cleanupInefectiveChains(std::vector<ExchangeCandidate> & result) const;
	void addHeroChain(const std::vector<ExchangeCandidate> & result);

	void calculateTownPortalTeleportations(std::vector<CGPathNode *> & neighbours);
	void fillChainInfo(const AIPathNode * node, AIPath & path, int parentIndex) const;

	ExchangeCandidate calculateExchange(
		ChainActor * exchangeActor, 
		AIPathNode * carrierParentNode, 
		AIPathNode * otherParentNode) const;
};
