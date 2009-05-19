#ifndef __CPLAYERINTERFACE_H__
#define __CPLAYERINTERFACE_H__
#include "global.h"
#include "CGameInterface.h"
#include "SDL_framerate.h"
#include <map>
#include <list>
#include <algorithm>

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
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
class CStack;
class SComponent;
class CCreature;
struct SDL_Surface;
struct CPath;
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

namespace boost
{
	class mutex;
	class recursive_mutex;
};

class CPlayerInterface : public CGameInterface
{
public:
	//minor interfaces
	CondSh<bool> *showingDialog; //indicates if dialog box is displayed

	boost::recursive_mutex *pim;
	bool makingTurn; //if player is already making his turn

	//TODO: exclude to some kind Settings struct
	int heroMoveSpeed; //speed of player's hero movement
	void setHeroMoveSpeed(int newSpeed) {heroMoveSpeed = newSpeed;} //set for the member above
	int mapScrollingSpeed; //map scrolling speed
	void setMapScrollingSpeed(int newSpeed) {mapScrollingSpeed = newSpeed;} //set the member above

	SDL_Event * current; //current event
	//CMainInterface *curint;
	CAdvMapInt * adventureInt;
	CCastleInterface * castleInt; //NULL if castle window isn't opened
	CBattleInterface * battleInt; //NULL if no battle
	CInGameConsole * cingconsole;
	FPSmanager * mainFPSmng; //to keep const framerate
	IStatusBar *statusbar; //current statusbar - will be used to show hover tooltips
	
	CCallback * cb; //to communicate with engine
	const BattleAction *curAction; //during the battle - action currently performed by active stack (or NULL)

	std::list<CInfoWindow *> dialogs; //queue of dialogs awaiting to be shown (not currently shown!)
	std::list<IShowActivable *> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	void totalRedraw(); //forces total redraw (using showAll)
	void popInt(IShowActivable *top); //removes given interface from the top and activates next
	void popIntTotally(IShowActivable *top); //deactivates, deletes, removes given interface from the top and activates next
	void pushInt(IShowActivable *newInt); //deactivate old top interface, activates this one and pushes to the top
	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	IShowActivable *topInt(); //returns top interface

	//GUI elements
	std::list<ClickableL*> lclickable;
	std::list<ClickableR*> rclickable;
	std::list<Hoverable*> hoverable;
	std::list<KeyInterested*> keyinterested;
	std::list<MotionInterested*> motioninterested;
	std::list<TimeInterested*> timeinterested;
	std::vector<IShowable*> objsToBlit;

	//overloaded funcs from CGameInterface
	void buildChanged(const CGTownInstance *town, int buildingID, int what); //what: 1 - built, 2 - demolished
	void garrisonChanged(const CGObjectInstance * obj);
	void heroArtifactSetChanged(const CGHeroInstance* hero);
	void heroCreated(const CGHeroInstance* hero);
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	void heroInGarrisonChange(const CGTownInstance *town);
	void heroKilled(const CGHeroInstance* hero);
	void heroMoved(const HeroMoveDetails & details);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void heroManaPointsChanged(const CGHeroInstance * hero);
	void heroMovePointsChanged(const CGHeroInstance * hero);
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town);
	void receivedResource(int type, int val);
	void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID);
	//void showSelDialog(const std::string &text, const std::vector<Component*> &components, ui32 askID);
	//void showYesNoDialog(const std::string &text, const std::vector<Component*> &components, ui32 askID);
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd);
	void tileHidden(const std::set<int3> &pos); //called when given tiles become hidden under fog of war
	void tileRevealed(const std::set<int3> &pos); //called when fog of war disappears from given tiles
	void yourTurn();
	void availableCreaturesChanged(const CGTownInstance *town);
	void heroBonusChanged(const CGHeroInstance *hero, const HeroBonus &bonus, bool gain);//if gain hero received bonus, else he lost it
	void requestRealized(PackageApplied *pa);
	void serialize(COSer<CSaveFile> &h, const int version); //saving
	void serialize(CISer<CLoadFile> &h, const int version); //loading

	//for battles
	void actionFinished(const BattleAction* action);//occurs AFTER action taken by active stack or by the hero
	void actionStarted(const BattleAction* action);//occurs BEFORE action taken by active stack or by the hero
	BattleAction activeStack(int stackID); //called when it's turn of that stack
	void battleAttack(BattleAttack *ba); //stack performs attack
	void battleEnd(BattleResult *br); //end of battle
	//void battleResultQuited();
	void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(int ID, int dest, int distance, bool end);
	void battleSpellCast(SpellCast *sc);
	void battleStacksEffectsSet(SetStackEffect & sse); //called when a specific effect is set to stacks
	void battleStacksAttacked(std::set<BattleStackAttacked> & bsa);
	void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning


	//-------------//
	bool shiftPressed() const; //determines if shift key is pressed (left or right or both)
	void redrawHeroWin(const CGHeroInstance * hero);
	void updateWater();
	void showComp(SComponent comp); //TODO: comment me
	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero
	SDL_Surface * infoWin(const CGObjectInstance * specific); //specific=0 => draws info about selected town/hero
	void handleEvent(SDL_Event * sEvent);
	void handleKeyDown(SDL_Event *sEvent);
	void handleKeyUp(SDL_Event *sEvent);
	void handleMouseMotion(SDL_Event *sEvent);
	void init(ICallback * CB);
	int3 repairScreenPos(int3 pos); //returns position closest to pos we can center screen on
	void showInfoDialog(const std::string &text, const std::vector<SComponent*> & components, int soundID);
	void showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps); //deactivateCur - whether current main interface should be deactivated; delComps - if components will be deleted on window close
	bool moveHero(const CGHeroInstance *h, CPath path);

	CPlayerInterface(int Player, int serial);//c-tor
	~CPlayerInterface();//d-tor

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serializeTempl(Handler &h, const int version);
};

extern CPlayerInterface * LOCPLINT;


#endif // __CPLAYERINTERFACE_H__
