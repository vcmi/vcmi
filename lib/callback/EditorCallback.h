/*
 * EditorCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/callback/MapInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE EditorCallback : public MapInfoCallback
{

public:
	explicit EditorCallback(const CMap * map);

	void setMap(const CMap * map);
	const CMap * getMapConstPtr() const override;

	// Access to full game state — not available in editor
	CGameState & gameState() override;
	const CGameState & gameState() const override;

	// Unused in editor — return null or dummy
	const StartInfo * getStartInfo() const override;
	int getDate(Date mode) const override;

	const TerrainTile * getTile(int3 tile, bool verbose) const override;
	const TerrainTile * getTileUnchecked(int3 tile) const override;
	bool isTileGuardedUnchecked(int3 tile) const override;
	const CGObjectInstance * getTopObj(int3 pos) const override;
	EDiggingStatus getTileDigStatus(int3 tile, bool verbose) const override;
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const override;
	int3 guardingCreaturePosition(int3 pos) const override;
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const override;
	std::vector<const CGObjectInstance*> getGuardingCreatures(int3 pos) const override;

	void getTilesInRange(std::unordered_set<int3> & tiles, const int3 & pos, int radius, ETileVisibility mode, std::optional<PlayerColor> player, int3::EDistanceFormula formula) const override;
	void getAllTiles(std::unordered_set<int3> &tiles, std::optional<PlayerColor> player, int level, const std::function<bool(const TerrainTile *)> & filter) const override;

	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player) const override;
	std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor player) const override;
	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor player) const override;
	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player) const override;
	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player) const override;
	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player) const override;
	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const override;

	bool isVisibleFor(int3 pos, PlayerColor player) const override;
	bool isVisibleFor(const CGObjectInstance * obj, PlayerColor player) const override;

// Optional scripting
#if SCRIPTING_ENABLED
	scripting::Pool * getGlobalContextPool() const override;
#endif

	// Player-related (stub or throw)
	const TeamState * getTeam(TeamID teamID) const override;
	const TeamState * getPlayerTeam(PlayerColor color) const override;
	const PlayerState * getPlayerState(PlayerColor color, bool verbose) const override;
	const PlayerSettings * getPlayerSettings(PlayerColor color) const override;
	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	int getHeroCount(PlayerColor player, bool includeGarrisoned) const override;
	EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose) const override;
	int getResource(PlayerColor player, GameResID which) const override;

	virtual ~EditorCallback() = default;

private:
	const CMap * map;
};

VCMI_LIB_NAMESPACE_END
