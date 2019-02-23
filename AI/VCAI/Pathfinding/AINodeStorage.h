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

#include "../../../lib/CPathfinder.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"
#include "../FuzzyHelper.h"
#include "../Goals/AbstractGoal.h"
#include "Actions/ISpecialAction.h"

#define VCMI_TRACE_PATHFINDER

class ChainActor
{
private:
	std::vector<ChainActor> specialActors;

	void copyFrom(ChainActor * base);
	void setupSpecialActors();

public:
	// chain flags, can be combined meaning hero exchange and so on
	uint64_t chainMask;
	bool isMovable;
	bool allowUseResources;
	bool allowBattle;
	bool allowSpellCast;
	const CGHeroInstance * hero;
	const ChainActor * battleActor;
	const ChainActor * castActor;
	const ChainActor * resourceActor;
	int3 initialPosition;
	EPathfindingLayer layer;
	uint32_t initialMovement;
	uint32_t initialTurn;
	uint64_t armyValue;

	ChainActor()
	{
	}

	ChainActor(const CGHeroInstance * hero, int chainMask);
};

struct AIPathNode : public CGPathNode
{
	uint64_t danger;
	uint64_t armyLoss;
	uint32_t manaCost;
	std::shared_ptr<const ISpecialAction> specialAction;
	const ChainActor * actor;
};

struct AIPathNodeInfo
{
	float cost;
	int turns;
	int3 coord;
	uint64_t danger;
};

struct AIPath
{
	std::vector<AIPathNodeInfo> nodes;
	std::shared_ptr<const ISpecialAction> specialAction;
	uint64_t targetObjectDanger;
	const CGHeroInstance * targetHero;

	AIPath();

	/// Gets danger of path excluding danger of visiting the target object like creature bank
	uint64_t getPathDanger() const;

	/// Gets danger of path including danger of visiting the target object like creature bank
	uint64_t getTotalDanger(HeroPtr hero) const;

	int3 firstTileToGet() const;

	float movementCost() const;
};

class AINodeStorage : public INodeStorage
{
private:
	int3 sizes;

	/// 1-3 - position on map, 4 - layer (air, water, land), 5 - chain (normal, battle, spellcast and combinations)
	boost::multi_array<AIPathNode, 5> nodes;
	const CPlayerSpecificInfoCallback * cb;
	const VCAI * ai;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
	std::vector<std::shared_ptr<ChainActor>> actors;

	STRONG_INLINE
	void resetTile(const int3 & tile, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility);

public:
	/// more than 1 chain layer for each hero allows us to have more than 1 path to each tile so we can chose more optimal one.
	static const int NUM_CHAINS = 3 * GameConstants::MAX_HEROES_PER_PLAYER;
	
	AINodeStorage(const int3 & sizes);
	~AINodeStorage();

	void initialize(const PathfinderOptions & options, const CGameState * gs) override;

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

	const AIPathNode * getAINode(const CGPathNode * node) const;
	void updateAINode(CGPathNode * node, std::function<void (AIPathNode *)> updater);

	bool hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;
	boost::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, const ChainActor * actor);
	std::vector<AIPath> getChainInfo(const int3 & pos, bool isOnLand) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & pos, const EPathfindingLayer layer) const;

	void setHeroes(std::vector<HeroPtr> heroes, const VCAI * ai);

	const CGHeroInstance * getHero(const CGPathNode * node) const;

	const std::set<const CGHeroInstance *> getAllHeroes() const;

	void clear();

	uint64_t evaluateDanger(const int3 &  tile, const CGHeroInstance * hero) const
	{
		return dangerEvaluator->evaluateDanger(tile, hero, ai);
	}

private:
	void calculateTownPortalTeleportations(const PathNodeInfo & source, std::vector<CGPathNode *> & neighbours);
};
