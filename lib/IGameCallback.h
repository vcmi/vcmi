#ifndef __IGAMECALLBACK_H__
#define __IGAMECALLBACK_H__

#include "../global.h"
#include <vector>
#include <set>
#include "../client/FunctionList.h"

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
class CGameState;
struct PlayerSettings;
struct CPackForClient;
class CArtHandler;
class CArtifact;
class CArmedInstance;
struct TerrainTile;

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
	virtual const CGObjectInstance* getObj(int objid);
	virtual const CGHeroInstance* getHero(int objid);
	virtual const CGTownInstance* getTown(int objid);
	virtual const CGHeroInstance* getSelectedHero(int player); //NULL if no hero is selected
	virtual int getCurrentPlayer()=0;
	virtual int getSelectedHero()=0;
	virtual const PlayerSettings * getPlayerSettings(int color);
	virtual int getHeroCount(int player, bool includeGarrisoned);
	virtual void getTilesInRange(std::set<int3> &tiles, int3 pos, int radious, int player=-1, int mode=0);  //mode 1 - only unrevealed tiles; mode 0 - all, mode -1 -  only unrevealed
	virtual void getAllTiles (std::set<int3> &tiles, int player=-1, int level=-1, int surface=0); //returns all tiles on given level (-1 - both levels, otherwise number of level); surface: 0 - land and water, 1 - only land, 2 - only water
	virtual bool isAllowed(int type, int id); //type: 0 - spell; 1- artifact
	virtual void getAllowedArts(std::vector<CArtifact*> &out, std::vector<CArtifact*> CArtHandler::*arts);
	virtual void getAllowed(std::vector<CArtifact*> &out, int flags); //flags: bitfield uses EartClass
	virtual TerrainTile * getTile(int3 pos);

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
	virtual void showGarrisonDialog(int upobj, int hid, const boost::function<void()> &cb) =0; //cb will be called when player closes garrison window
	virtual void giveResource(int player, int which, int val)=0;
	virtual void giveCreatures (int objid, const CGHeroInstance * h, CCreatureSet *creatures)=0;
	virtual void showCompInfo(ShowInInfobox * comp)=0;
	virtual void heroVisitCastle(int obj, int heroID)=0;
	virtual void stopHeroVisitCastle(int obj, int heroID)=0;
	virtual void giveHeroArtifact(int artid, int hid, int position)=0; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank, boost::function<void(BattleResult*)> cb = 0, const CGTownInstance *town = NULL)=0; //use hero=NULL for no hero
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank, boost::function<void(BattleResult*)> cb = 0)=0; //if any of armies is hero, hero will be used
	virtual void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank, boost::function<void(BattleResult*)> cb = 0)=0; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
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


	friend struct CPackForClient;
	friend struct CPackForServer;
};
#endif // __IGAMECALLBACK_H__