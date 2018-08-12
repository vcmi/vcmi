/*
* AIhelper.h, part of VCMI engine
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
#include "AIUtility.h"

class IVirtualObject
{
public:
	virtual void materialize();
};

struct AIPathNode : public CGPathNode
{
	uint32_t chainMask;
	uint64_t danger;
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
	static const int NUM_CHAINS = 2;
	static const int NORMAL_CHAIN = 0;
	static const int BATTLE_CHAIN = 1;

	AINodeStorage(const int3 & sizes);
	~AINodeStorage();

	virtual CGPathNode * getInitialNode() override;
	virtual void resetTile(const int3 & tile, EPathfindingLayer layer, CGPathNode::EAccessibility accessibility) override;

	virtual std::vector<CGPathNode *> calculateNeighbours(
		CPathNodeInfo & source,
		CPathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) override;

	virtual std::vector<CGPathNode *> calculateTeleportations(
		CPathNodeInfo & source,
		CPathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) override;

	virtual void commit(CDestinationNodeInfo & destination, CPathNodeInfo & source) override;

	AIPathNode * getAINode(CPathNodeInfo & nodeInfo) const;
	AIPathNode * getAINode(CGPathNode * node) const;
	bool isBattleNode(CGPathNode * node) const;
	bool hasBetterChain(CPathNodeInfo & source, CDestinationNodeInfo & destination) const;
	AIPathNode * getNode(const int3 & coord, const EPathfindingLayer layer, int chainNumber);
	std::vector<AIPath> getChainInfo(int3 pos);

	void setHero(HeroPtr heroPtr)
	{
		hero = heroPtr.get();
	}

	const CGHeroInstance * getHero() const
	{
		return hero;
	}
};
