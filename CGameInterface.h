#ifndef CGAMEINTERFACE_H
#define CGAMEINTERFACE_H
#include "global.h"
#include <set>
#include <vector>
#include "lib/BattleAction.h"
#include "client/FunctionList.h"

using namespace boost::logic;
class CCallback;
class ICallback;
class CGlobalAI;
class CGHeroInstance;
class Component;
class CSelectableComponent;
struct HeroMoveDetails;
class CGHeroInstance;
class CGTownInstance;
class CGObjectInstance;
class CCreatureSet;
class CArmedInstance;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
class CObstacle
{
	int ID;
	int position;
	//TODO: add some kind of the blockmap
};

struct StackState
{
	StackState(){attackBonus=defenseBonus=healthBonus=speedBonus=morale=luck=shotsLeft=currentHealth=0;};
	int attackBonus, defenseBonus, healthBonus, speedBonus;
	int currentHealth;
	int shotsLeft;
	std::set<int> effects;
	int morale, luck;
};

class CGameInterface
{
public:
	bool human;
	int playerID, serialID;

	virtual void buildChanged(const CGTownInstance *town, int buildingID, int what){}; //what: 1 - built, 2 - demolished
	virtual void garrisonChanged(const CGObjectInstance * obj){};
	virtual void heroArtifactSetChanged(const CGHeroInstance*hero){};
	virtual void heroCreated(const CGHeroInstance*){};
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)=0; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void heroInGarrisonChange(const CGTownInstance *town){};
	virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroMoved(const HeroMoveDetails & details){};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val){};
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void init(ICallback * CB){};
	virtual void receivedResource(int type, int val){};
	virtual void showInfoDialog(std::string &text, const std::vector<Component*> &components){};
	virtual void showSelDialog(std::string &text, const std::vector<Component*> &components, ui32 askID){};
	virtual void showYesNoDialog(std::string &text, const std::vector<Component*> &components, ui32 askID){};
	virtual void tileHidden(int3 pos){};
	virtual void tileRevealed(int3 pos){};
	virtual void yourTurn(){};
	virtual void availableCreaturesChanged(const CGTownInstance *town){};

	//battle call-ins
	virtual void actionFinished(const BattleAction *action){};//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action){};//occurs BEFORE every action taken by any stack or by the hero
	virtual BattleAction activeStack(int stackID)=0; //called when it's turn of that stack
	virtual void battleAttack(BattleAttack *ba){}; //called when stack is performing attack
	virtual void battleStackAttacked(BattleStackAttacked * bsa){}; //called when stack receives damage (after battleAttack())
	virtual void battleEnd(BattleResult *br){};
	virtual void battleNewRound(int round){}; //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(int ID, int dest){};
	virtual void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side){}; //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles){}; //called when battlefield is prepared, prior the battle beginning
};
class CAIHandler
{
public:
	static CGlobalAI * getNewAI(CCallback * cb, std::string dllname);
};
class CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	//CGlobalAI();
	virtual void yourTurn(){};
	virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroCreated(const CGHeroInstance*){};
	virtual void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving){};
	virtual void battleStackAttacking(int ID, int dest){};
	virtual void battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting){};
	virtual BattleAction activeStack(int stackID) {BattleAction ba; ba.actionType = 3; ba.stackNumber = stackID; return ba;};
};
#endif //CGAMEINTERFACE_H
