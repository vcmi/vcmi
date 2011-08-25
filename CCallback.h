#ifndef __CCALLBACK_H__
#define __CCALLBACK_H__

#include "global.h"
#ifdef _WIN32
#include "tchar.h"
#else
#include "tchar_amigaos4.h" //XXX this is mingw header are we need this for something? for 'true'
//support of unicode we should use ICU or some boost wraper areound it
//(boost using this lib during compilation i dont know what for exactly)
#endif

#include "lib/IGameCallback.h"

/*
 * CCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
class CGameState;
struct CPath;
class CGObjectInstance;
class CArmedInstance;
struct BattleAction;
class CGTownInstance;
struct lua_State;
class CClient;
class IShipyard;
struct CGPathNode;
struct CGPath;
struct CPathsInfo;

class IBattleCallback
{
public:
	bool waitTillRealize; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual int battleMakeAction(BattleAction* action)=0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual bool battleMakeTacticAction(BattleAction * action) =0; // performs tactic phase actions
};

class IGameActionCallback
{
public:
	//hero
	virtual bool moveHero(const CGHeroInstance *h, int3 dst) =0; //dst must be free, neighbouring tile (this function can move hero only by one tile)
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses given hero; true - successfuly, false - not successfuly
	virtual void dig(const CGObjectInstance *hero)=0; 
	virtual void castSpell(const CGHeroInstance *hero, int spellID, const int3 &pos = int3(-1, -1, -1))=0; //cast adventure map spell

	//town
	virtual void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)=0;
	virtual bool buildBuilding(const CGTownInstance *town, si32 buildingID)=0;
	virtual void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1)=0; //if newID==-1 then best possible upgrade will be made
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;

	virtual void trade(const CGObjectInstance *market, int mode, int id1, int id2, int val1, const CGHeroInstance *hero = NULL)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce

	virtual void selectionMade(int selection, int asker) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)=0;//swaps creatures between two possibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)=0;//joins first stack to the second (creatures must be same type)
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2, int val)=0;//split creatures from the first stack
	virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool assembleArtifacts(const CGHeroInstance * hero, ui16 artifactSlot, bool assemble, ui32 assembleTo)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, int stackPos)=0;
	virtual void endTurn()=0;
	virtual void buyArtifact(const CGHeroInstance *hero, int aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void setFormation(const CGHeroInstance * hero, bool tight)=0;
	virtual void setSelection(const CArmedInstance * obj)=0;


	virtual void save(const std::string &fname) = 0;
	virtual void sendMessage(const std::string &mess) = 0;
	virtual void buildBoat(const IShipyard *obj) = 0;
};

class CBattleCallback : public IBattleCallback, public CBattleInfoCallback
{
private:
	CBattleCallback(CGameState *GS, int Player, CClient *C);


protected:
	void sendRequest(const CPack *request);
	CClient *cl;
	//virtual bool hasAccess(int playerId) const;

public:
	int battleMakeAction(BattleAction* action) OVERRIDE;//for casting spells by hero - DO NOT use it for moving active stack
	bool battleMakeTacticAction(BattleAction * action) OVERRIDE; // performs tactic phase actions

	friend class CCallback;
	friend class CClient;
};

class CCallback : public CPlayerSpecificInfoCallback, public IGameActionCallback, public CBattleCallback
{
private:
	CCallback(CGameState * GS, int Player, CClient *C);
public:
	//client-specific functionalities (pathfinding)
	virtual bool getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret); //DEPRACATED!!!
	virtual const CGPathNode *getPathInfo(int3 tile); //uses main, client pathfinder info
	virtual bool getPath2(int3 dest, CGPath &ret); //uses main, client pathfinder info
	virtual void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out, int3 src = int3(-1,-1,-1), int movement = -1);
	virtual void recalculatePaths(); //updates main, client pathfinder info (should be called when moving hero is over)

	void unregisterMyInterface(); //stops delivering information about game events to that player's interface -> can be called ONLY after victory/loss

//commands
	bool moveHero(const CGHeroInstance *h, int3 dst); //dst must be free, neighbouring tile (this function can move hero only by one tile)
	bool teleportHero(const CGHeroInstance *who, const CGTownInstance *where);
	void selectionMade(int selection, int asker);
	int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2);
	int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2); //first goes to the second
	int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2, int val);
	bool dismissHero(const CGHeroInstance * hero);
	bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2);
	bool assembleArtifacts(const CGHeroInstance * hero, ui16 artifactSlot, bool assemble, ui32 assembleTo);
	bool buildBuilding(const CGTownInstance *town, si32 buildingID);
	void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount, si32 level=-1);
	bool dismissCreature(const CArmedInstance *obj, int stackPos);
	bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1);
	void endTurn();
	void swapGarrisonHero(const CGTownInstance *town);
	void buyArtifact(const CGHeroInstance *hero, int aid);
	void trade(const CGObjectInstance *market, int mode, int id1, int id2, int val1, const CGHeroInstance *hero = NULL);
	void setFormation(const CGHeroInstance * hero, bool tight);
	void setSelection(const CArmedInstance * obj);
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero);
	void save(const std::string &fname);
	void sendMessage(const std::string &mess);
	void buildBoat(const IShipyard *obj);
	void dig(const CGObjectInstance *hero); 
	void castSpell(const CGHeroInstance *hero, int spellID, const int3 &pos = int3(-1, -1, -1));

//friends
	friend class CClient;
};

#endif // __CCALLBACK_H__
