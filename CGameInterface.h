#ifndef CGAMEINTERFACE_H
#define CGAMEINTERFACE_H
#include "global.h"
#include "CCallback.h"
BOOST_TRIBOOL_THIRD_STATE(outOfRange)

using namespace boost::logic;
class CCallback;
class CGlobalAI;
class CGHeroInstance;
class CSelectableComponent;
class CObstacle
{
	int ID;
	int position;
	//TODO: add some kind of the blockmap
};
struct BattleAction
{
	bool side; //who made this action: false - left, true - right player
	int stackNumber;//stack ID, -1 left hero, -2 right hero, 
	int actionType; //    0 = Cancel BattleAction   1 = Hero cast a spell   2 = Walk   3 = Defend   4 = Retreat from the battle   5 = Surrender   6 = Walk and Attack   7 = Shoot    8 = Wait   9 = Catapult 10 = Monster casts a spell (i.e. Faerie Dragons) 
	int destinationTile; 
	int additionalInfo; // e.g. spell number if type is 1 || 10
};
class CGameInterface
{
public:
	bool human;
	int playerID, serialID;

	virtual void init(ICallback * CB)=0{};
	virtual void yourTurn()=0{};
	virtual void heroKilled(const CGHeroInstance*)=0{};
	virtual void heroCreated(const CGHeroInstance*)=0{};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val)=0{};
	virtual void heroMoved(const HeroMoveDetails & details)=0{};
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void tileRevealed(int3 pos){};
	virtual void tileHidden(int3 pos){};
	virtual void receivedResource(int type, int val){};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID)=0{};
	virtual void garrisonChanged(const CGObjectInstance * obj){};
	virtual void buildChanged(const CGTownInstance *town, int buildingID, int what){}; //what: 1 - built, 2 - demolished
	//battle call-ins 
	virtual void battleStart(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side){}; //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles){}; //called when battlefield is prepared, prior the battle beginning
	virtual void battleNewRound(int round){}; //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void actionStarted(BattleAction action){};//occurs BEFORE every action taken by any stack or by the hero
	virtual void actionFinished(BattleAction action){};//occurs AFTER every action taken by any stack or by the hero
	virtual void activeStack(int stackID){}; //called when it's turn of that stack
	virtual void battleEnd(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, std::vector<int> capturedArtifacts, int expForWinner, bool winner){};
	virtual void battleStackMoved(int ID, int dest)=0;
	//

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
	virtual void battleStackMoved(int ID, int dest){};
};
#endif //CGAMEINTERFACE_H