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

#include "int3.h"
#include "ResourceSet.h" // for Res

#define ASSERT_IF_CALLED_WITH_PLAYER if(!getPlayerID()) {logGlobal->error(BOOST_CURRENT_FUNCTION); assert(0);}

VCMI_LIB_NAMESPACE_BEGIN

class Player;
class Team;
class IGameSettings;

struct InfoWindow;
struct PlayerSettings;
struct CPackForClient;
struct TerrainTile;
class PlayerState;
class CTown;
struct StartInfo;
struct CPathsInfo;

struct InfoAboutHero;
struct InfoAboutTown;

class UpgradeInfo;
struct SThievesGuildInfo;
class CMapHeader;
struct TeamState;
struct QuestInfo;
class CGameState;
class PathfinderConfig;
struct TurnTimerInfo;

struct ArtifactLocation;
class CArtifactSet;
class CArmedInstance;
class CGObjectInstance;
class CGHeroInstance;
class CGDwelling;
class CGTeleport;
class CGTownInstance;
class IMarket;

class DLL_LINKAGE IGameInfoCallback : boost::noncopyable
{
public:
	//TODO: all other public methods of CGameInfoCallback

//	//various
	virtual int getDate(Date mode=Date::DAY) const = 0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual const StartInfo * getStartInfo() const = 0;
	virtual const StartInfo * getInitialStartInfo() const = 0;
	virtual bool isAllowed(SpellID id) const = 0;
	virtual bool isAllowed(ArtifactID id) const = 0;
	virtual bool isAllowed(SecondarySkill id) const = 0;

	//player
	virtual std::optional<PlayerColor> getPlayerID() const = 0;
	virtual const Player * getPlayer(PlayerColor color) const = 0;
//	virtual int getResource(PlayerColor Player, EGameResID which) const = 0;
//	bool isVisible(int3 pos) const;
//	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
//	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
//	EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
//	const PlayerSettings * getPlayerSettings(PlayerColor color) const;


//	//armed object
//	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out)const;

	//hero
	virtual const CGHeroInstance * getHero(ObjectInstanceID objid) const = 0;
//	int getHeroCount(PlayerColor player, bool includeGarrisoned) const;
//	bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
//	int32_t getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
//	int64_t estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
//	const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const;
//	const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const;
//	const CGObjectInstance * getArmyInstance(ObjectInstanceID oid) const;

	//objects
	virtual const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const = 0;
//	std::vector <const CGObjectInstance * > getBlockingObjs(int3 pos) const;
	virtual std::vector<const CGObjectInstance *> getVisitableObjs(int3 pos, bool verbose = true) const = 0;
//	std::vector <const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
//	const CGObjectInstance * getTopObj (int3 pos) const;
//	PlayerColor getOwner(ObjectInstanceID heroID) const;

	//map
//	int3 guardingCreaturePosition (int3 pos) const;
//	std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
//	const CMapHeader * getMapHeader()const;
//	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
//	const TerrainTile * getTile(int3 tile, bool verbose = true) const;
//	bool isInTheMap(const int3 &pos) const;

	//town
//	const CGTownInstance* getTown(ObjectInstanceID objid) const;
//	int howManyTowns(PlayerColor Player) const;
//	const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
//	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
//	std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
//	EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
//	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
//	const TeamState *getTeam(TeamID teamID) const;
//	const TeamState *getPlayerTeam(PlayerColor color) const;
//	EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
//	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
//	std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
//	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
//	ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const;
};

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

class DLL_LINKAGE CPlayerSpecificInfoCallback : public CGameInfoCallback
{
public:
	// keep player-specific override in scope
	using CGameInfoCallback::howManyTowns;

	virtual int howManyTowns() const;
	virtual int howManyHeroes(bool includeGarrisoned = true) const;
	virtual int3 getGrailPos(double *outKnownRatio);

	virtual std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	virtual int getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned=true) const;
	virtual const CGTownInstance* getTownBySerial(int serialId) const; // serial id is [0, number of towns)
	virtual const CGHeroInstance* getHeroBySerial(int serialId, bool includeGarrisoned=true) const; // serial id is [0, number of heroes)
	virtual std::vector <const CGHeroInstance *> getHeroesInfo() const;
	virtual std::vector <const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	virtual std::vector <QuestInfo> getMyQuests() const;

	virtual int getResourceAmount(GameResID type) const;
	virtual TResources getResourceAmount() const;
};

VCMI_LIB_NAMESPACE_END
