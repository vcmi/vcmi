#pragma once

#include "VCMI_Lib.h"
#include "mapping/CMap.h"
#include "IGameCallback.h"
#include "int3.h"

#include <boost/heap/priority_queue.hpp>

/*
 * CPathfinder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
class CGObjectInstance;
struct TerrainTile;

struct DLL_LINKAGE CGPathNode
{
	typedef EPathfindingLayer ELayer;

	enum ENodeAction
	{
		UNKNOWN = -1,
		NORMAL = 0,
		EMBARK = 1,
		DISEMBARK, //2
		BATTLE,//3
		VISIT,//4
		BLOCKING_VISIT//5
	};

	enum EAccessibility
	{
		NOT_SET = 0,
		ACCESSIBLE = 1, //tile can be entered and passed
		VISITABLE, //tile can be entered as the last tile in path
		BLOCKVIS,  //visitable from neighbouring tile but not passable
		BLOCKED //tile can't be entered nor visited
	};

	bool locked;
	EAccessibility accessible;
	ui8 turns; //how many turns we have to wait before reachng the tile - 0 means current turn
	ui32 moveRemains; //remaining tiles after hero reaches the tile
	CGPathNode * theNodeBefore;
	int3 coord; //coordinates
	ELayer layer;
	ENodeAction action;

	CGPathNode(int3 Coord, ELayer Layer);
	void reset();
	bool reachable() const;
};

struct DLL_LINKAGE CGPath
{
	std::vector<CGPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
	void convert(ui8 mode); //mode=0 -> from 'manifest' to 'object'
};

struct DLL_LINKAGE CPathsInfo
{
	typedef EPathfindingLayer ELayer;

	mutable boost::mutex pathMx;

	const CGHeroInstance *hero;
	int3 hpos;
	int3 sizes;
	boost::multi_array<CGPathNode *, 4> nodes; //[w][h][level][layer]

	CPathsInfo(const int3 &Sizes);
	~CPathsInfo();
	const CGPathNode * getPathInfo(const int3 &tile, const ELayer &layer = ELayer::AUTO) const;
	bool getPath(CGPath &out, const int3 &dst, const ELayer &layer = ELayer::AUTO) const;
	int getDistance(const int3 &tile, const ELayer &layer = ELayer::AUTO) const;
	CGPathNode *getNode(const int3 &coord, const ELayer &layer) const;
};

class CPathfinder : private CGameInfoCallback
{
public:
	CPathfinder(CPathsInfo &_out, CGameState *_gs, const CGHeroInstance *_hero);
	void startPathfinder();
	void calculatePaths(); //calculates possible paths for hero, uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists

private:
	typedef EPathfindingLayer ELayer;

	struct PathfinderOptions
	{
		bool useFlying;
		bool useWaterWalking;
		bool useEmbarkAndDisembark;
		bool useTeleportTwoWay; // Two-way monoliths and Subterranean Gate
		bool useTeleportOneWay; // One-way monoliths with one known exit only
		bool useTeleportOneWayRandom; // One-way monoliths with more than one known exit
		bool useTeleportWhirlpool; // Force enabled if hero protected or unaffected (have one stack of one creature)

		/// TODO: Find out with client and server code, merge with normal teleporters.
		/// Likely proper implementation would require some refactoring of CGTeleport.
		/// So for now this is unfinished and disabled by default.
		bool useCastleGate;

		/// If true transition into air layer only possible from initial node.
		/// This is drastically decrease path calculation complexity (and time).
		/// Downside is less MP effective paths calculation.
		bool lightweightFlyingMode;

		/// This option enable one turn limitation for flying and water walking.
		/// So if we're out of MP while cp is blocked or water tile we won't add dest tile to queue.
		///
		/// Following imitation is default H3 mechanics, but someone may want to disable it in mods.
		/// After all this limit should benefit performance on maps with tons of water or blocked tiles.
		bool oneTurnSpecialLayersLimit;

		PathfinderOptions();
	} options;

	CPathsInfo &out;
	const CGHeroInstance *hero;

	struct NodeComparer
	{
		bool operator()(const CGPathNode * lhs, const CGPathNode * rhs) const
		{
			if(rhs->turns > lhs->turns)
				return false;
			else if(rhs->turns == lhs->turns && rhs->moveRemains < lhs->moveRemains)
				return false;

			return true;
		}
	};
	boost::heap::priority_queue<CGPathNode *, boost::heap::compare<NodeComparer> > pq;

	std::vector<int3> neighbours;

	CGPathNode *cp; //current (source) path node -> we took it from the queue
	CGPathNode *dp; //destination node -> it's a neighbour of cp that we consider
	const TerrainTile *ct, *dt; //tile info for both nodes
	const CGObjectInstance *sTileObj;
	CGPathNode::ENodeAction destAction;

	void addNeighbours(const int3 &coord);
	void addTeleportExits(bool noTeleportExcludes = false);

	bool isLayerTransitionPossible() const;
	bool isMovementToDestPossible();
	bool isMovementAfterDestPossible() const;

	bool isSourceInitialPosition() const;
	int3 getSourceGuardPosition() const;
	bool isSourceGuarded() const;
	bool isDestinationGuarded(const bool ignoreAccessibility = true) const;
	bool isDestinationGuardian() const;

	void initializeGraph();

	CGPathNode::EAccessibility evaluateAccessibility(const int3 &pos, const TerrainTile *tinfo) const;
	bool canMoveBetween(const int3 &a, const int3 &b) const; //checks only for visitable objects that may make moving between tiles impossible, not other conditions (like tiles itself accessibility)

	bool addTeleportTwoWay(const CGTeleport * obj) const;
	bool addTeleportOneWay(const CGTeleport * obj) const;
	bool addTeleportOneWayRandom(const CGTeleport * obj) const;
	bool addTeleportWhirlpool(const CGWhirlpool * obj) const;

	bool canVisitObject() const;

};
