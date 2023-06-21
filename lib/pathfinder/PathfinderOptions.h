/*
 * PathfinderOptions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class INodeStorage;
class IPathfindingRule;
class CPathfinderHelper;
class CGameState;
class CGHeroInstance;

struct PathNodeInfo;
struct CPathsInfo;

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
	virtual ~SingleHeroPathfinderConfig();

	virtual CPathfinderHelper * getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs) override;

	static std::vector<std::shared_ptr<IPathfindingRule>> buildRuleSet();
};

VCMI_LIB_NAMESPACE_END
