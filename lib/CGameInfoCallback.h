#pragma once

#include "ResourceSet.h" // for Res::ERes
#include "CBattleCallback.h" //for CCallbackBase

/*
 * CGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGObjectInstance;
struct InfoWindow;
struct PlayerSettings;
struct CPackForClient;
struct TerrainTile;
struct PlayerState;
class CTown;
struct StartInfo;
struct InfoAboutTown;
struct UpgradeInfo;
struct SThievesGuildInfo;
class CGDwelling;
class CGTeleport;
class CMapHeader;
struct TeamState;
struct QuestInfo;
class int3;


class DLL_LINKAGE CGameInfoCallback : public virtual CCallbackBase
{
protected:
	CGameInfoCallback();
	CGameInfoCallback(CGameState *GS, boost::optional<PlayerColor> Player);
	bool hasAccess(boost::optional<PlayerColor> playerId) const;
	bool isVisible(int3 pos, boost::optional<PlayerColor> Player) const;
	bool isVisible(const CGObjectInstance *obj, boost::optional<PlayerColor> Player) const;
	bool isVisible(const CGObjectInstance *obj) const;

	bool canGetFullInfo(const CGObjectInstance *obj) const; //true we player owns obj or ally owns obj or privileged mode
	bool isOwnedOrVisited(const CGObjectInstance *obj) const;

public:
	//various
	int getDate(Date::EDateType mode=Date::DAY)const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	const StartInfo * getStartInfo(bool beforeRandomization = false)const;
	bool isAllowed(int type, int id); //type: 0 - spell; 1- artifact; 2 - secondary skill

	//player
	const PlayerState * getPlayer(PlayerColor color, bool verbose = true) const;
	int getResource(PlayerColor Player, Res::ERes which) const;
	bool isVisible(int3 pos) const;
	PlayerRelations::PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
	EPlayerStatus::EStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
	PlayerColor getCurrentPlayer() const; //player that currently makes move // TODO synchronous turns
	virtual PlayerColor getLocalPlayer() const; //player that is currently owning given client (if not a client, then returns current player)
	const PlayerSettings * getPlayerSettings(PlayerColor color) const;


	//armed object
	void getUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out)const;

	//hero
	const CGHeroInstance* getHero(ObjectInstanceID objid) const;
	const CGHeroInstance* getHeroWithSubid(int subid) const;
	int getHeroCount(PlayerColor player, bool includeGarrisoned) const;
	bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
	int getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
	int estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
	const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const;
	const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const;

	//objects
	const CGObjectInstance* getObj(ObjectInstanceID objid, bool verbose = true) const;
	std::vector <const CGObjectInstance * > getBlockingObjs(int3 pos)const;
	std::vector <const CGObjectInstance * > getVisitableObjs(int3 pos, bool verbose = true)const;
	std::vector <const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
	const CGObjectInstance * getTopObj (int3 pos) const;
	PlayerColor getOwner(ObjectInstanceID heroID) const;
	const CGObjectInstance *getObjByQuestIdentifier(int identifier) const; //nullptr if object has been removed (eg. killed)

	//map
	int3 guardingCreaturePosition (int3 pos) const;
	std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
	const CMapHeader * getMapHeader()const;
	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
	const TerrainTile * getTile(int3 tile, bool verbose = true) const;
	std::shared_ptr<boost::multi_array<TerrainTile*, 3>> getAllVisibleTiles() const;
	bool isInTheMap(const int3 &pos) const;

	//town
	const CGTownInstance* getTown(ObjectInstanceID objid) const;
	int howManyTowns(PlayerColor Player) const;
	const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
	std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
	EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;
	const CTown *getNativeTown(PlayerColor color) const;

	//from gs
	const TeamState *getTeam(TeamID teamID) const;
	const TeamState *getPlayerTeam(PlayerColor color) const;
	EBuildingState::EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
	std::vector<ObjectInstanceID> getTeleportChannelEntraces(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const;
};

class DLL_LINKAGE CPlayerSpecificInfoCallback : public CGameInfoCallback
{
public:
	int howManyTowns() const;
	int howManyHeroes(bool includeGarrisoned = true) const;
	int3 getGrailPos(double *outKnownRatio);
	boost::optional<PlayerColor> getMyColor() const;

	std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	int getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned=true) const;
	const CGTownInstance* getTownBySerial(int serialId) const; // serial id is [0, number of towns)
	const CGHeroInstance* getHeroBySerial(int serialId, bool includeGarrisoned=true) const; // serial id is [0, number of heroes)
	std::vector <const CGHeroInstance *> getHeroesInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	std::vector <const CGDwelling *> getMyDwellings() const; //returns all dwellings that belong to player
	std::vector <const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	std::vector <QuestInfo> getMyQuests() const;

	int getResourceAmount(Res::ERes type) const;
	TResources getResourceAmount() const;
	const std::vector< std::vector< std::vector<ui8> > > & getVisibilityMap()const; //returns visibility map
	const PlayerSettings * getPlayerSettings(PlayerColor color) const;
};

class DLL_LINKAGE IGameEventRealizer
{
public:
	virtual void commitPackage(CPackForClient *pack) = 0;

	virtual void showInfoDialog(InfoWindow *iw);
	virtual void setObjProperty(ObjectInstanceID objid, int prop, si64 val);


	virtual void showInfoDialog(const std::string &msg, PlayerColor player);
};
