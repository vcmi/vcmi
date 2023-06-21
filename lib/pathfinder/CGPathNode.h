/*
 * CGPathNode.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "../int3.h"

#include <boost/heap/fibonacci_heap.hpp>

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGObjectInstance;
class CGameState;
class CPathfinderHelper;
struct TerrainTile;

template<typename N>
struct DLL_LINKAGE NodeComparer
{
	STRONG_INLINE
	bool operator()(const N * lhs, const N * rhs) const
	{
		return lhs->getCost() > rhs->getCost();
	}
};

enum class EPathAccessibility : ui8
{
	NOT_SET,
	ACCESSIBLE, //tile can be entered and passed
	VISITABLE, //tile can be entered as the last tile in path
	BLOCKVIS,  //visitable from neighboring tile but not passable
	FLYABLE, //can only be accessed in air layer
	BLOCKED //tile can be neither entered nor visited
};

enum class EPathNodeAction : ui8
{
	UNKNOWN,
	EMBARK,
	DISEMBARK,
	NORMAL,
	BATTLE,
	VISIT,
	BLOCKING_VISIT,
	TELEPORT_NORMAL,
	TELEPORT_BLOCKING_VISIT,
	TELEPORT_BATTLE
};

struct DLL_LINKAGE CGPathNode
{
	using ELayer = EPathfindingLayer;

	CGPathNode * theNodeBefore;
	int3 coord; //coordinates
	ELayer layer;
	ui32 moveRemains; //remaining movement points after hero reaches the tile
	ui8 turns; //how many turns we have to wait before reaching the tile - 0 means current turn

	EPathAccessibility accessible;
	EPathNodeAction action;
	bool locked;
	bool inPQ;

	CGPathNode()
		: coord(-1),
		layer(ELayer::WRONG),
		pqHandle(nullptr)
	{
		reset();
	}

	STRONG_INLINE
	void reset()
	{
		locked = false;
		accessible = EPathAccessibility::NOT_SET;
		moveRemains = 0;
		cost = std::numeric_limits<float>::max();
		turns = 255;
		theNodeBefore = nullptr;
		action = EPathNodeAction::UNKNOWN;
		inPQ = false;
		pq = nullptr;
	}

	STRONG_INLINE
	float getCost() const
	{
		return cost;
	}

	STRONG_INLINE
	void setCost(float value)
	{
		if(value == cost)
			return;

		bool getUpNode = value < cost;
		cost = value;
		// If the node is in the heap, update the heap.
		if(inPQ && pq != nullptr)
		{
			if(getUpNode)
			{
				pq->increase(this->pqHandle);
			}
			else
			{
				pq->decrease(this->pqHandle);
			}
		}
	}

	STRONG_INLINE
	void update(const int3 & Coord, const ELayer Layer, const EPathAccessibility Accessible)
	{
		if(layer == ELayer::WRONG)
		{
			coord = Coord;
			layer = Layer;
		}
		else
		{
			reset();
		}

		accessible = Accessible;
	}

	STRONG_INLINE
	bool reachable() const
	{
		return turns < 255;
	}

	using TFibHeap = boost::heap::fibonacci_heap<CGPathNode *, boost::heap::compare<NodeComparer<CGPathNode>>>;

	TFibHeap::handle_type pqHandle;
	TFibHeap* pq;

private:
	float cost; //total cost of the path to this tile measured in turns with fractions
};

struct DLL_LINKAGE CGPath
{
	std::vector<CGPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
};

struct DLL_LINKAGE CPathsInfo
{
	using ELayer = EPathfindingLayer;

	const CGHeroInstance * hero;
	int3 hpos;
	int3 sizes;
	boost::multi_array<CGPathNode, 4> nodes; //[layer][level][w][h]

	CPathsInfo(const int3 & Sizes, const CGHeroInstance * hero_);
	~CPathsInfo();
	const CGPathNode * getPathInfo(const int3 & tile) const;
	bool getPath(CGPath & out, const int3 & dst) const;
	const CGPathNode * getNode(const int3 & coord) const;

	STRONG_INLINE
	CGPathNode * getNode(const int3 & coord, const ELayer layer)
	{
		return &nodes[layer][coord.z][coord.x][coord.y];
	}
};

struct DLL_LINKAGE PathNodeInfo
{
	CGPathNode * node;
	const CGObjectInstance * nodeObject;
	const CGHeroInstance * nodeHero;
	const TerrainTile * tile;
	int3 coord;
	bool guarded;
	PlayerRelations::PlayerRelations objectRelations;
	PlayerRelations::PlayerRelations heroRelations;
	bool isInitialPosition;

	PathNodeInfo();

	virtual void setNode(CGameState * gs, CGPathNode * n);

	void updateInfo(CPathfinderHelper * hlp, CGameState * gs);

	bool isNodeObjectVisitable() const;
};

struct DLL_LINKAGE CDestinationNodeInfo : public PathNodeInfo
{
	EPathNodeAction action;
	int turn;
	int movementLeft;
	float cost; //same as CGPathNode::cost
	bool blocked;
	bool isGuardianTile;

	CDestinationNodeInfo();

	virtual void setNode(CGameState * gs, CGPathNode * n) override;

	virtual bool isBetterWay() const;
};

VCMI_LIB_NAMESPACE_END
