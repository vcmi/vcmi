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
#include "../Goals/AbstractGoal.h"

class AIPathNode;

class ISpecialAction
{
public:
	virtual Goals::TSubgoal whatToDo(HeroPtr hero) const = 0;

	virtual void applyOnDestination(
		HeroPtr hero,
		CDestinationNodeInfo & destination, 
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
	}
};

struct AIPathNode : public CGPathNode
{
	uint32_t chainMask;
	uint64_t danger;
	uint32_t manaCost;
	std::shared_ptr<const ISpecialAction> specialAction;
};

struct AIPathNodeInfo
{
	uint32_t movementPointsLeft;
	uint32_t movementPointsUsed;
	int turns;
	int3 coord;
	uint64_t danger;
};

struct AIPath
{
	std::vector<AIPathNodeInfo> nodes;
	std::shared_ptr<const ISpecialAction> specialAction;

	AIPath();

	/// Gets danger of path excluding danger of visiting the target object like creature bank
	uint64_t getPathDanger() const;

	/// Gets danger of path including danger of visiting the target object like creature bank
	uint64_t getTotalDanger(HeroPtr hero) const;

	int3 firstTileToGet() const;

	uint32_t movementCost() const;
};

class AINodeStorage : public INodeStorage
{
private:
	int3 sizes;

	/// 1-3 - position on map, 4 - layer (air, water, land), 5 - chain (normal, battle, spellcast and combinations)
	boost::multi_array<AIPathNode, 5> nodes;
	const CGHeroInstance * hero;

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

	virtual CGPathNode * getInitialNode() override;
	virtual void resetTile(const int3 & tile, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility) override;

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
	boost::optional<AIPathNode *> getOrCreateNode(const int3 & coord, const EPathfindingLayer layer, int chainNumber);
	std::vector<AIPath> getChainInfo(int3 pos, bool isOnLand) const;

	void setHero(HeroPtr heroPtr)
	{
		hero = heroPtr.get();
	}

	const CGHeroInstance * getHero() const
	{
		return hero;
	}
};
