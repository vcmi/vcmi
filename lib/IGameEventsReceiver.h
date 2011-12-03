#pragma once
#include "../global.h"
#include <set>

class CGTownInstance;
class CCreature;
class CArmedInstance;
struct StackLocation;
struct TryMoveHero;
struct ArtifactLocation;
class CGHeroInstance;
class CCallback;
class IShipyard;
class CGDwelling;
struct Component;
class CStackInstance;
class CGBlackMarket;
class CGObjectInstance;
struct Bonus;
class IMarket;
struct SetObjectProperty;
struct PackageApplied;
struct BattleAction;
struct BattleStackAttacked;
struct BattleResult;
struct BattleSpellCast;
struct CatapultAttack;
struct BattleStacksRemoved;
class CStack;
class CCreatureSet;
struct BattleAttack;
struct SetStackEffect;


class DLL_EXPORT IBattleEventsReceiver
{
public:
///	 occurs AFTER every action taken by any stack or by the hero
	virtual void actionFinished(const BattleAction *action){};

///	 occurs BEFORE every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action){};

///	 called when stack is performing attack
	virtual void battleAttack(const BattleAttack *ba){}; 

///	 called when stack receives damage (after battleAttack())
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa){}; 

	virtual void battleEnd(const BattleResult *br){};
///	 called at the beginning of each turn before changes are applied;
	virtual void battleNewRoundFirst(int round){}; 

///	 called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleNewRound(int round){}; 

///	 called when stack moves between tiles; dest is vector of tiles traversed
	virtual void battleStackMoved(const CStack * stack, std::vector<THex> dest, int distance){}; 

///	 called each time after spell is cast
	virtual void battleSpellCast(const BattleSpellCast *sc){}; 

///	 called when a specific effect is set to stacks
	virtual void battleStacksEffectsSet(const SetStackEffect & sse){};

///	 called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side){}; 

///	 called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	virtual void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom){}; 

///	 not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	virtual void battleNewStackAppeared(const CStack * stack){}; 

///	 called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	virtual void battleObstaclesRemoved(const std::set<si32> & removedObstacles){}; 

///	 called when catapult makes an attack
	virtual void battleCatapultAttacked(const CatapultAttack & ca){}; 

///	 called when certain stack is completely removed from battlefield
	virtual void battleStacksRemoved(const BattleStacksRemoved & bsr){}; 

};

class DLL_EXPORT IGameEventsReceiver
{
public:
	virtual void buildChanged(const CGTownInstance *town, int buildingID, int what){}; /// what: 1 - built, 2 - demolished

	virtual void battleResultsApplied(){}; /// called when all effects of last battle are applied

	//garrison operations
	virtual void stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute){}; /// if absolute, change is the new count; otherwise count was modified by adding change
	virtual void stackChangedType(const StackLocation &location, const CCreature &newType){}; /// used eg. when upgrading creatures
	virtual void stacksErased(const StackLocation &location){}; /// stack removed from previously filled slot
	virtual void stacksSwapped(const StackLocation &loc1, const StackLocation &loc2){};
	virtual void newStackInserted(const StackLocation &location, const CStackInstance &stack){}; /// new stack inserted at given (previously empty position)
	virtual void stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count){}; /// moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
	//virtual void garrisonChanged(const CGObjectInstance * obj){};

	//artifacts operations
	virtual void artifactPut(const ArtifactLocation &al){};
	virtual void artifactRemoved(const ArtifactLocation &al){};
	virtual void artifactAssembled(const ArtifactLocation &al){};
	virtual void artifactDisassembled(const ArtifactLocation &al){};
	virtual void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst){};

	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start){};
	virtual void heroCreated(const CGHeroInstance*){};
	virtual void heroInGarrisonChange(const CGTownInstance *town){};
	virtual void heroMoved(const TryMoveHero & details){};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val){};
	virtual void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val){};
	virtual void heroManaPointsChanged(const CGHeroInstance * hero){} /// not called at the beginning of turn and after spell casts
	virtual void heroMovePointsChanged(const CGHeroInstance * hero){} /// not called at the beginning of turn and after movement
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void receivedResource(int type, int val){};
	virtual void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID){};
	virtual void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level){}
	virtual void showShipyardDialog(const IShipyard *obj){} /// obj may be town or shipyard; state: 0 - can buid, 1 - lack of resources, 2 - dest tile is blocked, 3 - no water

	virtual void showPuzzleMap(){};
	virtual void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor){};
	virtual void showTavernWindow(const CGObjectInstance *townOrTavern){};
	virtual void advmapSpellCast(const CGHeroInstance * caster, int spellID){}; /// called when a hero casts a spell
	virtual void tileHidden(const boost::unordered_set<int3, ShashInt3> &pos){};
	virtual void tileRevealed(const boost::unordered_set<int3, ShashInt3> &pos){};
	virtual void newObject(const CGObjectInstance * obj){}; /// eg. ship built in shipyard
	virtual void availableArtifactsChanged(const CGBlackMarket *bm = NULL){}; /// bm may be NULL, then artifacts are changed in the global pool (used by merchants in towns)
	virtual void centerView (int3 pos, int focusTime){};
	virtual void availableCreaturesChanged(const CGDwelling *town){};
	virtual void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain){};/// if gain hero received bonus, else he lost it
	virtual void playerBonusChanged(const Bonus &bonus, bool gain){};/// if gain hero received bonus, else he lost it
	virtual void requestRealized(PackageApplied *pa){};
	virtual void heroExchangeStarted(si32 hero1, si32 hero2){};
	virtual void objectPropertyChanged(const SetObjectProperty * sop){}; /// eg. mine has been flagged
	virtual void objectRemoved(const CGObjectInstance *obj){}; /// eg. collected resource, picked artifact, beaten hero
	virtual void playerBlocked(int reason){}; /// reason: 0 - upcoming battle
	virtual void gameOver(ui8 player, bool victory){}; /// player lost or won the game
	virtual void playerStartsTurn(ui8 player){};
};