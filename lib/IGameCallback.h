#ifndef __IGAMECALLBACK_H__
#define __IGAMECALLBACK_H__

#include "../global.h"
#include <vector>
#include <set>
#include "../client/FunctionList.h"

struct SetMovePoints;
struct GiveBonus;
class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
struct SelectionDialog;
struct YesNoDialog;
struct InfoWindow;
struct MetaString;
struct ShowInInfobox;
struct BattleResult;
class CGameState;
struct PlayerSettings;

class DLL_EXPORT IGameCallback
{
protected:
	CGameState *gs;
public:
	virtual ~IGameCallback(){};

	virtual int getOwner(int heroID);
	virtual int getResource(int player, int which);
	virtual int getDate(int mode=0);
	virtual const CGObjectInstance* getObj(int objid);
	virtual const CGHeroInstance* getHero(int objid);
	virtual const CGTownInstance* getTown(int objid);
	virtual const CGHeroInstance* getSelectedHero(int player); //NULL if no hero is selected
	virtual int getCurrentPlayer()=0;
	virtual int getSelectedHero()=0;
	virtual const PlayerSettings * getPlayerSettings(int color);
	virtual int getHeroCount(int player, bool includeGarrisoned);

	//do sth
	virtual void changeSpells(int hid, bool give, const std::set<ui32> &spells)=0;
	virtual void removeObject(int objid)=0;
	virtual void setBlockVis(int objid, bool bv)=0;
	virtual void setOwner(int objid, ui8 owner)=0;
	virtual void setHoverName(int objid, MetaString * name)=0;
	virtual void setObjProperty(int objid, int prop, int val)=0;
	virtual void changePrimSkill(int ID, int which, int val, bool abs=false)=0;
	virtual void changeSecSkill(int ID, int which, int val, bool abs=false)=0; 
	virtual void showInfoDialog(InfoWindow *iw)=0;
	virtual void showYesNoDialog(YesNoDialog *iw, const CFunctionList<void(ui32)> &callback)=0;
	virtual void showSelectionDialog(SelectionDialog *iw, const CFunctionList<void(ui32)> &callback)=0; //returns question id
	virtual void giveResource(int player, int which, int val)=0;
	virtual void showCompInfo(ShowInInfobox * comp)=0;
	virtual void heroVisitCastle(int obj, int heroID)=0;
	virtual void stopHeroVisitCastle(int obj, int heroID)=0;
	virtual void giveHeroArtifact(int artid, int hid, int position)=0; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	virtual void startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb)=0; //use hero=NULL for no hero
	virtual void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb)=0; //for hero<=>neutral army
	virtual void setAmount(int objid, ui32 val)=0;
	virtual void moveHero(int hid, int3 pos, bool instant)=0;
	virtual void giveHeroBonus(GiveBonus * bonus)=0;
	virtual void setMovePoints(SetMovePoints * smp)=0;
	virtual void setManaPoints(int hid, int val)=0;
	virtual void giveHero(int id, int player)=0;
	virtual void changeObjPos(int objid, int3 newPos, ui8 flags)=0;

	friend struct CPackForClient;
};
#endif // __IGAMECALLBACK_H__