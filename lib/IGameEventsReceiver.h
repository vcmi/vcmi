/*
 * IGameEventsReceiver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once


#include "NetPacksBase.h"
#include "battle/BattleHex.h"
#include "GameConstants.h"
#include "int3.h"

class CCallback;

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;
class CCreature;
class CArmedInstance;
struct StackLocation;
struct TryMoveHero;
struct ArtifactLocation;
class CGHeroInstance;
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
class BattleAction;
struct BattleStackAttacked;
struct BattleResult;
struct BattleSpellCast;
struct CatapultAttack;
class CStack;
class CCreatureSet;
struct BattleAttack;
struct SetStackEffect;
struct BattleTriggerEffect;
struct CObstacleInstance;
struct CPackForServer;
class EVictoryLossCheckResult;
class MetaString;
class ObstacleChanges;
class UnitChanges;

class DLL_LINKAGE IBattleEventsReceiver
{
public:
	virtual void actionFinished(const BattleAction &action){};//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction &action){};//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(const BattleAttack *ba){}; //called when stack is performing attack
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa, bool ranged){}; //called when stack receives damage (after battleAttack())
	virtual void battleEnd(const BattleResult *br, QueryID queryID){};
	virtual void battleNewRoundFirst(int round){}; //called at the beginning of each turn before changes are applied;
	virtual void battleNewRound(int round){}; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleLogMessage(const std::vector<MetaString> & lines){};
	virtual void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport){};
	virtual void battleSpellCast(const BattleSpellCast *sc){};
	virtual void battleStacksEffectsSet(const SetStackEffect & sse){};//called when a specific effect is set to stacks
	virtual void battleTriggerEffect(const BattleTriggerEffect & bte){}; //called for various one-shot effects
	virtual void battleStartBefore(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2) {}; //called just before battle start
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side){}; //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battleUnitsChanged(const std::vector<UnitChanges> & units){};
	virtual void battleObstaclesChanged(const std::vector<ObstacleChanges> & obstacles){};
	virtual void battleCatapultAttacked(const CatapultAttack & ca){}; //called when catapult makes an attack
	virtual void battleGateStateChanged(const EGateState state){};
};

class DLL_LINKAGE IGameEventsReceiver
{
public:
	virtual void buildChanged(const CGTownInstance *town, BuildingID buildingID, int what){}; //what: 1 - built, 2 - demolished

	virtual void battleResultsApplied(){}; //called when all effects of last battle are applied

	virtual void garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2){};

	//artifacts operations
	virtual void artifactPut(const ArtifactLocation &al){};
	virtual void artifactRemoved(const ArtifactLocation &al){};
	virtual void artifactAssembled(const ArtifactLocation &al){};
	virtual void artifactDisassembled(const ArtifactLocation &al){};
	virtual void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst){};
	virtual void bulkArtMovementStart(size_t numOfArts) {};
	virtual void askToAssembleArtifact(const ArtifactLocation & dst) {};

	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start){};
	virtual void heroCreated(const CGHeroInstance*){};
	virtual void heroInGarrisonChange(const CGTownInstance *town){};
	virtual void heroMoved(const TryMoveHero & details, bool verbose = true){};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val){};
	virtual void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val){};
	virtual void heroManaPointsChanged(const CGHeroInstance * hero){} //not called at the beginning of turn and after spell casts
	virtual void heroMovePointsChanged(const CGHeroInstance * hero){} //not called at the beginning of turn and after movement
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void receivedResource(){};
	virtual void showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID){};
	virtual void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level){}
	virtual void showShipyardDialog(const IShipyard *obj){} //obj may be town or shipyard; state: 0 - can buid, 1 - lack of resources, 2 - dest tile is blocked, 3 - no water

	virtual void showPuzzleMap(){};
	virtual void viewWorldMap(){};
	virtual void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor){};
	virtual void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor){};
	virtual void showTavernWindow(const CGObjectInstance *townOrTavern){};
	virtual void showThievesGuildWindow (const CGObjectInstance * obj){};
	virtual void showQuestLog(){};
	virtual void advmapSpellCast(const CGHeroInstance * caster, int spellID){}; //called when a hero casts a spell
	virtual void tileHidden(const std::unordered_set<int3> &pos){};
	virtual void tileRevealed(const std::unordered_set<int3> &pos){};
	virtual void newObject(const CGObjectInstance * obj){}; //eg. ship built in shipyard
	virtual void availableArtifactsChanged(const CGBlackMarket *bm = nullptr){}; //bm may be nullptr, then artifacts are changed in the global pool (used by merchants in towns)
	virtual void centerView (int3 pos, int focusTime){};
	virtual void availableCreaturesChanged(const CGDwelling *town){};
	virtual void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain){};//if gain hero received bonus, else he lost it
	virtual void playerBonusChanged(const Bonus &bonus, bool gain){};//if gain hero received bonus, else he lost it
	virtual void requestSent(const CPackForServer *pack, int requestID){};
	virtual void requestRealized(PackageApplied *pa){};
	virtual void beforeObjectPropertyChanged(const SetObjectProperty * sop){}; //eg. mine has been flagged
	virtual void objectPropertyChanged(const SetObjectProperty * sop){}; //eg. mine has been flagged
	virtual void objectRemoved(const CGObjectInstance *obj){}; //eg. collected resource, picked artifact, beaten hero
	virtual void objectRemovedAfter(){}; //eg. collected resource, picked artifact, beaten hero
	virtual void playerBlocked(int reason, bool start){}; //reason: 0 - upcoming battle
	virtual void gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult) {}; //player lost or won the game
	virtual void playerStartsTurn(PlayerColor player){};

	//TODO shouldn't be moved down the tree?
	virtual void heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID){};
};

VCMI_LIB_NAMESPACE_END
