/*
 * CGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "battle/AutocombatPreferences.h"
#include "IGameEventsReceiver.h"

#include "spells/ViewSpellInt.h"

class CBattleCallback;
class CCallback;

VCMI_LIB_NAMESPACE_BEGIN

using boost::logic::tribool;

class Environment;

class ICallback;
class CGlobalAI;
struct Component;
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
class BattleAction;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
struct BattleSpellCast;
struct SetStackEffect;
struct Bonus;
struct PackageApplied;
struct SetObjectProperty;
struct CatapultAttack;
struct StackLocation;
class CStackInstance;
class CCommanderInstance;
class CStack;
class CCreature;
class CLoadFile;
class CSaveFile;
class BattleStateInfo;
struct ArtifactLocation;
class BattleStateInfoForRetreat;

#if SCRIPTING_ENABLED
namespace scripting
{
	class Module;
}
#endif

using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

class DLL_LINKAGE CBattleGameInterface : public IBattleEventsReceiver
{
public:
	bool human = false;
	PlayerColor playerID;
	std::string dllName;

	virtual ~CBattleGameInterface() {};
	virtual void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB){};
	virtual void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences){};

	//battle call-ins
	virtual void activeStack(const BattleID & battleID, const CStack * stack)=0; //called when it's turn of that stack
	virtual void yourTacticPhase(const BattleID & battleID, int distance)=0; //called when interface has opportunity to use Tactics skill -> use cb->battleMakeTacticAction from this function
};

/// Central class for managing human player / AI interface logic
class DLL_LINKAGE CGameInterface : public CBattleGameInterface, public IGameEventsReceiver
{
public:
	virtual ~CGameInterface() = default;
	virtual void initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB){};
	virtual void yourTurn(QueryID askID){}; //called AFTER playerStartsTurn(player)

	//pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)=0;
	virtual void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)=0;

	// Show a dialog, player must take decision. If selection then he has to choose between one of given components,
	// if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called
	// with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept) = 0;

	// all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) = 0;
	virtual void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) = 0;
	virtual void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) = 0;
	virtual void finish(){}; //if for some reason we want to end

	virtual void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain){};

	virtual std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) = 0;
};

class DLL_LINKAGE CDynLibHandler
{
public:
	static std::shared_ptr<CGlobalAI> getNewAI(const std::string & dllname);
	static std::shared_ptr<CBattleGameInterface> getNewBattleAI(const std::string & dllname);
#if SCRIPTING_ENABLED
	static std::shared_ptr<scripting::Module> getNewScriptingModule(const boost::filesystem::path & dllname);
#endif
};

class DLL_LINKAGE CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	std::shared_ptr<Environment> env;
	CGlobalAI();
};

//class to  be inherited by adventure-only AIs, it cedes battle actions to given battle-AI
class DLL_LINKAGE CAdventureAI : public CGlobalAI
{
public:
	CAdventureAI() = default;

	std::shared_ptr<CBattleGameInterface> battleAI;
	std::shared_ptr<CBattleCallback> cbc;

	virtual std::string getBattleAIName() const = 0; //has to return name of the battle AI to be used

	//battle interface
	void activeStack(const BattleID & battleID, const CStack * stack) override;
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	void battleNewRound(const BattleID & battleID) override;
	void battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca) override;
	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) override;
	void battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged) override;
	void actionStarted(const BattleID & battleID, const BattleAction &action) override;
	void battleNewRoundFirst(const BattleID & battleID) override;
	void actionFinished(const BattleID & battleID, const BattleAction &action) override;
	void battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse) override;
	void battleTriggerEffect(const BattleID & battleID, const BattleTriggerEffect & bte) override;
	void battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles) override;
	void battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport) override;
	void battleAttack(const BattleID & battleID, const BattleAttack *ba) override;
	void battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc) override;
	void battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID) override;
	void battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units) override;
};

VCMI_LIB_NAMESPACE_END
