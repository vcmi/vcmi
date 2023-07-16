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
#include "battle/CCallbackBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class Player;
class Team;

struct InfoWindow;
struct PlayerSettings;
struct CPackForClient;
struct TerrainTile;
struct PlayerState;
class CTown;
struct StartInfo;
struct CPathsInfo;

struct InfoAboutHero;
struct InfoAboutTown;

struct UpgradeInfo;
struct SThievesGuildInfo;
class CMapHeader;
struct TeamState;
struct QuestInfo;
class CGameState;
class PathfinderConfig;

class CArmedInstance;
class CGObjectInstance;
class CGHeroInstance;
class CGDwelling;
class CGTeleport;
class CGTownInstance;

class DLL_LINKAGE IGameInfoCallback
{
public:
	//TODO: all other public methods of CGameInfoCallback

//	//various
	virtual int getDate(Date::EDateType mode=Date::DAY) const = 0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
//	const StartInfo * getStartInfo(bool beforeRandomization = false)const;
	virtual bool isAllowed(int32_t type, int32_t id) const = 0; //type: 0 - spell; 1- artifact; 2 - secondary skill

	//player
	virtual const Player * getPlayer(PlayerColor color) const = 0;
//	virtual int getResource(PlayerColor Player, EGameResID which) const = 0;
//	bool isVisible(int3 pos) const;
//	PlayerRelations::PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
//	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
//	EPlayerStatus::EStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
//	PlayerColor getCurrentPlayer() const; //player that currently makes move // TODO synchronous turns
	virtual PlayerColor getLocalPlayer() const = 0; //player that is currently owning given client (if not a client, then returns current player)
//	const PlayerSettings * getPlayerSettings(PlayerColor color) const;


//	//armed object
//	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out)const;

	//hero
	virtual const CGHeroInstance * getHero(ObjectInstanceID objid) const = 0;
	virtual const CGHeroInstance * getHeroWithSubid(int subid) const = 0;
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
//	const CGObjectInstance *getObjByQuestIdentifier(int identifier) const; //nullptr if object has been removed (eg. killed)

	//map
//	int3 guardingCreaturePosition (int3 pos) const;
//	std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
//	const CMapHeader * getMapHeader()const;
//	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
//	const TerrainTile * getTile(int3 tile, bool verbose = true) const;
//	std::shared_ptr<boost::multi_array<TerrainTile*, 3>> getAllVisibleTiles() const;
//	bool isInTheMap(const int3 &pos) const;
//	void getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula = int3::DIST_2D) const;

	//town
//	const CGTownInstance* getTown(ObjectInstanceID objid) const;
//	int howManyTowns(PlayerColor Player) const;
//	const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
//	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
//	std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
//	EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
//	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
//	const TeamState *getTeam(TeamID teamID) const;
//	const TeamState *getPlayerTeam(PlayerColor color) const;
//	EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
//	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
//	std::vector<ObjectInstanceID> getTeleportChannelEntraces(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
//	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
//	ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
//	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const;
};

class DLL_LINKAGE CGameInfoCallback : public virtual CCallbackBase, public IGameInfoCallback
{
protected:
	CGameState * gs;//todo: replace with protected const getter, only actual Server and Client objects should hold game state

	CGameInfoCallback() = default;
	CGameInfoCallback(CGameState * GS, std::optional<PlayerColor> Player);
	bool hasAccess(std::optional<PlayerColor> playerId) const;

	bool canGetFullInfo(const CGObjectInstance *obj) const; //true we player owns obj or ally owns obj or privileged mode
	bool isOwnedOrVisited(const CGObjectInstance *obj) const;

public:
	//various
	int getDate(Date::EDateType mode=Date::DAY)const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual const StartInfo * getStartInfo(bool beforeRandomization = false)const;
	bool isAllowed(int32_t type, int32_t id) const override; //type: 0 - spell; 1- artifact; 2 - secondary skill

	//player
	const Player * getPlayer(PlayerColor color) const override;
	virtual const PlayerState * getPlayerState(PlayerColor color, bool verbose = true) const;
	virtual int getResource(PlayerColor Player, GameResID which) const;
	virtual PlayerRelations::PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
	virtual void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
	virtual EPlayerStatus::EStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
	virtual PlayerColor getCurrentPlayer() const; //player that currently makes move // TODO synchronous turns
	PlayerColor getLocalPlayer() const override; //player that is currently owning given client (if not a client, then returns current player)
	virtual const PlayerSettings * getPlayerSettings(PlayerColor color) const;

	//map
	virtual bool isVisible(int3 pos, const std::optional<PlayerColor> & Player) const;
	virtual bool isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & Player) const;
	virtual bool isVisible(const CGObjectInstance * obj) const;
	virtual bool isVisible(int3 pos) const;


	//armed object
	virtual void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out)const;

	//hero
	virtual const CGHeroInstance * getHero(ObjectInstanceID objid) const override;
	const CGHeroInstance * getHeroWithSubid(int subid) const override;
	virtual int getHeroCount(PlayerColor player, bool includeGarrisoned) const;
	virtual bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
	virtual int32_t getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
	virtual int64_t estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
	virtual const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const;
	virtual const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const;
	//virtual const CGObjectInstance * getArmyInstance(ObjectInstanceID oid) const;

	//objects
	virtual const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const override;
	virtual std::vector <const CGObjectInstance * > getBlockingObjs(int3 pos)const;
	virtual std::vector <const CGObjectInstance * > getVisitableObjs(int3 pos, bool verbose = true) const override;
	virtual std::vector <const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
	virtual const CGObjectInstance * getTopObj (int3 pos) const;
	virtual PlayerColor getOwner(ObjectInstanceID heroID) const;
	virtual const CGObjectInstance *getObjByQuestIdentifier(int identifier) const; //nullptr if object has been removed (eg. killed)

	//map
	virtual int3 guardingCreaturePosition (int3 pos) const;
	virtual std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
	virtual const CMapHeader * getMapHeader()const;
	virtual int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
	virtual const TerrainTile * getTile(int3 tile, bool verbose = true) const;
	virtual std::shared_ptr<const boost::multi_array<TerrainTile*, 3>> getAllVisibleTiles() const;
	virtual bool isInTheMap(const int3 &pos) const;
	virtual void getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula = int3::DIST_2D) const;
	virtual void calculatePaths(const std::shared_ptr<PathfinderConfig> & config);
	virtual void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out);
	virtual EDiggingStatus getTileDigStatus(int3 tile, bool verbose = true) const;

	//town
	virtual const CGTownInstance* getTown(ObjectInstanceID objid) const;
	virtual int howManyTowns(PlayerColor Player) const;
	//virtual const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	virtual std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
	virtual std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
	virtual EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
	virtual const TeamState *getTeam(TeamID teamID) const;
	virtual const TeamState *getPlayerTeam(PlayerColor color) const;
	//virtual EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
	virtual std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
	virtual std::vector<ObjectInstanceID> getTeleportChannelEntraces(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
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
	virtual std::optional<PlayerColor> getMyColor() const;

	virtual std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	virtual int getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned=true) const;
	virtual const CGTownInstance* getTownBySerial(int serialId) const; // serial id is [0, number of towns)
	virtual const CGHeroInstance* getHeroBySerial(int serialId, bool includeGarrisoned=true) const; // serial id is [0, number of heroes)
	virtual std::vector <const CGHeroInstance *> getHeroesInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	virtual std::vector <const CGDwelling *> getMyDwellings() const; //returns all dwellings that belong to player
	virtual std::vector <const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	virtual std::vector <QuestInfo> getMyQuests() const;

	virtual int getResourceAmount(GameResID type) const;
	virtual TResources getResourceAmount() const;
	virtual std::shared_ptr<const boost::multi_array<ui8, 3>> getVisibilityMap() const; //returns visibility map
	//virtual const PlayerSettings * getPlayerSettings(PlayerColor color) const;
};

VCMI_LIB_NAMESPACE_END
