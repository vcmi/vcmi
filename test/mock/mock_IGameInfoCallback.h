/*
 * mock_IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/callback/CGameInfoCallback.h"

#if SCRIPTING_ENABLED
#include <vcmi/scripting/Service.h>
#endif

class IGameInfoCallbackMock : public IGameInfoCallback
{
public:
	//various
	MOCK_CONST_METHOD1(getDate, int(Date));
	MOCK_CONST_METHOD0(getStartInfo, const StartInfo *());
	MOCK_CONST_METHOD0(getMapHeader, const CMapHeader *());
	MOCK_CONST_METHOD0(getMapSize, int3());
	MOCK_CONST_METHOD0(getInitialStartInfo, const StartInfo *());

	MOCK_CONST_METHOD1(isAllowed, bool(SpellID));
	MOCK_CONST_METHOD1(isAllowed, bool(ArtifactID));
	MOCK_CONST_METHOD1(isAllowed, bool(SecondarySkill));

	//player
	MOCK_CONST_METHOD1(getPlayer, const Player *(PlayerColor));
	MOCK_CONST_METHOD0(getLocalPlayer, PlayerColor());
	MOCK_CONST_METHOD0(getPlayerID, std::optional<PlayerColor>());

	//hero
	MOCK_CONST_METHOD1(getHero, const CGHeroInstance *(ObjectInstanceID));
	MOCK_CONST_METHOD1(getHeroWithSubid, const CGHeroInstance *(int));

	//objects
	MOCK_CONST_METHOD2(getObj, const CGObjectInstance *(ObjectInstanceID, bool));
	MOCK_CONST_METHOD2(getVisitableObjs, std::vector<const CGObjectInstance*>(int3, bool));

	CGameState & gameState() { throw std::runtime_error("not implemented");}
	const CGameState & gameState() const { throw std::runtime_error("not implemented");}
	const IGameSettings & getSettings() const { throw std::runtime_error("not implemented");}

	MOCK_CONST_METHOD2(isVisibleFor, bool(int3 pos, PlayerColor player));
	MOCK_CONST_METHOD2(isVisibleFor, bool(const CGObjectInstance * obj, PlayerColor player));
	MOCK_CONST_METHOD1(isInTheMap, bool(const int3 & pos));
	MOCK_CONST_METHOD1(getTeam, const TeamState *(TeamID teamID));
	MOCK_CONST_METHOD1(getPlayerTeam, const TeamState *(PlayerColor color));
	MOCK_CONST_METHOD2(getPlayerState, const PlayerState *(PlayerColor color, bool verbose));
	MOCK_CONST_METHOD1(getPlayerSettings, const PlayerSettings *(PlayerColor color));
	MOCK_CONST_METHOD2(getPlayerRelations, PlayerRelations(PlayerColor color1, PlayerColor color2));
	MOCK_CONST_METHOD2(getHeroCount, int(PlayerColor player, bool includeGarrisoned));
	MOCK_CONST_METHOD2(getPlayerStatus, EPlayerStatus(PlayerColor player, bool verbose));
	MOCK_CONST_METHOD2(getResource, int(PlayerColor Player, GameResID which));
	MOCK_CONST_METHOD1(getTown, const CGTownInstance *(ObjectInstanceID objid));
	MOCK_CONST_METHOD1(getObjInstance, const CGObjectInstance *(ObjectInstanceID oid));
	MOCK_CONST_METHOD1(getArtInstance, const CArtifactInstance *(ArtifactInstanceID aid));
	MOCK_CONST_METHOD2(getTile, const TerrainTile *(int3 tile, bool verbose));
	MOCK_CONST_METHOD1(getTileUnchecked, const TerrainTile *(int3 tile));
	MOCK_CONST_METHOD1(getTopObj, const CGObjectInstance *(int3 pos));
	MOCK_CONST_METHOD2(getTileDigStatus, EDiggingStatus(int3 tile, bool verbose));
	MOCK_CONST_METHOD1(calculatePaths, void(const std::shared_ptr<PathfinderConfig> & config));
	MOCK_CONST_METHOD6(getTilesInRange, void( std::unordered_set<int3> & tiles, const int3 & pos, int radius, ETileVisibility mode, std::optional<PlayerColor> player, int3::EDistanceFormula formula));
	MOCK_CONST_METHOD4(getAllTiles, void(std::unordered_set<int3> & tiles, std::optional<PlayerColor> player, int level, std::function<bool(const TerrainTile *)> filter));
	MOCK_CONST_METHOD2(getVisibleTeleportObjects, std::vector<ObjectInstanceID>(std::vector<ObjectInstanceID> ids, PlayerColor player));
	MOCK_CONST_METHOD2(getTeleportChannelEntrances, std::vector<ObjectInstanceID>(TeleportChannelID id, PlayerColor Player));
	MOCK_CONST_METHOD2(getTeleportChannelExits, std::vector<ObjectInstanceID>(TeleportChannelID id, PlayerColor Player));
	MOCK_CONST_METHOD2(isTeleportChannelImpassable, bool(TeleportChannelID id, PlayerColor player));
	MOCK_CONST_METHOD2(isTeleportChannelBidirectional, bool(TeleportChannelID id, PlayerColor player));
	MOCK_CONST_METHOD2(isTeleportChannelUnidirectional, bool(TeleportChannelID id, PlayerColor player));
	MOCK_CONST_METHOD2(isTeleportEntrancePassable, bool(const CGTeleport * obj, PlayerColor player));
	MOCK_CONST_METHOD1(guardingCreaturePosition, int3(int3 pos));
	MOCK_CONST_METHOD2(checkForVisitableDir, bool(const int3 & src, const int3 & dst));
	MOCK_CONST_METHOD1(getGuardingCreatures, std::vector<const CGObjectInstance *>(int3 pos));

	MOCK_METHOD2(pickAllowedArtsSet, void(std::vector<ArtifactID> & out, vstd::RNG & rand));

#if SCRIPTING_ENABLED
	MOCK_CONST_METHOD0(getGlobalContextPool, scripting::Pool *());
#endif
};
