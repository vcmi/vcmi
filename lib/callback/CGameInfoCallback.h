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

#include "MapInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

struct SThievesGuildInfo;
class Player;

class DLL_LINKAGE CGameInfoCallback : public MapInfoCallback
{
protected:
	const CMap * getMapConstPtr() const override;

	bool hasAccess(std::optional<PlayerColor> playerId) const;

	bool canGetFullInfo(const CGObjectInstance *obj) const; //true we player owns obj or ally owns obj or privileged mode

public:
	//various
	int getDate(Date mode=Date::DAY)const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	const StartInfo * getStartInfo() const override;
	const StartInfo * getInitialStartInfo() const;

	//player
	virtual std::optional<PlayerColor> getPlayerID() const;
	const Player * getPlayer(PlayerColor color) const;
	const PlayerState * getPlayerState(PlayerColor color, bool verbose = true) const override;
	int getResource(PlayerColor Player, GameResID which) const override;
	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj);
	EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose = true) const override;
	bool isPlayerMakingTurn(PlayerColor player) const;
	const PlayerSettings * getPlayerSettings(PlayerColor color) const override;
	TurnTimerInfo getPlayerTurnTime(PlayerColor color) const;

	//map
	bool isVisibleFor(int3 pos, PlayerColor player) const override;
	bool isVisibleFor(const CGObjectInstance * obj, PlayerColor player) const override;
	bool isVisible(const CGObjectInstance * obj) const;
	bool isVisible(int3 pos) const;

	//armed object
	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const;

	//hero
	int getHeroCount(PlayerColor player, bool includeGarrisoned) const override;
	std::vector<const CGHeroInstance*> getHeroes(PlayerColor player) const;
	bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
	int32_t getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const;
	int64_t estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const;
	const CArtifactSet * getArtSet(const ArtifactLocation & loc) const;

	//objects
	const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const override;
	std::vector<const CGObjectInstance *> getBlockingObjs(int3 pos) const;
	std::vector<const CGObjectInstance *> getVisitableObjs(int3 pos, bool verbose = true) const;
	std::vector<const CGObjectInstance *> getAllVisitableObjs() const;
	std::vector<const CGObjectInstance *> getFlaggableObjects(int3 pos) const;
	const CGObjectInstance * getTopObj(int3 pos) const override;
	const IMarket * getMarket(ObjectInstanceID objid) const;

	//map
	int3 guardingCreaturePosition (int3 pos) const override;
	std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const override;
	bool isTileGuardedUnchecked(int3 tile) const override;
	const TerrainTile * getTile(int3 tile, bool verbose = true) const override;
	const TerrainTile * getTileUnchecked(int3 tile) const override;
	void getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula = int3::DIST_2D) const;
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const override;
	EDiggingStatus getTileDigStatus(int3 tile, bool verbose = true) const override;
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const override;

	//town
	int howManyTowns(PlayerColor Player) const;
	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const;
	std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
	EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;
	bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
	const TeamState *getTeam(TeamID teamID) const override;
	const TeamState *getPlayerTeam(PlayerColor color) const override;

	//teleport
	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const override;
	std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const override;
	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const override;
	ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const override;
	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const override;
	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const override;
	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const override;

	//used for random spawns
	void getFreeTiles(std::vector<int3> &tiles) const;
	void getTilesInRange(std::unordered_set<int3> & tiles, const int3 & pos, int radius, ETileVisibility mode, std::optional<PlayerColor> player = std::optional<PlayerColor>(), int3::EDistanceFormula formula = int3::DIST_2D) const override;
	void getAllTiles(std::unordered_set<int3> &tiles, std::optional<PlayerColor> player, int level, const std::function<bool(const TerrainTile *)> & filter) const override;

	void getAllowedSpells(std::vector<SpellID> &out, std::optional<ui16> level = std::nullopt) const;

#if SCRIPTING_ENABLED
	virtual scripting::Pool * getGlobalContextPool() const override;
#endif
};

VCMI_LIB_NAMESPACE_END
