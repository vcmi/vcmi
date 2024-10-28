/*
 * CPlayerInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ArtifactsUIController.h"

#include "../lib/FunctionList.h"
#include "../lib/CGameInterface.h"
#include "gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class Artifact;
struct TryMoveHero;
class CStack;
class CCreature;
struct CGPath;
class CCreatureSet;
class CGObjectInstance;
struct UpgradeInfo;
class ConditionalWait;
struct CPathsInfo;

VCMI_LIB_NAMESPACE_END

class CButton;
class AdventureMapInterface;
class CCastleInterface;
class BattleInterface;
class CComponent;
class CSelectableComponent;
class CSlider;
class CInGameConsole;
class CInfoWindow;
class IShowActivatable;
class ClickableL;
class ClickableR;
class Hoverable;
class KeyInterested;
class MotionInterested;
class PlayerLocalState;
class TimeInterested;
class HeroMovementController;

namespace boost
{
	class mutex;
	class recursive_mutex;
}

/// Central class for managing user interface logic
class CPlayerInterface : public CGameInterface, public IUpdateable
{
	bool ignoreEvents;
	int autosaveCount;

	std::list<std::shared_ptr<CInfoWindow>> dialogs; //queue of dialogs awaiting to be shown (not currently shown!)

	std::unique_ptr<HeroMovementController> movementController;
public: // TODO: make private
	std::unique_ptr<ArtifactsUIController> artifactController;
	std::shared_ptr<Environment> env;

	std::unique_ptr<PlayerLocalState> localState;

	//minor interfaces
	ConditionalWait * showingDialog; //indicates if dialog box is displayed

	bool makingTurn; //if player is already making his turn

	CCastleInterface * castleInt; //nullptr if castle window isn't opened
	static std::shared_ptr<BattleInterface> battleInt; //nullptr if no battle
	CInGameConsole * cingconsole;

	std::shared_ptr<CCallback> cb; //to communicate with engine

	//During battle is quick combat mode is used
	std::shared_ptr<CBattleGameInterface> autofightingAI; //AI that makes decisions
	bool isAutoFightOn; //Flag, switch it to stop quick combat. Don't touch if there is no battle interface.
	bool isAutoFightEndBattle; //Flag, if battle forced to end with autocombat

protected: // Call-ins from server, should not be called directly, but only via GameInterface

	void update() override;
	void initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB) override;

	void garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2) override;
	void buildChanged(const CGTownInstance *town, BuildingID buildingID, int what) override; //what: 1 - built, 2 - demolished

	void artifactPut(const ArtifactLocation &al) override;
	void artifactRemoved(const ArtifactLocation &al) override;
	void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst) override;
	void bulkArtMovementStart(size_t totalNumOfArts, size_t possibleAssemblyNumOfArts) override;
	void artifactAssembled(const ArtifactLocation &al) override;
	void askToAssembleArtifact(const ArtifactLocation & dst) override;
	void artifactDisassembled(const ArtifactLocation &al) override;

	void heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start) override;
	void heroCreated(const CGHeroInstance* hero) override;
	void heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID) override;
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override;
	void heroInGarrisonChange(const CGTownInstance *town) override;
	void heroMoved(const TryMoveHero & details, bool verbose = true) override;
	void heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val) override;
	void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) override;
	void heroManaPointsChanged(const CGHeroInstance * hero) override;
	void heroMovePointsChanged(const CGHeroInstance * hero) override;
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town) override;
	void receivedResource() override;
	void showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID) override;
	void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level, QueryID queryID) override;
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept) override; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) override;
	void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) override;
	void showMarketWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID) override;
	void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor, QueryID queryID) override;
	void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor) override;
	void advmapSpellCast(const CGHeroInstance * caster, SpellID spellID) override; //called when a hero casts a spell
	void tileHidden(const std::unordered_set<int3> &pos) override; //called when given tiles become hidden under fog of war
	void tileRevealed(const std::unordered_set<int3> &pos) override; //called when fog of war disappears from given tiles
	void newObject(const CGObjectInstance * obj) override;
	void availableArtifactsChanged(const CGBlackMarket *bm = nullptr) override; //bm may be nullptr, then artifacts are changed in the global pool (used by merchants in towns)
	void yourTurn(QueryID queryID) override;
	void availableCreaturesChanged(const CGDwelling *town) override;
	void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain) override;//if gain hero received bonus, else he lost it
	void playerBonusChanged(const Bonus &bonus, bool gain) override;
	void requestRealized(PackageApplied *pa) override;
	void heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query) override;
	void centerView (int3 pos, int focusTime) override;
	void beforeObjectPropertyChanged(const SetObjectProperty * sop) override;
	void objectPropertyChanged(const SetObjectProperty * sop) override;
	void objectRemoved(const CGObjectInstance *obj, const PlayerColor & initiator) override;
	void objectRemovedAfter() override;
	void playerBlocked(int reason, bool start) override;
	void gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult) override;
	void playerStartsTurn(PlayerColor player) override; //called before yourTurn on active interface
	void playerEndsTurn(PlayerColor player) override;
	void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain) override;

	//for battles
	void actionFinished(const BattleID & battleID, const BattleAction& action) override;//occurs AFTER action taken by active stack or by the hero
	void actionStarted(const BattleID & battleID, const BattleAction& action) override;//occurs BEFORE action taken by active stack or by the hero
	void activeStack(const BattleID & battleID, const CStack * stack) override; //called when it's turn of that stack
	void battleAttack(const BattleID & battleID, const BattleAttack *ba) override; //stack performs attack
	void battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID) override; //end of battle
	void battleNewRoundFirst(const BattleID & battleID) override; //called at the beginning of each turn before changes are applied; used for HP regen handling
	void battleNewRound(const BattleID & battleID) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleLogMessage(const BattleID & battleID, const std::vector<MetaString> & lines) override;
	void battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport) override;
	void battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc) override;
	void battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse) override; //called when a specific effect is set to stacks
	void battleTriggerEffect(const BattleID & battleID, const BattleTriggerEffect & bte) override; //various one-shot effect
	void battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged) override;
	void battleStartBefore(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2) override; //called by engine just before battle starts; side=0 - left, side=1 - right
	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) override; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units) override;
	void battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles) override;
	void battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca) override; //called when catapult makes an attack
	void battleGateStateChanged(const BattleID & battleID, const EGateState state) override;
	void yourTacticPhase(const BattleID & battleID, int distance) override;
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

public: // public interface for use by client via LOCPLINT access

	// part of GameInterface that is also used by client code
	void showPuzzleMap() override;
	void viewWorldMap() override;
	void showQuestLog() override;
	void showThievesGuildWindow (const CGObjectInstance * obj) override;
	void showTavernWindow(const CGObjectInstance * object, const CGHeroInstance * visitor, QueryID queryID) override;
	void showShipyardDialog(const IShipyard *obj) override; //obj may be town or shipyard;

	void showHeroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2);
	void waitWhileDialog();
	void waitForAllDialogs();
	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero

	void showInfoDialog(const std::string &text, std::shared_ptr<CComponent> component);
	void showInfoDialog(const std::string &text, const std::vector<std::shared_ptr<CComponent>> & components = std::vector<std::shared_ptr<CComponent>>(), int soundID = 0);
	void showInfoDialogAndWait(std::vector<Component> & components, const MetaString & text);
	void showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, const std::vector<std::shared_ptr<CComponent>> & components = std::vector<std::shared_ptr<CComponent>>());

	void moveHero(const CGHeroInstance *h, const CGPath& path);

	void tryDigging(const CGHeroInstance *h);
	void showShipyardDialogOrProblemPopup(const IShipyard *obj); //obj may be town or shipyard;
	void proposeLoadingGame();
	void performAutosave();
	void gamePause(bool pause);
	void endNetwork();
	void closeAllDialogs();

	///returns true if all events are processed internally
	bool capturedAllEvents();

	void prepareAutoFightingAI(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side);

	CPlayerInterface(PlayerColor Player);
	~CPlayerInterface();

private:
	struct IgnoreEvents
	{
		CPlayerInterface & owner;
		IgnoreEvents(CPlayerInterface & Owner):owner(Owner)
		{
			owner.ignoreEvents = true;
		};
		~IgnoreEvents()
		{
			owner.ignoreEvents = false;
		};
	};

	void heroKilled(const CGHeroInstance* hero);
	void garrisonsChanged(std::vector<const CArmedInstance *> objs);
	void requestReturningToMainMenu(bool won);
	void acceptTurn(QueryID queryID, bool hotseatWait); //used during hot seat after your turn message is close
	void initializeHeroTownList();
	int getLastIndex(std::string namePrefix);
};

/// Provides global access to instance of interface of currently active player
extern CPlayerInterface * LOCPLINT;
