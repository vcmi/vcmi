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

#include "VCMI_Lib.h"
#include "IGameCallback.h"
#include "bonuses/Bonus.h"
#include "int3.h"

#include <boost/heap/fibonacci_heap.hpp>

VCMI_LIB_NAMESPACE_BEGIN


class CGHeroInstance;
class CGObjectInstance;
struct TerrainTile;
class CPathfinderHelper;
class CMap;
class CGWhirlpool;
class CPathfinderHelper;
class CPathfinder;
class PathfinderConfig;


template<typename N>
struct DLL_LINKAGE NodeComparer
{
	STRONG_INLINE
	bool operator()(const N * lhs, const N * rhs) const
	{
		return lhs->getCost() > rhs->getCost();
	}
};

struct DLL_LINKAGE CGPathNode
{
	using ELayer = EPathfindingLayer;

	enum ENodeAction : ui8
	{
		UNKNOWN = 0,
		EMBARK = 1,
		DISEMBARK,
		NORMAL,
		BATTLE,
		VISIT,
		BLOCKING_VISIT,
		TELEPORT_NORMAL,
		TELEPORT_BLOCKING_VISIT,
		TELEPORT_BATTLE
	};

	enum EAccessibility : ui8
	{
		NOT_SET = 0,
		ACCESSIBLE = 1, //tile can be entered and passed
		VISITABLE, //tile can be entered as the last tile in path
		BLOCKVIS,  //visitable from neighboring tile but not passable
		FLYABLE, //can only be accessed in air layer
		BLOCKED //tile can be neither entered nor visited
	};

	CGPathNode * theNodeBefore;
	int3 coord; //coordinates
	ELayer layer;
	ui32 moveRemains; //remaining movement points after hero reaches the tile
	ui8 turns; //how many turns we have to wait before reaching the tile - 0 means current turn

	EAccessibility accessible;
	ENodeAction action;
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
		accessible = NOT_SET;
		moveRemains = 0;
		cost = std::numeric_limits<float>::max();
		turns = 255;
		theNodeBefore = nullptr;
		action = UNKNOWN;
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
	void update(const int3 & Coord, const ELayer Layer, const EAccessibility Accessible)
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
	CGPathNode::ENodeAction action;
	int turn;
	int movementLeft;
	float cost; //same as CGPathNode::cost
	bool blocked;
	bool isGuardianTile;

	CDestinationNodeInfo();

	virtual void setNode(CGameState * gs, CGPathNode * n) override;

	virtual bool isBetterWay() const;
};

class IPathfindingRule
{
public:
	virtual ~IPathfindingRule() = default;
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const = 0;
};

class DLL_LINKAGE MovementCostRule : public IPathfindingRule
{
public:
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE LayerTransitionRule : public IPathfindingRule
{
public:
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE DestinationActionRule : public IPathfindingRule
{
public:
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE PathfinderBlockingRule : public IPathfindingRule
{
public:
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override
	{
		auto blockingReason = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);

		destination.blocked = blockingReason != BlockingReason::NONE;
	}

protected:
	enum class BlockingReason
	{
		NONE = 0,
		SOURCE_GUARDED = 1,
		DESTINATION_GUARDED = 2,
		SOURCE_BLOCKED = 3,
		DESTINATION_BLOCKED = 4,
		DESTINATION_BLOCKVIS = 5,
		DESTINATION_VISIT = 6
	};

	virtual BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const = 0;
};

class DLL_LINKAGE MovementAfterDestinationRule : public PathfinderBlockingRule
{
public:
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;

protected:
	virtual BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE MovementToDestinationRule : public PathfinderBlockingRule
{
protected:
	virtual BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const override;
};

struct DLL_LINKAGE PathfinderOptions
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
	///
	/// TODO: If this option end up useful for slow devices it's can be improved:
	/// - Allow transition into air layer not only from initial position, but also from teleporters.
	///   Movement into air can be also allowed when hero disembarked.
	/// - Other idea is to allow transition into air within certain radius of N tiles around hero.
	///   Patrol support need similar functionality so it's won't be ton of useless code.
	///   Such limitation could be useful as it's can be scaled depend on device performance.
	bool lightweightFlyingMode;

	/// This option enable one turn limitation for flying and water walking.
	/// So if we're out of MP while cp is blocked or water tile we won't add dest tile to queue.
	///
	/// Following imitation is default H3 mechanics, but someone may want to disable it in mods.
	/// After all this limit should benefit performance on maps with tons of water or blocked tiles.
	///
	/// TODO:
	/// - Behavior when option is disabled not implemented and will lead to crashes.
	bool oneTurnSpecialLayersLimit;

	/// VCMI have different movement rules to solve flaws original engine has.
	/// If this option enabled you'll able to do following things in fly:
	/// - Move from blocked tiles to visitable one
	/// - Move from guarded tiles to blockvis tiles without being attacked
	/// - Move from guarded tiles to guarded visitable tiles with being attacked after
	/// TODO:
	/// - Option should also allow same tile land <-> air layer transitions.
	///   Current implementation only allow go into (from) air layer only to neighbour tiles.
	///   I find it's reasonable limitation, but it's will make some movements more expensive than in H3.
	bool originalMovementRules;

	PathfinderOptions();
};

class DLL_LINKAGE INodeStorage
{
public:
	using ELayer = EPathfindingLayer;

	virtual ~INodeStorage() = default;

	virtual std::vector<CGPathNode *> getInitialNodes() = 0;

	virtual std::vector<CGPathNode *> calculateNeighbours(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) = 0;

	virtual std::vector<CGPathNode *> calculateTeleportations(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) = 0;

	virtual void commit(CDestinationNodeInfo & destination, const PathNodeInfo & source) = 0;

	virtual void initialize(const PathfinderOptions & options, const CGameState * gs) = 0;
};

class DLL_LINKAGE NodeStorage : public INodeStorage
{
private:
	CPathsInfo & out;

	STRONG_INLINE
	void resetTile(const int3 & tile, const EPathfindingLayer & layer, CGPathNode::EAccessibility accessibility);

public:
	NodeStorage(CPathsInfo & pathsInfo, const CGHeroInstance * hero);

	STRONG_INLINE
	CGPathNode * getNode(const int3 & coord, const EPathfindingLayer layer)
	{
		return out.getNode(coord, layer);
	}

	void initialize(const PathfinderOptions & options, const CGameState * gs) override;
	virtual ~NodeStorage() = default;

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
};

class DLL_LINKAGE PathfinderConfig
{
public:
	std::shared_ptr<INodeStorage> nodeStorage;
	std::vector<std::shared_ptr<IPathfindingRule>> rules;
	PathfinderOptions options;

	PathfinderConfig(
		std::shared_ptr<INodeStorage> nodeStorage,
		std::vector<std::shared_ptr<IPathfindingRule>> rules);
	virtual ~PathfinderConfig() = default;

	virtual CPathfinderHelper * getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs) = 0;
};

class DLL_LINKAGE SingleHeroPathfinderConfig : public PathfinderConfig
{
private:
	std::unique_ptr<CPathfinderHelper> pathfinderHelper;

public:
	SingleHeroPathfinderConfig(CPathsInfo & out, CGameState * gs, const CGHeroInstance * hero);
	virtual ~SingleHeroPathfinderConfig() = default;

	virtual CPathfinderHelper * getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs) override;

	static std::vector<std::shared_ptr<IPathfindingRule>> buildRuleSet();
};

class CPathfinder
{
public:
	friend class CPathfinderHelper;

	CPathfinder(
		CGameState * _gs,
		std::shared_ptr<PathfinderConfig> config);

	void calculatePaths(); //calculates possible paths for hero, uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists

private:
	CGameState * gamestate;

	using ELayer = EPathfindingLayer;

	std::shared_ptr<PathfinderConfig> config;

	boost::heap::fibonacci_heap<CGPathNode *, boost::heap::compare<NodeComparer<CGPathNode>> > pq;

	PathNodeInfo source; //current (source) path node -> we took it from the queue
	CDestinationNodeInfo destination; //destination node -> it's a neighbour of source that we consider

	bool isLayerTransitionPossible() const;
	CGPathNode::ENodeAction getTeleportDestAction() const;

	bool isDestinationGuardian() const;

	void initializeGraph();

	STRONG_INLINE
	void push(CGPathNode * node);

	STRONG_INLINE
	CGPathNode * topAndPop();
};

struct DLL_LINKAGE TurnInfo
{
	/// This is certainly not the best design ever and certainly can be improved
	/// Unfortunately for pathfinder that do hundreds of thousands calls onus system add too big overhead
	struct BonusCache {
		std::vector<bool> noTerrainPenalty;
		bool freeShipBoarding;
		bool flyingMovement;
		int flyingMovementVal;
		bool waterWalking;
		int waterWalkingVal;
		int pathfindingVal;

		BonusCache(const TConstBonusListPtr & bonusList);
	};
	std::unique_ptr<BonusCache> bonusCache;

	const CGHeroInstance * hero;
	mutable TConstBonusListPtr bonuses;
	mutable int maxMovePointsLand;
	mutable int maxMovePointsWater;
	TerrainId nativeTerrain;
	int turn;

	TurnInfo(const CGHeroInstance * Hero, const int Turn = 0);
	bool isLayerAvailable(const EPathfindingLayer & layer) const;
	bool hasBonusOfType(const Bonus::BonusType type, const int subtype = -1) const;
	int valOfBonuses(const Bonus::BonusType type, const int subtype = -1) const;
	void updateHeroBonuses(Bonus::BonusType type, const CSelector& sel) const;
	int getMaxMovePoints(const EPathfindingLayer & layer) const;
};

class DLL_LINKAGE CPathfinderHelper : private CGameInfoCallback
{
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
	std::vector<TurnInfo *> turnsInfo;
	const PathfinderOptions & options;

	CPathfinderHelper(CGameState * gs, const CGHeroInstance * Hero, const PathfinderOptions & Options);
	virtual ~CPathfinderHelper();
	void initializePatrol();
	bool isHeroPatrolLocked() const;
	bool isPatrolMovementAllowed(const int3 & dst) const;
	void updateTurnInfo(const int turn = 0);
	bool isLayerAvailable(const EPathfindingLayer & layer) const;
	const TurnInfo * getTurnInfo() const;
	bool hasBonusOfType(const Bonus::BonusType type, const int subtype = -1) const;
	int getMaxMovePoints(const EPathfindingLayer & layer) const;

	std::vector<int3> getCastleGates(const PathNodeInfo & source) const;
	bool isAllowedTeleportEntrance(const CGTeleport * obj) const;
	std::vector<int3> getAllowedTeleportChannelExits(const TeleportChannelID & channelID) const;
	bool addTeleportTwoWay(const CGTeleport * obj) const;
	bool addTeleportOneWay(const CGTeleport * obj) const;
	bool addTeleportOneWayRandom(const CGTeleport * obj) const;
	bool addTeleportWhirlpool(const CGWhirlpool * obj) const;
	bool canMoveBetween(const int3 & a, const int3 & b) const; //checks only for visitable objects that may make moving between tiles impossible, not other conditions (like tiles itself accessibility)

	std::vector<int3> getNeighbourTiles(const PathNodeInfo & source) const;
	std::vector<int3> getTeleportExits(const PathNodeInfo & source) const;

	void getNeighbours(
		const TerrainTile & srcTile,
		const int3 & srcCoord,
		std::vector<int3> & vec,
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
		const bool checkLast = true) const
	{
		return getMovementCost(
			src.coord,
			dst.coord,
			src.tile,
			dst.tile,
			remainingMovePoints,
			checkLast,
			dst.node->layer == EPathfindingLayer::SAIL,
			dst.node->layer == EPathfindingLayer::WATER
		);
	}

	int movementPointsAfterEmbark(int movement, int basicCost, bool disembark) const;
	bool passOneTurnLimitCheck(const PathNodeInfo & source) const;
};

VCMI_LIB_NAMESPACE_END
