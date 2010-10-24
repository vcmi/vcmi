#ifndef __CPLAYERINTERFACE_H__
#define __CPLAYERINTERFACE_H__
#include "../global.h"
#include "../CGameInterface.h"
#include "../lib/CondSh.h"
#include "SDL_framerate.h"
#include <map>
#include <list>
#include <algorithm>
#include "GUIBase.h"

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
class AdventureMapButton;
class CHighlightableButtonsGroup;
class CDefHandler;
struct TryMoveHero;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
class CStack;
class SComponent;
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
class IStatusBar;
class CInfoWindow;
class IShowActivable;
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
};

struct SystemOptions
{
	std::string playerName;

	ui8 heroMoveSpeed;/*, enemyMoveSpeed*/ //speed of player's hero movement
	ui8 mapScrollingSpeed; //map scrolling speed
	ui8 musicVolume, soundVolume;
	//TODO: rest of system options

	//battle settings
	ui8 printCellBorders; //if true, cell borders will be printed
	ui8 printStackRange; //if true,range of active stack will be printed
	ui8 animSpeed; //speed of animation; 1 - slowest, 2 - medium, 4 - fastest
	ui8 printMouseShadow; //if true, hex under mouse will be shaded
	ui8 showQueue;

	SystemOptions();
	void setHeroMoveSpeed(int newSpeed); //set for the member above
	void setMapScrollingSpeed(int newSpeed); //set the member above
	void setMusicVolume(int newVolume);
	void setSoundVolume(int newVolume);
	void setPlayerName(const std::string &newPlayerName);
	void settingsChanged(); //updates file with "default" settings for next running of application
	void apply();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName;
		h & heroMoveSpeed & mapScrollingSpeed & musicVolume & soundVolume;
		h & printCellBorders & printStackRange & animSpeed & printMouseShadow & showQueue;
	}
};

extern SystemOptions GDefaultOptions; //defined and inited in CMT.cpp, stores default settings loaded with application

class CPlayerInterface : public CGameInterface, public IUpdateable
{
public:
	//minor interfaces
	CondSh<bool> *showingDialog; //indicates if dialog box is displayed

	boost::recursive_mutex *pim;
	bool makingTurn; //if player is already making his turn
	int firstCall; // -1 - just loaded game; 1 - just started game; 0 otherwise
	int autosaveCount;
	static const int SAVES_COUNT = 5;
	static int howManyPeople;

	SystemOptions sysOpts;

	CCastleInterface * castleInt; //NULL if castle window isn't opened
	static CBattleInterface * battleInt; //NULL if no battle
	FPSmanager * mainFPSmng; //to keep const framerate
	CInGameConsole * cingconsole;
	
	CCallback * cb; //to communicate with engine
	const BattleAction *curAction; //during the battle - action currently performed by active stack (or NULL)

	std::list<CInfoWindow *> dialogs; //queue of dialogs awaiting to be shown (not currently shown!)


	std::vector<const CGHeroInstance *> wanderingHeroes; //our heroes on the adventure map (not the garrisoned ones)
	std::vector<const CGTownInstance *> towns; //our heroes on the adventure map (not the garrisoned ones)
	std::map<const CGHeroInstance *, CGPath> paths; //maps hero => selected path in adventure map

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

	void update();
	void recreateHeroTownList();
	const CGHeroInstance *getWHero(int pos); //returns NULL if position is not valid
	int getLastIndex(std::string namePrefix);

	//overloaded funcs from CGameInterface
	void buildChanged(const CGTownInstance *town, int buildingID, int what); //what: 1 - built, 2 - demolished
	void garrisonChanged(const CGObjectInstance * obj);
	void heroArtifactSetChanged(const CGHeroInstance* hero);
	void heroCreated(const CGHeroInstance* hero);
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	void heroInGarrisonChange(const CGTownInstance *town);
	void heroMoved(const TryMoveHero & details);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val);
	void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void heroManaPointsChanged(const CGHeroInstance * hero);
	void heroMovePointsChanged(const CGHeroInstance * hero);
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town);
	void receivedResource(int type, int val);
	void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID);
	void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level);
	void showShipyardDialog(const IShipyard *obj); //obj may be town or shipyard; 
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd);
	void showArtifactAssemblyDialog(ui32 artifactID, ui32 assembleTo, bool assemble, CFunctionList<void()> onYes, CFunctionList<void()> onNo);
	void showPuzzleMap();
	void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor);
	void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor);
	void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor);
	void showTavernWindow(const CGObjectInstance *townOrTavern);
	void advmapSpellCast(const CGHeroInstance * caster, int spellID); //called when a hero casts a spell
	void tileHidden(const std::set<int3> &pos); //called when given tiles become hidden under fog of war
	void tileRevealed(const std::set<int3> &pos); //called when fog of war disappears from given tiles
	void newObject(const CGObjectInstance * obj);
	void availableArtifactsChanged(const CGBlackMarket *bm = NULL); //bm may be NULL, then artifacts are changed in the global pool (used by merchants in towns)
	void yourTurn();
	void availableCreaturesChanged(const CGDwelling *town);
	void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain);//if gain hero received bonus, else he lost it
	void playerBonusChanged(const Bonus &bonus, bool gain);
	void requestRealized(PackageApplied *pa);
	void heroExchangeStarted(si32 hero1, si32 hero2);
	void centerView (int3 pos, int focusTime);
	void objectPropertyChanged(const SetObjectProperty * sop);
	void objectRemoved(const CGObjectInstance *obj);
	void gameOver(ui8 player, bool victory);
	void serialize(COSer<CSaveFile> &h, const int version); //saving
	void serialize(CISer<CLoadFile> &h, const int version); //loading

	//for battles
	void actionFinished(const BattleAction* action);//occurs AFTER action taken by active stack or by the hero
	void actionStarted(const BattleAction* action);//occurs BEFORE action taken by active stack or by the hero
	BattleAction activeStack(int stackID); //called when it's turn of that stack
	void battleAttack(BattleAttack *ba); //stack performs attack
	void battleEnd(BattleResult *br); //end of battle
	//void battleResultQuited();
	void battleNewRoundFirst(int round); //called at the beginning of each turn before changes are applied; used for HP regen handling
	void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(int ID, int dest, int distance, bool end);
	void battleSpellCast(BattleSpellCast *sc);
	void battleStacksEffectsSet(SetStackEffect & sse); //called when a specific effect is set to stacks
	void battleStacksAttacked(std::vector<BattleStackAttacked> & bsa);
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, si32 lifeDrainFrom); //called when stacks are healed / resurrected
	void battleNewStackAppeared(int stackID); //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	void battleObstaclesRemoved(const std::set<si32> & removedObstacles); //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	void battleCatapultAttacked(const CatapultAttack & ca); //called when catapult makes an attack
	void battleStacksRemoved(const BattleStacksRemoved & bsr); //called when certain stack is completely removed from battlefield

	//-------------//
	void heroKilled(const CGHeroInstance* hero);
	void waitWhileDialog();
	bool shiftPressed() const; //determines if shift key is pressed (left or right or both)
	bool ctrlPressed() const; //determines if ctrl key is pressed (left or right or both)
	bool altPressed() const; //determines if alt key is pressed (left or right or both)
	void redrawHeroWin(const CGHeroInstance * hero);
	void showComp(SComponent comp); //TODO: comment me
	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero
	SDL_Surface * infoWin(const CGObjectInstance * specific); //specific=0 => draws info about selected town/hero
	void updateInfo(const CGObjectInstance * specific);
	void init(ICallback * CB);
	int3 repairScreenPos(int3 pos); //returns position closest to pos we can center screen on
	void showInfoDialog(const std::string &text, const std::vector<SComponent*> & components = std::vector<SComponent*>(), int soundID = 0, bool delComps = false);
	void showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps); //deactivateCur - whether current main interface should be deactivated; delComps - if components will be deleted on window close
	void stopMovement();
	bool moveHero(const CGHeroInstance *h, CGPath path);
	void initMovement(const TryMoveHero &details, const CGHeroInstance * ho, const int3 &hp );//initializing objects and performing first step of move
	void movementPxStep( const TryMoveHero &details, int i, const int3 &hp, const CGHeroInstance * ho );//performing step of movement
	void finishMovement( const TryMoveHero &details, const int3 &hp, const CGHeroInstance * ho ); //finish movement
	void eraseCurrentPathOf( const CGHeroInstance * ho, bool checkForExistanceOfPath = true );
	CGPath *getAndVerifyPath( const CGHeroInstance * h );
	void acceptTurn(); //used during hot seat after your turn message is close
	void tryDiggging(const CGHeroInstance *h);
	void showShipyardDialogOrProblemPopup(const IShipyard *obj); //obj may be town or shipyard; 
	void requestReturningToMainMenu();
	void requestStoppingClient();
	void sendCustomEvent(int code);

	CPlayerInterface(int Player);//c-tor
	~CPlayerInterface();//d-tor

	CondSh<bool> terminate_cond; // confirm termination

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serializeTempl(Handler &h, const int version);
};

extern CPlayerInterface * LOCPLINT;


#endif // __CPLAYERINTERFACE_H__
