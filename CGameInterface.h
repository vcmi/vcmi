#ifndef __CGAMEINTERFACE_H__
#define __CGAMEINTERFACE_H__
#include "global.h"
#include <set>
#include <vector>
#include "lib/BattleAction.h"
#include "client/FunctionList.h"

/*
 * CGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::logic;
class CCallback;
class IBattleCallback;
class ICallback;
class CGlobalAI;
struct Component;
class CSelectableComponent;
struct TryMoveHero;
class CGHeroInstance;
class CGTownInstance;
class CGObjectInstance;
class CGBlackMarket;
class CGDwelling;
class CCreatureSet;
class CArmedInstance;
class IShipyard;
class IMarket;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
struct BattleSpellCast;
struct SetStackEffect;
struct Bonus;
struct PackageApplied;
struct SetObjectProperty;
struct CatapultAttack;
struct BattleStacksRemoved;
struct StackLocation;
class CStackInstance;
class CStack;
class CCreature;
class CLoadFile;
class CSaveFile;
typedef si32 TQuantity;
template <typename Serializer> class CISer;
template <typename Serializer> class COSer;
struct ArtifactLocation;

class CBattleGameInterface
{
public:
	bool human;
	int playerID;
	std::string dllName;

	virtual ~CBattleGameInterface() {};

	virtual void init(IBattleCallback * CB){};

	//battle call-ins
	virtual void actionFinished(const BattleAction *action){};//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action){};//occurs BEFORE every action taken by any stack or by the hero
	virtual BattleAction activeStack(const CStack * stack)=0; //called when it's turn of that stack
	virtual void battleAttack(const BattleAttack *ba){}; //called when stack is performing attack
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa){}; //called when stack receives damage (after battleAttack())
	virtual void battleEnd(const BattleResult *br){};
	virtual void battleResultsApplied(){}; //called when all effects of last battle are applied
	virtual void battleNewRoundFirst(int round){}; //called at the beginning of each turn before changes are applied;
	virtual void battleNewRound(int round){}; //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(const CStack * stack, THex dest, int distance, bool end){};
	virtual void battleSpellCast(const BattleSpellCast *sc){};
	virtual void battleStacksEffectsSet(const SetStackEffect & sse){};//called when a specific effect is set to stacks
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side){}; //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, si32 lifeDrainFrom){}; //called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	virtual void battleNewStackAppeared(const CStack * stack){}; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	virtual void battleObstaclesRemoved(const std::set<si32> & removedObstacles){}; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	virtual void battleCatapultAttacked(const CatapultAttack & ca){}; //called when catapult makes an attack
	virtual void battleStacksRemoved(const BattleStacksRemoved & bsr){}; //called when certain stack is completely removed from battlefield
};

/// Central class for managing human player / AI interface logic
class CGameInterface : public CBattleGameInterface
{
public:
	virtual void buildChanged(const CGTownInstance *town, int buildingID, int what){}; //what: 1 - built, 2 - demolished

	//garrison operations
	virtual void stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute){}; //if absolute, change is the new count; otherwise count was modified by adding change
	virtual void stackChangedType(const StackLocation &location, const CCreature &newType){}; //used eg. when upgrading creatures
	virtual void stacksErased(const StackLocation &location){}; //stack removed from previously filled slot
	virtual void stacksSwapped(const StackLocation &loc1, const StackLocation &loc2){};
	virtual void newStackInserted(const StackLocation &location, const CStackInstance &stack){}; //new stack inserted at given (previously empty position)
	virtual void stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count){}; //moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
	//virtual void garrisonChanged(const CGObjectInstance * obj){};

	//artifacts operations
	virtual void artifactPut(const ArtifactLocation &al){};
	virtual void artifactRemoved(const ArtifactLocation &al){};
	virtual void artifactAssembled(const ArtifactLocation &al){};
	virtual void artifactDisassembled(const ArtifactLocation &al){};
	virtual void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst){};

	virtual void heroCreated(const CGHeroInstance*){};
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)=0; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void heroInGarrisonChange(const CGTownInstance *town){};
	//virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroMoved(const TryMoveHero & details){};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val){};
	virtual void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val){};
	virtual void heroManaPointsChanged(const CGHeroInstance * hero){} //not called at the beginning of turn and after spell casts
	virtual void heroMovePointsChanged(const CGHeroInstance * hero){} //not called at the beginning of turn and after movement
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void init(ICallback * CB){};
	virtual void receivedResource(int type, int val){};
	virtual void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID){};
	virtual void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level){}
	virtual void showShipyardDialog(const IShipyard *obj){} //obj may be town or shipyard; state: 0 - can buid, 1 - lack of resources, 2 - dest tile is blocked, 3 - no water
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel) = 0; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd) = 0; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void showPuzzleMap(){};
	virtual void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor){};
	virtual void showTavernWindow(const CGObjectInstance *townOrTavern){};
	virtual void advmapSpellCast(const CGHeroInstance * caster, int spellID){}; //called when a hero casts a spell
	virtual void tileHidden(const boost::unordered_set<int3, ShashInt3> &pos){};
	virtual void tileRevealed(const boost::unordered_set<int3, ShashInt3> &pos){};
	virtual void newObject(const CGObjectInstance * obj){}; //eg. ship built in shipyard
	virtual void availableArtifactsChanged(const CGBlackMarket *bm = NULL){}; //bm may be NULL, then artifacts are changed in the global pool (used by merchants in towns)
	virtual void yourTurn(){};
	virtual void centerView (int3 pos, int focusTime){};
	virtual void availableCreaturesChanged(const CGDwelling *town){};
	virtual void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain){};//if gain hero received bonus, else he lost it
	virtual void playerBonusChanged(const Bonus &bonus, bool gain){};//if gain hero received bonus, else he lost it
	virtual void requestRealized(PackageApplied *pa){};
	virtual void heroExchangeStarted(si32 hero1, si32 hero2){};
	virtual void objectPropertyChanged(const SetObjectProperty * sop){}; //eg. mine has been flagged
	virtual void objectRemoved(const CGObjectInstance *obj){}; //eg. collected resource, picked artifact, beaten hero
	virtual void playerBlocked(int reason){}; //reason: 0 - upcoming battle
	virtual void gameOver(ui8 player, bool victory){}; //player lost or won the game
	virtual void serialize(COSer<CSaveFile> &h, const int version){}; //saving
	virtual void serialize(CISer<CLoadFile> &h, const int version){}; //loading
};

class CAIHandler
{
public:
	static CGlobalAI * getNewAI(CCallback * cb, std::string dllname);
	static CBattleGameInterface * getNewBattleAI(CCallback * cb, std::string dllname);
};
class CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	//CGlobalAI();
	virtual void yourTurn() OVERRIDE{};
	virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroCreated(const CGHeroInstance*) OVERRIDE{};
	virtual void battleStackMoved(const CStack * stack, THex dest, int distance, bool end) OVERRIDE{};
	virtual void battleStackAttacking(int ID, int dest) {};
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) OVERRIDE{};
	virtual BattleAction activeStack(const CStack * stack) OVERRIDE;
};

#endif // __CGAMEINTERFACE_H__
