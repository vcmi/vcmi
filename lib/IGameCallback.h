#ifndef __IGAMECALLBACK_H__
#define __IGAMECALLBACK_H__

#include "../global.h"
#include <vector>
#include <set>
#include "../client/FunctionList.h"
#include "CCreatureSet.h"

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
class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
struct BlockingDialog;
struct InfoWindow;
struct MetaString;
struct ShowInInfobox;
struct BattleResult;
struct Component;
class CGameState;
struct PlayerSettings;
struct CPackForClient;
class CArtHandler;
class CArtifact;
class CArmedInstance;
struct TerrainTile;
struct PlayerState;
class CTown;
struct StackLocation;
struct ArtifactLocation;
class CArtifactInstance;

class DLL_EXPORT IGameCallback
{
protected:
	CGameState *gs;
public:
	virtual ~IGameCallback(){};

	CGameState *const gameState ();
	virtual int getOwner(int heroID);
	virtual int getResource(int player, int which);
	virtual int getDate(int mode=0); ////mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual const CGObjectInstance* getObj(int objid, bool verbose = true);
	virtual const CGHeroInstance* getHero(int objid);
	virtual const CGTownInstance* getTown(int objid);
	virtual const CGHeroInstance* getSelectedHero(int player); //NULL if no hero is selected
	virtual const CGObjectInstance *getObjByQuestIdentifier(int identifier); //NULL if object has been removed (eg. killed)
	virtual int getCurrentPlayer()=0;
	virtual int getSelectedHero()=0;
	virtual const PlayerSettings * getPlayerSettings(int color);
	virtual int getHeroCount(int player, bool includeGarrisoned);
	virtual void getTilesInRange(boost::unordered_set<int3, ShashInt3> &tiles, int3 pos, int radious, int player=-1, int mode=0);  //mode 1 - only unrevealed tiles; mode 0 - all, mode -1 -  only unrevealed
	virtual void getAllTiles (boost::unordered_set<int3, ShashInt3> &tiles, int player=-1, int level=-1, int surface=0); //returns all tiles on given level (-1 - both levels, otherwise number of level); surface: 0 - land and water, 1 - only land, 2 - only water
	virtual void getFreeTiles (std::vector<int3> &tiles); //used for random spawns
	virtual bool isAllowed(int type, int id); //type: 0 - spell; 1- artifact; 2 - secondary skill
	virtual ui16 getRandomArt (int flags);
	virtual ui16 getArtSync (ui32 rand, int flags); //synchronic
	virtual void pickAllowedArtsSet(std::vector<const CArtifact*> &out); //gives 3 treasures, 3 minors, 1 major -> used by Black Market and Artifact Merchant
	virtual void erasePickedArt (si32 id);
	virtual void getAllowedSpells(std::vector<ui16> &out, ui16 level);
	virtual int3 getMapSize(); //returns size of the map
	virtual TerrainTile * getTile(int3 pos);
	virtual const PlayerState * getPlayerState(int color);
	virtual const CTown *getNativeTown(int color);

	//do sth
	virtual void changeSpells(int hid, bool give, const std::set<ui32> &spells)=0;
	virtual bool removeObject(int objid)=0;
	virtual void setBlockVis(int objid, bool bv)=0;
	virtual void setOwner(int objid, ui8 owner)=0;
	virtual void setHoverName(int objid, MetaString * name)=0;
	virtual void setObjProperty(int objid, int prop, si64 val)=0;
	virtual void changePrimSkill(int ID, int which, si64 val, bool abs=false)=0;
	virtual void changeSecSkill(int ID, int which, int val, bool abs=false)=0; 
	virtual void showInfoDialog(InfoWindow *iw)=0;
	virtual void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback)=0;
	virtual ui32 showBlockingDialog(BlockingDialog *iw) =0; //synchronous version of above //TODO:
	virtual void showGarrisonDialog(int upobj, int hid, bool removableUnits, const boost::function<void()> &cb) =0; //cb will be called when player closes garrison window
	virtual void showThievesGuildWindow(int requestingObjId) =0;
	virtual void giveResource(int player, int which, int val)=0;

	virtual void giveCreatures (const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) =0;
	virtual void takeCreatures (int objid, std::vector<CStackBasicDescriptor> creatures) =0;
	virtual bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) =0;
	virtual bool changeStackType(const StackLocation &sl, CCreature *c) =0;
	virtual bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count = -1) =0; //count -1 => moves whole stack
	virtual bool eraseStack(const StackLocation &sl, bool forceRemoval = false) =0;
	virtual bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) =0;
	virtual bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) =0; //makes new stack or increases count of already existing
	virtual void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) =0; //merges army from src do dst or opens a garrison window
	virtual bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count) = 0;

	virtual void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, int pos) = 0;
	virtual void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, int pos) = 0; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	virtual void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) = 0;
	virtual void removeArtifact(const ArtifactLocation &al) = 0;
	virtual void moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) = 0;

	virtual void showCompInfo(ShowInInfobox * comp)=0;
	virtual void heroVisitCastle(int obj, int heroID)=0;
	virtual void stopHeroVisitCastle(int obj, int heroID)=0;
	//virtual void giveHeroArtifact(int artid, int hid, int position)=0; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	//virtual void giveNewArtifact(int hid, int position)=0;
	//virtual bool removeArtifact(const CArtifact* art, int hid) = 0;
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, boost::function<void(BattleResult*)> cb = 0, const CGTownInstance *town = NULL)=0; //use hero=NULL for no hero
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false)=0; //if any of armies is hero, hero will be used
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false)=0; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	//virtual void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb)=0; //for hero<=>neutral army
	virtual void setAmount(int objid, ui32 val)=0;
	virtual bool moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255)=0;
	virtual void giveHeroBonus(GiveBonus * bonus)=0;
	virtual void setMovePoints(SetMovePoints * smp)=0;
	virtual void setManaPoints(int hid, int val)=0;
	virtual void giveHero(int id, int player)=0;
	virtual void changeObjPos(int objid, int3 newPos, ui8 flags)=0;
	virtual void sendAndApply(CPackForClient * info)=0;
	virtual void heroExchange(si32 hero1, si32 hero2)=0; //when two heroes meet on adventure map

	friend struct CPack;
	friend struct CPackForClient;
	friend struct CPackForServer;
};
#endif // __IGAMECALLBACK_H__