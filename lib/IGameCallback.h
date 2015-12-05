#pragma once

#include "CGameInfoCallback.h" // for CGameInfoCallback

/*
 * IGameCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct SetMovePoints;
struct GiveBonus;
struct BlockingDialog;
struct TeleportDialog;
struct MetaString;
struct ShowInInfobox;
struct StackLocation;
struct ArtifactLocation;
class CCreatureSet;
class CStackBasicDescriptor;
class CGCreature;
struct ShashInt3;

class DLL_LINKAGE CPrivilagedInfoCallback : public CGameInfoCallback
{
public:
	CGameState * gameState();
	void getFreeTiles (std::vector<int3> &tiles) const; //used for random spawns
	void getTilesInRange(std::unordered_set<int3, ShashInt3> &tiles, int3 pos, int radious, boost::optional<PlayerColor> player = boost::optional<PlayerColor>(), int mode = 0, bool patrolDistance = false) const;  //mode 1 - only unrevealed tiles; mode 0 - all, mode -1 -  only unrevealed
	void getAllTiles (std::unordered_set<int3, ShashInt3> &tiles, boost::optional<PlayerColor> player = boost::optional<PlayerColor>(), int level=-1, int surface=0) const; //returns all tiles on given level (-1 - both levels, otherwise number of level); surface: 0 - land and water, 1 - only land, 2 - only water
	void pickAllowedArtsSet(std::vector<const CArtifact*> &out); //gives 3 treasures, 3 minors, 1 major -> used by Black Market and Artifact Merchant
	void getAllowedSpells(std::vector<SpellID> &out, ui16 level);

	template<typename Saver>
	void saveCommonState(Saver &out) const; //stores GS and VLC

	template<typename Loader>
	void loadCommonState(Loader &in); //loads GS and VLC
};

class DLL_LINKAGE IGameEventCallback : public IGameEventRealizer
{
public:
	virtual void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells)=0;
	virtual bool removeObject(const CGObjectInstance * obj)=0;
	virtual void setBlockVis(ObjectInstanceID objid, bool bv)=0;
	virtual void setOwner(const CGObjectInstance * objid, PlayerColor owner)=0;
	virtual void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs=false)=0;
	virtual void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false)=0; 
	virtual void showBlockingDialog(BlockingDialog *iw) =0;
	virtual void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) =0; //cb will be called when player closes garrison window
	virtual void showTeleportDialog(TeleportDialog *iw) =0;
	virtual void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) =0;
	virtual void giveResource(PlayerColor player, Res::ERes which, int val)=0;
	virtual void giveResources(PlayerColor player, TResources resources)=0;

	virtual void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) =0;
	virtual void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures) =0;
	virtual bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) =0;
	virtual bool changeStackType(const StackLocation &sl, const CCreature *c) =0;
	virtual bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count = -1) =0; //count -1 => moves whole stack
	virtual bool eraseStack(const StackLocation &sl, bool forceRemoval = false) =0;
	virtual bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) =0;
	virtual bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) =0; //makes new stack or increases count of already existing
	virtual void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) =0; //merges army from src do dst or opens a garrison window
	virtual bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count) = 0;

	virtual void removeAfterVisit(const CGObjectInstance *object) = 0; //object will be destroyed when interaction is over. Do not call when interaction is not ongoing!

	virtual void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, ArtifactPosition pos) = 0;
	virtual void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, ArtifactPosition pos) = 0; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	virtual void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) = 0;
	virtual void removeArtifact(const ArtifactLocation &al) = 0;
	virtual bool moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) = 0;
	virtual void synchronizeArtifactHandlerLists() = 0;

	virtual void showCompInfo(ShowInInfobox * comp)=0;
	virtual void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)=0;
	virtual void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)=0;
	virtual void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr)=0; //use hero=nullptr for no hero
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false)=0; //if any of armies is hero, hero will be used
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false)=0; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	virtual void setAmount(ObjectInstanceID objid, ui32 val)=0;
	virtual bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL)=0;
	virtual void giveHeroBonus(GiveBonus * bonus)=0;
	virtual void setMovePoints(SetMovePoints * smp)=0;
	virtual void setManaPoints(ObjectInstanceID hid, int val)=0;
	virtual void giveHero(ObjectInstanceID id, PlayerColor player)=0;
	virtual void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags)=0;
	virtual void sendAndApply(CPackForClient * info)=0;
	virtual void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)=0; //when two heroes meet on adventure map
	virtual void addQuest(int player, QuestInfo & quest){};
	virtual void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) = 0;
	virtual void changeFogOfWar(std::unordered_set<int3, ShashInt3> &tiles, PlayerColor player, bool hide) = 0;
};

class DLL_LINKAGE CNonConstInfoCallback : public CPrivilagedInfoCallback
{
public:
	PlayerState *getPlayer(PlayerColor color, bool verbose = true);
	TeamState *getTeam(TeamID teamID);//get team by team ID
	TeamState *getPlayerTeam(PlayerColor color);// get team by player color
	CGHeroInstance *getHero(ObjectInstanceID objid);
	CGTownInstance *getTown(ObjectInstanceID objid);
	TerrainTile * getTile(int3 pos);
	CArtifactInstance * getArtInstance(ArtifactInstanceID aid);
	CGObjectInstance * getObjInstance(ObjectInstanceID oid);
};

/// Interface class for handling general game logic and actions
class DLL_LINKAGE IGameCallback : public CPrivilagedInfoCallback, public IGameEventCallback
{
public:
	virtual ~IGameCallback(){};

	//do sth
	const CGObjectInstance *putNewObject(Obj ID, int subID, int3 pos);
	const CGCreature *putNewMonster(CreatureID creID, int count, int3 pos);

	//get info
	virtual bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero);

	friend struct CPack;
	friend struct CPackForClient;
	friend struct CPackForServer;
};

