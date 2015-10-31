#pragma once

#include "VCMI_Lib.h"
#include "mapping/CMap.h"
#include "IGameCallback.h"
#include "int3.h"

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
	enum EAccessibility
	{
		NOT_SET = 0,
		ACCESSIBLE = 1, //tile can be entered and passed
		VISITABLE, //tile can be entered as the last tile in path
		BLOCKVIS,  //visitable from neighbouring tile but not passable
		BLOCKED //tile can't be entered nor visited
	};

	EAccessibility accessible;
	ui8 land;
	ui8 turns; //how many turns we have to wait before reachng the tile - 0 means current turn
	ui32 moveRemains; //remaining tiles after hero reaches the tile
	CGPathNode * theNodeBefore;
	int3 coord; //coordinates

	CGPathNode();
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
	mutable boost::mutex pathMx;

	const CGHeroInstance *hero;
	int3 hpos;
	int3 sizes;
	CGPathNode ***nodes; //[w][h][level]

	CPathsInfo(const int3 &Sizes);
	~CPathsInfo();
	const CGPathNode * getPathInfo( int3 tile ) const;
	bool getPath(const int3 &dst, CGPath &out) const;
	int getDistance( int3 tile ) const;
};

class CPathfinder : private CGameInfoCallback
{
public:
	CPathfinder(CPathsInfo &_out, CGameState *_gs, const CGHeroInstance *_hero);
	void calculatePaths(); //calculates possible paths for hero, uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists

private:
	struct PathfinderOptions
	{
		bool useFlying;
		bool useWaterWalking;
		bool useEmbarkAndDisembark;
		bool useTeleportTwoWay; // Two-way monoliths and Subterranean Gate
		bool useTeleportOneWay; // One-way monoliths with one known exit only
		bool useTeleportOneWayRandom; // One-way monoliths with more than one known exit
		bool useTeleportWhirlpool; // Force enabled if hero protected or unaffected (have one stack of one creature)

		PathfinderOptions();
	} options;

	CPathsInfo &out;
	const CGHeroInstance *hero;
	const std::vector<std::vector<std::vector<ui8> > > &FoW;

	std::list<CGPathNode*> mq; //BFS queue -> nodes to be checked

	std::vector<int3> neighbours;

	CGPathNode *cp; //current (source) path node -> we took it from the queue
	CGPathNode *dp; //destination node -> it's a neighbour of cp that we consider
	const TerrainTile *ct, *dt; //tile info for both nodes
	const CGObjectInstance *sTileObj;
	ui8 useEmbarkCost; //0 - usual movement; 1 - embark; 2 - disembark

	void addNeighbours(const int3 &coord);
	void addTeleportExits(bool noTeleportExcludes = false);

	bool isMovementPossible(); //checks if current move will be between sea<->land. If so, checks it legality (returns false if movement is not possible) and sets useEmbarkCost
	bool checkDestinationTile();

	int3 getSourceGuardPosition();
	bool isSourceGuarded();
	bool isDestinationGuarded();
	bool isDestinationGuardian();

	void initializeGraph();

	CGPathNode *getNode(const int3 &coord);
	CGPathNode::EAccessibility evaluateAccessibility(const int3 &pos, const TerrainTile *tinfo) const;
	bool canMoveBetween(const int3 &a, const int3 &b) const; //checks only for visitable objects that may make moving between tiles impossible, not other conditions (like tiles itself accessibility)

	bool addTeleportTwoWay(const CGTeleport * obj) const;
	bool addTeleportOneWay(const CGTeleport * obj) const;
	bool addTeleportOneWayRandom(const CGTeleport * obj) const;
	bool addTeleportWhirlpool(const CGWhirlpool * obj) const;
};
