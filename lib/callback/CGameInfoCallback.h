/*
 * CGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IGameInfoCallback.h"

#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

struct PlayerSettings;
struct TerrainTile;
struct InfoAboutHero;
struct InfoAboutTown;
struct SThievesGuildInfo;
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

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CGameInfoCallback : public IGameInfoCallback
{
protected:
	virtual CGameState & gameState() = 0;
	virtual const CGameState & gameState() const = 0;

	bool hasAccess(std::optional<PlayerColor> playerId) const;

	bool canGetFullInfo(const CGObjectInstance *obj) const; //true we player owns obj or ally owns obj or privileged mode
	bool isOwnedOrVisited(const CGObjectInstance *obj) const;

public:
	//various
	int getDate(Date mode=Date::DAY)const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	const StartInfo * getStartInfo() const override;
	const StartInfo * getInitialStartInfo() const override;
	bool isAllowed(SpellID id) const override;
	bool isAllowed(ArtifactID id) const override;
	bool isAllowed(SecondarySkill id) const override;
	const IGameSettings & getSettings() const;

	//player
	std::optional<PlayerColor> getPlayerID() const override;
	const Player * getPlayer(PlayerColor color) const override;
	virtual const PlayerState * getPlayerState(PlayerColor color, bool verbose = true) const;
	virtual int getResource(PlayerColor Player, GameResID which) const;
	virtual PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
	virtual void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
	virtual EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
	virtual bool isPlayerMakingTurn(PlayerColor player) const; //player that currently makes move // TODO synchronous turns
	virtual const PlayerSettings * getPlayerSettings(PlayerColor color) const;
	virtual TurnTimerInfo getPlayerTurnTime(PlayerColor color) const;

	//map
	virtual bool isVisible(int3 pos, const std::optional<PlayerColor> & Player) const;
	virtual bool isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & Player) const;
	virtual bool isVisible(const CGObjectInstance * obj) const;
	virtual bool isVisible(int3 pos) const;

	//armed object
	virtual void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const;

	//hero
	const CGHeroInstance * getHero(ObjectInstanceID objid) const override;
	virtual int getHeroCount(PlayerColor player, bool includeGarrisoned) const;
	virtual bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
	virtual int32_t getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
	virtual int64_t estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
	virtual const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const;
	virtual const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const;
	virtual const CArtifactSet * getArtSet(const ArtifactLocation & loc) const;
	//virtual const CGObjectInstance * getArmyInstance(ObjectInstanceID oid) const;

	//objects
	const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const override;
	virtual std::vector<const CGObjectInstance *> getBlockingObjs(int3 pos) const;
	std::vector<const CGObjectInstance *> getVisitableObjs(int3 pos, bool verbose = true) const override;
	std::vector<const CGObjectInstance *> getAllVisitableObjs() const;
	virtual std::vector<const CGObjectInstance *> getFlaggableObjects(int3 pos) const;
	virtual const CGObjectInstance * getTopObj(int3 pos) const;
	virtual PlayerColor getOwner(ObjectInstanceID heroID) const;
	virtual const IMarket * getMarket(ObjectInstanceID objid) const;

	//map
	virtual int3 guardingCreaturePosition (int3 pos) const;
	virtual std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
	virtual bool isTileGuardedUnchecked(int3 tile) const;
	virtual const CMapHeader * getMapHeader()const;
	virtual int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
	virtual const TerrainTile * getTile(int3 tile, bool verbose = true) const;
	virtual const TerrainTile * getTileUnchecked(int3 tile) const;
	virtual bool isInTheMap(const int3 &pos) const;
	virtual void getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula = int3::DIST_2D) const;
	virtual void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const;
	virtual EDiggingStatus getTileDigStatus(int3 tile, bool verbose = true) const;

	//town
	virtual const CGTownInstance* getTown(ObjectInstanceID objid) const;
	virtual int howManyTowns(PlayerColor Player) const;
	//virtual const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	virtual std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
	virtual std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
	virtual EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
	virtual const TeamState *getTeam(TeamID teamID) const;
	virtual const TeamState *getPlayerTeam(PlayerColor color) const;
	//virtual EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
	virtual std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
	virtual std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	virtual std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	virtual ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	virtual bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	virtual bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	virtual bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	virtual bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const;
};

VCMI_LIB_NAMESPACE_END
