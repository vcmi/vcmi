/*
 * IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../constants/Enumerations.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;

class CGHeroInstance;
class CGObjectInstance;

struct PlayerSettings;
struct TerrainTile;
struct InfoAboutHero;
struct InfoAboutTown;
struct TeamState;
struct TurnTimerInfo;
struct ArtifactLocation;

class IGameSettings;
class PlayerState;
class UpgradeInfo;
class CMapHeader;
class CGameState;
class PathfinderConfig;
class CArtifactSet;
class CArmedInstance;
class CGTeleport;
class CGTownInstance;
class IMarket;

#if SCRIPTING_ENABLED
namespace scripting
{
class Pool;
}
#endif

namespace vstd
{
class RNG;
}

/// Provide interfaces through which map objects can access game state data
/// TODO: currently it is also used as Environment::GameCb. Consider separating these two interfaces
class DLL_LINKAGE IGameInfoCallback : boost::noncopyable
{
public:
	~IGameInfoCallback() = default;

	/// Access underlying non-const gamestate
	/// TODO: remove ASAP
	virtual CGameState & gameState() = 0;

	/// Access underlying gamestate
	/// TODO: remove
	virtual const CGameState & gameState() const = 0;

	/// Returns current date:
	/// DAY - number of days since start of the game (1..inf)
	/// DAY_OF_WEEK - number of days since start of the week (1..7)
	/// WEEK - number of week within month (1..4)
	/// MONTH - current month (1..inf)
	/// DAY_OF_MONTH - number of day within current month, (1..28)
	virtual int getDate(Date mode=Date::DAY) const = 0;

	/// Return pointer to static map header for current map
	virtual const CMapHeader * getMapHeader() const = 0;

	/// Returns post-randomized startin information on current game
	virtual const StartInfo * getStartInfo() const = 0;

	/// Returns true if corresponding spell is allowed, and not banned in map settings
	virtual bool isAllowed(SpellID id) const = 0;
	/// Returns true if corresponding artifact is allowed, and not banned in map settings
	virtual bool isAllowed(ArtifactID id) const = 0;
	/// Returns true if corresponding secondary skill is allowed, and not banned in map settings
	virtual bool isAllowed(SecondarySkill id) const = 0;

	/// Returns true if specified tile is visible for specific player. Player must be valid
	virtual bool isVisibleFor(int3 pos, PlayerColor player) const = 0;

	/// Returns true if specified object is visible for specific player. Player must be valid
	virtual bool isVisibleFor(const CGObjectInstance * obj, PlayerColor player) const = 0;

	//// Returns game settings for current map
	virtual const IGameSettings & getSettings() const = 0;

	/// Returns dimesions for current map. 'z' coordinate indicates number of level (2 for maps with underground layer)
	virtual int3 getMapSize() const = 0;
	/// Returns true if tile with specified position exists within map
	virtual bool isInTheMap(const int3 &pos) const = 0;

	/// Returns pointer to specified team. Team must be valid
	virtual const TeamState * getTeam(TeamID teamID) const = 0;
	/// Returns pointer to specified team. Player must be valid. Players without belong to a team with single member
	virtual const TeamState * getPlayerTeam(PlayerColor color) const = 0;
	/// Returns current state of a specific player. Player must be valid
	virtual const PlayerState * getPlayerState(PlayerColor color, bool verbose = true) const = 0;
	/// Returns starting settings of a specified player. Player must be valid
	virtual const PlayerSettings * getPlayerSettings(PlayerColor color) const = 0;
	/// Returns relations (allies, enemies, etc) of two specified, valid players
	virtual PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const = 0;
	/// Returns number of wandering heroes or wandering and garrisoned heroes for specified player
	virtual int getHeroCount(PlayerColor player, bool includeGarrisoned) const = 0;
	/// Returns in-game status if specified player. Player must be valid
	virtual EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose = true) const = 0;
	/// Returns amount of resource of a specific type owned by a specified player
	virtual int getResource(PlayerColor Player, GameResID which) const = 0;

	/// Returns pointer to hero using provided object ID. Returns null on failure
	virtual const CGHeroInstance * getHero(ObjectInstanceID objid) const = 0;
	/// Returns pointer to town using provided object ID. Returns null on failure
	virtual const CGTownInstance * getTown(ObjectInstanceID objid) const = 0;
	/// Returns pointer to object using provided object ID. Returns null on failure
	virtual const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const = 0;
	/// Returns pointer to object using provided object ID. Returns null on failure
	virtual const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const = 0;
	/// Returns pointer to artifact using provided object ID. Returns null on failure
	virtual const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const = 0;

	/// Returns pointer to specified tile or nullptr on error
	virtual const TerrainTile * getTile(int3 tile, bool verbose = true) const = 0;
	/// Returns pointer to specified tile without checking for permissions. Avoid its usage!
	virtual const TerrainTile * getTileUnchecked(int3 tile) const = 0;
	/// Returns pointer to top-most object on specified tile, or nullptr on error
	virtual const CGObjectInstance * getTopObj(int3 pos) const = 0;
	/// Returns whether it is possible to dig for Grail on specified tile
	virtual EDiggingStatus getTileDigStatus(int3 tile, bool verbose = true) const = 0;
	/// Calculates pathfinding data into specified pathfinder config
	virtual void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const = 0;

	/// Returns position of creature that guards specified tile, or invalid tile if there are no guards
	virtual int3 guardingCreaturePosition (int3 pos) const = 0;
	/// Return true if src tile is visitable from dst tile
	virtual bool checkForVisitableDir(const int3 & src, const int3 & dst) const = 0;
	/// Returns all wandering monsters that guard specified tile
	virtual std::vector<const CGObjectInstance *> getGuardingCreatures (int3 pos) const = 0;

	/// Returns all tiles within specified range with specific tile visibility mode
	virtual void getTilesInRange(std::unordered_set<int3> & tiles, const int3 & pos, int radius, ETileVisibility mode, std::optional<PlayerColor> player = std::optional<PlayerColor>(), int3::EDistanceFormula formula = int3::DIST_2D) const = 0;

	/// returns all tiles on given level (-1 - both levels, otherwise number of level)
	virtual void getAllTiles(std::unordered_set<int3> &tiles, std::optional<PlayerColor> player, int level, std::function<bool(const TerrainTile *)> filter) const = 0;

	virtual std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const  = 0;
	virtual std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const  = 0;
	virtual std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const  = 0;
	virtual bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const  = 0;
	virtual bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const  = 0;
	virtual bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const  = 0;
	virtual bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const  = 0;

#if SCRIPTING_ENABLED
	virtual scripting::Pool * getGlobalContextPool() const = 0;
#endif
};

VCMI_LIB_NAMESPACE_END
