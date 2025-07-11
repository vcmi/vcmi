/*
 * EditorCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorCallback.h"
#include "../lib/mapping/CMap.h"

#define THROW_EDITOR_UNSUPPORTED \
	throw std::runtime_error(std::string("EditorCallback: ") + __func__ + " is not available in map editor")

VCMI_LIB_NAMESPACE_BEGIN

const CMap * EditorCallback::getMapConstPtr() const
{
	if(!map)
		throw std::runtime_error("EditorCallback: map pointer is null");
	return map;
}

EditorCallback::EditorCallback(const CMap * map)
	: map(map)
{}

void EditorCallback::setMap(const CMap * newMap)
{
	map = newMap;
}

CGameState & EditorCallback::gameState()
{
	THROW_EDITOR_UNSUPPORTED;
}

const CGameState & EditorCallback::gameState() const
{
	THROW_EDITOR_UNSUPPORTED;
}

const StartInfo * EditorCallback::getStartInfo() const
{
	THROW_EDITOR_UNSUPPORTED;
}

int EditorCallback::getDate(Date mode) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const TerrainTile * EditorCallback::getTile(int3, bool) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const TerrainTile * EditorCallback::getTileUnchecked(int3) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isTileGuardedUnchecked(int3 tile) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const CGObjectInstance * EditorCallback::getTopObj(int3) const
{
	THROW_EDITOR_UNSUPPORTED;
}

EDiggingStatus EditorCallback::getTileDigStatus(int3, bool) const
{
	THROW_EDITOR_UNSUPPORTED;
}

void EditorCallback::calculatePaths(const std::shared_ptr<PathfinderConfig> &) const
{
	THROW_EDITOR_UNSUPPORTED;
}

int3 EditorCallback::guardingCreaturePosition(int3) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::checkForVisitableDir(const int3 &, const int3 &) const
{
	THROW_EDITOR_UNSUPPORTED;
}

std::vector<const CGObjectInstance*> EditorCallback::getGuardingCreatures(int3) const
{
	THROW_EDITOR_UNSUPPORTED;
}

void EditorCallback::getTilesInRange(std::unordered_set<int3> &, const int3 &, int, ETileVisibility, std::optional<PlayerColor>, int3::EDistanceFormula) const
{
	THROW_EDITOR_UNSUPPORTED;
}

void EditorCallback::getAllTiles(std::unordered_set<int3> &, std::optional<PlayerColor>, int, const std::function<bool(const TerrainTile *)> &) const
{
	THROW_EDITOR_UNSUPPORTED;
}

std::vector<ObjectInstanceID> EditorCallback::getVisibleTeleportObjects(std::vector<ObjectInstanceID>, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

std::vector<ObjectInstanceID> EditorCallback::getTeleportChannelEntrances(TeleportChannelID, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

std::vector<ObjectInstanceID> EditorCallback::getTeleportChannelExits(TeleportChannelID, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isTeleportChannelImpassable(TeleportChannelID, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isTeleportChannelBidirectional(TeleportChannelID, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isTeleportChannelUnidirectional(TeleportChannelID, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isTeleportEntrancePassable(const CGTeleport *, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isVisibleFor(int3 pos, PlayerColor player) const
{
	THROW_EDITOR_UNSUPPORTED;
}

bool EditorCallback::isVisibleFor(const CGObjectInstance *obj, PlayerColor player) const
{
	THROW_EDITOR_UNSUPPORTED;
}

#if SCRIPTING_ENABLED
scripting::Pool * EditorCallback::getGlobalContextPool() const
{
	THROW_EDITOR_UNSUPPORTED;
}
#endif

const TeamState * EditorCallback::getTeam(TeamID) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const TeamState * EditorCallback::getPlayerTeam(PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const PlayerState * EditorCallback::getPlayerState(PlayerColor, bool) const
{
	THROW_EDITOR_UNSUPPORTED;
}

const PlayerSettings * EditorCallback::getPlayerSettings(PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

PlayerRelations EditorCallback::getPlayerRelations(PlayerColor, PlayerColor) const
{
	THROW_EDITOR_UNSUPPORTED;
}

int EditorCallback::getHeroCount(PlayerColor, bool) const
{
	THROW_EDITOR_UNSUPPORTED;
}

EPlayerStatus EditorCallback::getPlayerStatus(PlayerColor, bool) const
{
	THROW_EDITOR_UNSUPPORTED;
}

int EditorCallback::getResource(PlayerColor, GameResID) const
{
	THROW_EDITOR_UNSUPPORTED;
}

VCMI_LIB_NAMESPACE_END
