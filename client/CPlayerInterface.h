#pragma once


#include "../lib/FunctionList.h"
#include "../lib/CGameInterface.h"
#include "../lib/NetPacksBase.h"
#include "gui/CIntObject.h"

#ifdef __GNUC__
#define sprintf_s snprintf
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

/*
 * CPlayerInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CDefEssential;
class CButton;
class CToggleGroup;
class CDefHandler;
struct TryMoveHero;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
class CStack;
class CComponent;
class CCreature;
struct SDL_Surface;
struct CGPath;
class CCreatureAnimation;
class CSelectableComponent;
class CCreatureSet;
class CGObjectInstance;
class CSlider;
struct UpgradeInfo;
template <typename T> struct CondSh;
class CInGameConsole;
class CGarrisonInt;
class CInGameConsole;
union SDL_Event;
class CInfoWindow;
class IShowActivatable;
class ClickableL;
class ClickableR;
class Hoverable;
class KeyInterested;
class MotionInterested;
class TimeInterested;
class IShowable;
struct CPathsInfo;

namespace boost
{
	class mutex;
	class recursive_mutex;
}

enum
{
	/*CHANGE_SCREEN_RESOLUTION = 1,*/
	RETURN_TO_MAIN_MENU = 2,
	STOP_CLIENT = 3,
	RESTART_GAME,
	RETURN_TO_MENU_LOAD,
	FULLSCREEN_TOGGLED,
	PREPARE_RESTART_CAMPAIGN,
	FORCE_QUIT //quit client without question
};

/// Central class for managing user interface logic
class CPlayerInterface : public CGameInterface, public IUpdateable
{
	const CArmedInstance * currentSelection;
public:
	bool observerInDuelMode;
	ObjectInstanceID destinationTeleport; //contain -1 or object id if teleportation
	int3 destinationTeleportPos;

	//minor interfaces
	CondSh<bool> *showingDialog; //indicates if dialog box is displayed

	static boost::recursive_mutex *pim;
	bool makingTurn; //if player is already making his turn
	int firstCall; // -1 - just loaded game; 1 - just started game; 0 otherwise
	int autosaveCount;
	static const int SAVES_COUNT = 5;
	static int howManyPeople;

	CCastleInterface * castleInt; //nullptr if castle window isn't opened
	static CBattleInterface * battleInt; //nullptr if no battle
	CInGameConsole * cingconsole;

	std::shared_ptr<CCallback> cb; //to communicate with engine
	const BattleAction *curAction; //during the battle - action currently performed by active stack (or nullptr)

	std::list<CInfoWindow *> dialogs; //queue of dialogs awaiting to be shown (not currently shown!)

	std::vector<const CGHeroInstance *> wanderingHeroes; //our heroes on the adventure map (not the garrisoned ones)
	std::vector<const CGTownInstance *> towns; //our towns on the adventure map
	std::map<const CGHeroInstance *, CGPath> paths; //maps hero => selected path in adventure map
	std::vector<const CGHeroInstance *> sleepingHeroes; //if hero is in here, he's sleeping

	//During battle is quick combat mode is used
	std::shared_ptr<CBattleGameInterface> autofightingAI; //AI that makes decisions
	bool isAutoFightOn; //Flag, switch it to stop quick combat. Don't touch if there is no battle interface.

	const CArmedInstance * getSelection();
	void setSelection(const CArmedInstance * obj);

	struct SpellbookLastSetting
	{
		int spellbookLastPageBattle, spellbokLastPageAdvmap; //on which page we left spellbook
		int spellbookLastTabBattle, spellbookLastTabAdvmap; //on which page we left spellbook

		SpellbookLastSetting();
		template <typename Handler> void serialize( Handler &h, const int version )
		{
			h & spellbookLastPageBattle & spellbokLastPageAdvmap & spellbookLastTabBattle & spellbookLastTabAdvmap;
		}
	} spellbookSettings;

	void update() override;
	void initializeHeroTownList();
	int getLastIndex(std::string namePrefix);

	//overridden funcs from CGameInterface
	void buildChanged(const CGTownInstance *town, BuildingID buildingID, int what) override; //what: 1 - built, 2 - demolished
	void stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute) override; //if absolute, change is the new count; otherwise count was modified by adding change
	void stackChangedType(const StackLocation &location, const CCreature &newType) override; //used eg. when upgrading creatures
	void stacksErased(const StackLocation &location) override; //stack removed from previously filled slot
	void stacksSwapped(const StackLocation &loc1, const StackLocation &loc2) override;
	void newStackInserted(const StackLocation &location, const CStackInstance &stack) override; //new stack inserted at given (previously empty position)
	void stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count) override; //moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks

	void artifactPut(const ArtifactLocation &al) override;
	void artifactRemoved(const ArtifactLocation &al) override;
	void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst) override;
	void artifactAssembled(const ArtifactLocation &al) override;
	void artifactDisassembled(const ArtifactLocation &al) override;

	void heroCreated(const CGHeroInstance* hero) override;
	void heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID) override;
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override;
	void heroInGarrisonChange(const CGTownInstance *town) override;
	void heroMoved(const TryMoveHero & details) override;
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val) override;
	void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) override;
	void heroManaPointsChanged(const CGHeroInstance * hero) override;
	void heroMovePointsChanged(const CGHeroInstance * hero) override;
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town) override;
	void receivedResource(int type, int val) override;
	void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID) override;
	void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level) override;
	void showShipyardDialog(const IShipyard *obj) override; //obj may be town or shipyard;
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, int soundID, bool selection, bool cancel) override; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) override;
	void showPuzzleMap() override;
	void viewWorldMap() override;
	void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor) override;
	void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor) override;
	void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor) override;
	void showTavernWindow(const CGObjectInstance *townOrTavern) override;
	void showThievesGuildWindow (const CGObjectInstance * obj) override;
	void showQuestLog() override;
	void advmapSpellCast(const CGHeroInstance * caster, int spellID) override; //called when a hero casts a spell
	void tileHidden(const std::unordered_set<int3, ShashInt3> &pos) override; //called when given tiles become hidden under fog of war
	void tileRevealed(const std::unordered_set<int3, ShashInt3> &pos) override; //called when fog of war disappears from given tiles
	void newObject(const CGObjectInstance * obj) override;
	void availableArtifactsChanged(const CGBlackMarket *bm = nullptr) override; //bm may be nullptr, then artifacts are changed in the global pool (used by merchants in towns)
	void yourTurn() override;
	void availableCreaturesChanged(const CGDwelling *town) override;
	void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain) override;//if gain hero received bonus, else he lost it
	void playerBonusChanged(const Bonus &bonus, bool gain) override;
	void requestRealized(PackageApplied *pa) override;
	void heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query) override;
	void centerView (int3 pos, int focusTime) override;
	void objectPropertyChanged(const SetObjectProperty * sop) override;
	void objectRemoved(const CGObjectInstance *obj) override;
	void gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult) override;
	void playerStartsTurn(PlayerColor player) override; //called before yourTurn on active itnerface
	void showComp(const Component &comp, std::string message) override; //display component in the advmapint infobox
	void saveGame(COSer & h, const int version) override; //saving
	void loadGame(CISer & h, const int version) override; //loading
	void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions) override;

	//for battles
	void actionFinished(const BattleAction& action) override;//occurs AFTER action taken by active stack or by the hero
	void actionStarted(const BattleAction& action) override;//occurs BEFORE action taken by active stack or by the hero
	BattleAction activeStack(const CStack * stack) override; //called when it's turn of that stack
	void battleAttack(const BattleAttack *ba) override; //stack performs attack
	void battleEnd(const BattleResult *br) override; //end of battle
	void battleNewRoundFirst(int round) override; //called at the beginning of each turn before changes are applied; used for HP regen handling
	void battleNewRound(int round) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) override;
	void battleSpellCast(const BattleSpellCast *sc) override;
	void battleStacksEffectsSet(const SetStackEffect & sse) override; //called when a specific effect is set to stacks
	void battleTriggerEffect(const BattleTriggerEffect & bte) override; //various one-shot effect
	void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) override;
	void battleStartBefore(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2) override; //called by engine just before battle starts; side=0 - left, side=1 - right
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) override; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) override; //called when stacks are healed / resurrected
	void battleNewStackAppeared(const CStack * stack) override; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	void battleObstaclesRemoved(const std::set<si32> & removedObstacles) override; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	void battleCatapultAttacked(const CatapultAttack & ca) override; //called when catapult makes an attack
	void battleStacksRemoved(const BattleStacksRemoved & bsr) override; //called when certain stack is completely removed from battlefield
	void battleObstaclePlaced(const CObstacleInstance &obstacle) override;
	void yourTacticPhase(int distance) override;

	//-------------//
	void showArtifactAssemblyDialog(ui32 artifactID, ui32 assembleTo, bool assemble, CFunctionList<bool()> onYes, CFunctionList<bool()> onNo);
	void garrisonsChanged(std::vector<const CGObjectInstance *> objs);
	void garrisonChanged(const CGObjectInstance * obj);
	void heroKilled(const CGHeroInstance* hero);
	void waitWhileDialog(bool unlockPim = true);
	void waitForAllDialogs(bool unlockPim = true);
	bool shiftPressed() const; //determines if shift key is pressed (left or right or both)
	bool ctrlPressed() const; //determines if ctrl key is pressed (left or right or both)
	bool altPressed() const; //determines if alt key is pressed (left or right or both)
	void redrawHeroWin(const CGHeroInstance * hero);
	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero
	void updateInfo(const CGObjectInstance * specific);
	void init(std::shared_ptr<CCallback> CB) override;
	int3 repairScreenPos(int3 pos); //returns position closest to pos we can center screen on

	// show dialogs
	void showInfoDialog(const std::string &text, CComponent * component);
	void showInfoDialog(const std::string &text, const std::vector<CComponent*> & components = std::vector<CComponent*>(), int soundID = 0, bool delComps = false);
	void showInfoDialogAndWait(std::vector<Component> & components, const MetaString & text);
	void showOkDialog(std::vector<Component> & components, const MetaString & text, const std::function<void()> & onOk);
	void showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps = false, const std::vector<CComponent*> & components = std::vector<CComponent*>()); //deactivateCur - whether current main interface should be deactivated; delComps - if components will be deleted on window close

	void stopMovement();
	void moveHero(const CGHeroInstance *h, CGPath path);
	void initMovement(const TryMoveHero &details, const CGHeroInstance * ho, const int3 &hp );//initializing objects and performing first step of move
	void movementPxStep( const TryMoveHero &details, int i, const int3 &hp, const CGHeroInstance * ho );//performing step of movement
	void finishMovement( const TryMoveHero &details, const int3 &hp, const CGHeroInstance * ho ); //finish movement
	void eraseCurrentPathOf( const CGHeroInstance * ho, bool checkForExistanceOfPath = true );

	void removeLastNodeFromPath(const CGHeroInstance *ho);
	CGPath *getAndVerifyPath( const CGHeroInstance * h );
	void acceptTurn(); //used during hot seat after your turn message is close
	void tryDiggging(const CGHeroInstance *h);
	void showShipyardDialogOrProblemPopup(const IShipyard *obj); //obj may be town or shipyard;
	void requestReturningToMainMenu();
	void requestStoppingClient();
	void sendCustomEvent(int code);
	void proposeLoadingGame();

	///returns true if all events are processed internally
	bool capturedAllEvents();

	CPlayerInterface(PlayerColor Player);//c-tor
	~CPlayerInterface();//d-tor

private:

	template <typename Handler> void serializeTempl(Handler &h, const int version);

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



	bool duringMovement;
	bool ignoreEvents;

	void doMoveHero(const CGHeroInstance *h, CGPath path);
	void setMovementStatus(bool value);
	void askToAssembleArtifact(const ArtifactLocation &al);
};

extern CPlayerInterface * LOCPLINT;
