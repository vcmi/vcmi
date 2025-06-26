/*
 * CPathfinder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGPathNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class IGameInfoCallback;
class PathfinderConfig;
class CGWhirlpool;
class TurnInfo;
class CGTeleport;
struct PathfinderOptions;

// Optimized storage - tile can have 0-8 neighbour tiles
// static_vector uses fixed, preallocated storage (capacity) and dynamic size
// this avoid dynamic allocations on huge number of neighbour list queries
using NeighbourTilesVector = boost::container::static_vector<int3, 8>;

// Optimized storage to minimize dynamic allocations - most of teleporters have only one exit, but some may have more (premade maps, castle gates)
using TeleporterTilesVector = boost::container::small_vector<int3, 4>;

class DLL_LINKAGE CPathfinder
{
public:
	friend class CPathfinderHelper;

	CPathfinder(
		const IGameInfoCallback & gameInfo,
		std::shared_ptr<PathfinderConfig> config);

	void calculatePaths(); //calculates possible paths for hero, uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists

private:
	const IGameInfoCallback & gameInfo;

	using ELayer = EPathfindingLayer;

	std::shared_ptr<PathfinderConfig> config;

	boost::heap::fibonacci_heap<CGPathNode *, boost::heap::compare<NodeComparer<CGPathNode>> > pq;

	PathNodeInfo source; //current (source) path node -> we took it from the queue
	CDestinationNodeInfo destination; //destination node -> it's a neighbour of source that we consider

	bool isLayerTransitionPossible() const;
	EPathNodeAction getTeleportDestAction() const;

	bool isDestinationGuardian() const;

	void initializeGraph();

	STRONG_INLINE
	void push(CGPathNode * node);

	STRONG_INLINE
	CGPathNode * topAndPop();
};

class DLL_LINKAGE CPathfinderHelper : boost::noncopyable
{
	/// returns base movement cost for movement between specific tiles. Does not accounts for diagonal movement or last tile exception
	ui32 getTileMovementCost(const TerrainTile & dest, const TerrainTile & from, const TurnInfo * ti) const;

	const IGameInfoCallback & gameInfo;
public:
	enum EPatrolState
	{
		PATROL_NONE = 0,
		PATROL_LOCKED = 1,
		PATROL_RADIUS
	} patrolState;
	std::unordered_set<int3> patrolTiles;

	int turn;
	PlayerColor owner;
	const CGHeroInstance * hero;
	std::vector<std::unique_ptr<TurnInfo>> turnsInfo;
	const PathfinderOptions & options;
	bool canCastFly;
	bool canCastWaterWalk;
	bool whirlpoolProtection;

	CPathfinderHelper(const IGameInfoCallback & gameInfo, const CGHeroInstance * Hero, const PathfinderOptions & Options);
	virtual ~CPathfinderHelper();
	void initializePatrol();
	bool isHeroPatrolLocked() const;
	bool canMoveFromNode(const PathNodeInfo & source) const;
	bool isPatrolMovementAllowed(const int3 & dst) const;
	void updateTurnInfo(const int turn = 0);
	bool isLayerAvailable(const EPathfindingLayer & layer) const;
	const TurnInfo * getTurnInfo() const;
	int getMaxMovePoints(const EPathfindingLayer & layer) const;

	TeleporterTilesVector getCastleGates(const PathNodeInfo & source) const;
	bool isAllowedTeleportEntrance(const CGTeleport * obj) const;
	TeleporterTilesVector getAllowedTeleportChannelExits(const TeleportChannelID & channelID) const;
	bool addTeleportTwoWay(const CGTeleport * obj) const;
	bool addTeleportOneWay(const CGTeleport * obj) const;
	bool addTeleportOneWayRandom(const CGTeleport * obj) const;
	bool addTeleportWhirlpool(const CGWhirlpool * obj) const;
	bool canMoveBetween(const int3 & a, const int3 & b) const; //checks only for visitable objects that may make moving between tiles impossible, not other conditions (like tiles itself accessibility)

	void calculateNeighbourTiles(NeighbourTilesVector & result, const PathNodeInfo & source) const;
	TeleporterTilesVector getTeleportExits(const PathNodeInfo & source) const;

	void getNeighbours(
		const TerrainTile & srcTile,
		const int3 & srcCoord,
		NeighbourTilesVector & vec,
		const boost::logic::tribool & onLand,
		const bool limitCoastSailing) const;

	int getMovementCost(
		const int3 & src,
		const int3 & dst,
		const TerrainTile * ct,
		const TerrainTile * dt,
		const int remainingMovePoints = -1,
		const bool checkLast = true,
		boost::logic::tribool isDstSailLayer = boost::logic::indeterminate,
		boost::logic::tribool isDstWaterLayer = boost::logic::indeterminate) const;

	int getMovementCost(
		const PathNodeInfo & src,
		const PathNodeInfo & dst,
		const int remainingMovePoints = -1,
		const bool checkLast = true) const;

	int movementPointsAfterEmbark(int movement, int basicCost, bool disembark) const;
	bool passOneTurnLimitCheck(const PathNodeInfo & source) const;

	int getGuardiansCount(int3 tile) const;
};

VCMI_LIB_NAMESPACE_END
