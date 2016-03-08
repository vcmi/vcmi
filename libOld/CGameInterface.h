#pragma once


#include "BattleAction.h"
#include "IGameEventsReceiver.h"
#include "CGameStateFwd.h"

#include "spells/ViewSpellInt.h"

#include "mapObjects/CObjectHandler.h"

/*
 * CGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using boost::logic::tribool;
class CCallback;
class CBattleCallback;
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
class CCommanderInstance;
class CStack;
class CPathsInfo;
class CCreature;
class CLoadFile;
class CSaveFile;
class CISer;
class COSer;
struct ArtifactLocation;
class CScriptingModule;

class DLL_LINKAGE CBattleGameInterface : public IBattleEventsReceiver
{
public:
	bool human;
	PlayerColor playerID;
	std::string dllName;

	virtual ~CBattleGameInterface() {};
	virtual void init(std::shared_ptr<CBattleCallback> CB){};

	//battle call-ins
	virtual BattleAction activeStack(const CStack * stack)=0; //called when it's turn of that stack
	virtual void yourTacticPhase(int distance){}; //called when interface has opportunity to use Tactics skill -> use cb->battleMakeTacticAction from this function

	virtual void saveGame(COSer &h, const int version);
	virtual void loadGame(CISer &h, const int version);

};

/// Central class for managing human player / AI interface logic
class DLL_LINKAGE CGameInterface : public CBattleGameInterface, public IGameEventsReceiver
{
public:
	virtual void init(std::shared_ptr<CCallback> CB){};
	virtual void yourTurn(){}; //called AFTER playerStartsTurn(player)

	//pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)=0;
	virtual	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)=0;

	// Show a dialog, player must take decision. If selection then he has to choose between one of given components,
	// if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called
	// with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel) = 0;

	// all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) = 0;
	virtual void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) = 0;
	virtual void finish(){}; //if for some reason we want to end
	
	virtual void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions){};
};

class DLL_LINKAGE CDynLibHandler
{
public:
	static std::shared_ptr<CGlobalAI> getNewAI(std::string dllname);
	static std::shared_ptr<CBattleGameInterface> getNewBattleAI(std::string dllname);
	static std::shared_ptr<CScriptingModule> getNewScriptingModule(std::string dllname);
};

class DLL_LINKAGE CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	CGlobalAI();
	virtual BattleAction activeStack(const CStack * stack) override;
};

//class to  be inherited by adventure-only AIs, it cedes battle actions to given battle-AI
class DLL_LINKAGE CAdventureAI : public CGlobalAI
{
public:
	CAdventureAI() {};

	std::shared_ptr<CBattleGameInterface> battleAI;
	std::shared_ptr<CBattleCallback> cbc;

	virtual std::string getBattleAIName() const = 0; //has to return name of the battle AI to be used

	//battle interface
	virtual BattleAction activeStack(const CStack * stack) override;
	virtual void yourTacticPhase(int distance) override;
	virtual void battleNewRound(int round) override;
	virtual void battleCatapultAttacked(const CatapultAttack & ca) override;
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) override;
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) override;
	virtual void actionStarted(const BattleAction &action) override;
	virtual void battleNewRoundFirst(int round) override;
	virtual void actionFinished(const BattleAction &action) override;
	virtual void battleStacksEffectsSet(const SetStackEffect & sse) override;
	//virtual void battleTriggerEffect(const BattleTriggerEffect & bte);
	virtual void battleStacksRemoved(const BattleStacksRemoved & bsr) override;
	virtual void battleObstaclesRemoved(const std::set<si32> & removedObstacles) override;
	virtual void battleNewStackAppeared(const CStack * stack) override;
	virtual void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) override;
	virtual void battleAttack(const BattleAttack *ba) override;
	virtual void battleSpellCast(const BattleSpellCast *sc) override;
	virtual void battleEnd(const BattleResult *br) override;
	virtual void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) override;

	virtual void saveGame(COSer & h, const int version) override; //saving
	virtual void loadGame(CISer & h, const int version) override; //loading
};
