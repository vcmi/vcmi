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

class CCallback;

extern boost::thread_specific_ptr<CCallback> cb; //for templates

struct AIPathNode : public CGPathNode
{
	uint32_t chainMask;
	uint64_t danger;
	uint32_t manaCost;
	std::shared_ptr<const ISpecialAction> specialAction;
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

	// 1 - layer (air, water, land)
	// 2-4 - position on map[z][x][y]
	// 5 - chain (normal, battle, spellcast and combinations)
	boost::multi_array<AIPathNode, 5> nodes;
	const CPlayerSpecificInfoCallback * cb;
	const VCAI * ai;
	const CGHeroInstance * hero;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;

	STRONG_INLINE
	void resetTile(const int3 & tile, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility);

public:
	/// more than 1 chain layer allows us to have more than 1 path to each tile so we can chose more optimal one.
	static const int NUM_CHAINS = 3;

	// chain flags, can be combined
	static const int NORMAL_CHAIN = 1;
	static const int BATTLE_CHAIN = 2;
	static const int CAST_CHAIN = 4;
	static const int RESOURCE_CHAIN = 8;

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

	bool isBattleNode(const CGPathNode * node) const;
	bool hasBetterChain(const PathNodeInfo & source, CDestinationNodeInfo & destination) const;
	std::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, int chainNumber);
	std::vector<AIPath> getChainInfo(const int3 & pos, bool isOnLand) const;
	bool isTileAccessible(const int3 & pos, const EPathfindingLayer layer) const;

	void setHero(HeroPtr heroPtr, const VCAI * ai);
	const CGHeroInstance * getHero() const { return hero; }

	uint64_t evaluateDanger(const int3 &  tile) const
	{
		return dangerEvaluator->evaluateDanger(tile, hero, ai);
	}

private:
	void calculateTownPortalTeleportations(const PathNodeInfo & source, std::vector<CGPathNode *> & neighbours);
};
