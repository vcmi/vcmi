#ifndef __GUICLASSES_H__
#define __GUICLASSES_H__

#include "../global.h"
#include "SDL_framerate.h"
#include "GUIBase.h"
#include "FunctionList.h"
#include <set>
#include <list>
#include <boost/thread/mutex.hpp>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

/*
 * GUIClasses.h, part of VCMI engine
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
class CGDwelling;
class CSlider;
struct UpgradeInfo;
template <typename T> struct CondSh;
class CInGameConsole;
class CGarrisonInt;
class CInGameConsole;
class Component;
class CArmedInstance;
class CGTownInstance;
class StackState;
class CPlayerInterface;
class CHeroWindow;
class CArtifact;
class CArtifactsOfHero;
class CResDataBar;
struct SPuzzleInfo;

class CInfoWindow : public CSimpleWindow //text + comp. + ok button
{ //window able to delete its components when closed
public:
	bool delComps; //whether comps will be deleted
	std::vector<AdventureMapButton *> buttons;
	std::vector<SComponent*> components;
	CSlider *slider;
	virtual void close();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void activate();
	void sliderMoved(int to);
	void deactivate();
	CInfoWindow(std::string text, int player, int charperline, const std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, bool delComps); //c-tor
	CInfoWindow(); //c-tor
	~CInfoWindow(); //d-tor

	static void showYesNoDialog( const std::string & text, const std::vector<SComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, int player); //use only before the game starts! (showYesNoDialog in LOCPLINT must be used then)
};
class CSelWindow : public CInfoWindow //component selection window
{ //warning - this window deletes its components by closing!
public:
	void selectionChange(unsigned to);
	void madeChoice(); //looks for selected component and calls callback
	CSelWindow(const std::string& text, int player, int charperline ,const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, int askID); //c-tor
	CSelWindow(){}; //c-tor
	//notification - this class inherits important destructor from CInfoWindow
};

class CRClickPopup : public CIntObject //popup displayed on R-click
{
public:
	virtual void activate();
	virtual void deactivate();
	virtual void close();
	void clickRight(tribool down, bool previousState);
	virtual ~CRClickPopup(){}; //d-tor
};

class CRClickPopupInt : public CRClickPopup //popup displayed on R-click
{
public:
	IShowActivable *inner;
	bool delInner;

	void show(SDL_Surface * to);
	CRClickPopupInt(IShowActivable *our, bool deleteInt); //c-tor
	virtual ~CRClickPopupInt(); //d-tor
};

class CInfoPopup : public CRClickPopup
{
public:
	bool free; //TODO: comment me
	SDL_Surface * bitmap; //popup background
	void close();
	void show(SDL_Surface * to);
	CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free=false); //c-tor
	CInfoPopup(SDL_Surface *Bitmap = NULL, bool Free = false); //default c-tor
	~CInfoPopup(); //d-tor
};

class SComponent : public virtual CIntObject //common popup window component
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact, experience, secskill44, spell, morale, luck, building
	} type; //component type
	int subtype; //TODO: comment me
	int val; //TODO: comment me

	std::string description; //r-click
	std::string subtitle; //TODO: comment me

	void init(Etype Type, int Subtype, int Val);
	SComponent(Etype Type, int Subtype, int Val); //c-tor
	SComponent(const Component &c); //c-tor
	SComponent(){}; //c-tor
	virtual ~SComponent(){}; //d-tor

	void clickRight(tribool down, bool previousState); //call-in
	virtual SDL_Surface * getImg();
	virtual void show(SDL_Surface * to);
	virtual void activate();
	virtual void deactivate();
};

class CCustomImgComponent :  public SComponent
{
public:
	SDL_Surface *bmp; //our image
	bool free; //should surface be freed on delete
	SDL_Surface * getImg();
	CCustomImgComponent(Etype Type, int Subtype, int Val, SDL_Surface *sur, bool freeSur); //c-tor
	~CCustomImgComponent(); //d-tor
};

class CSelectableComponent : public SComponent, public KeyShortcut
{
public:
	bool selected; //if true, this component is selected

	bool customB; //TODO: comment me
	SDL_Surface * border, *myBitmap;
	boost::function<void()> onSelect; //function called on selection change

	void clickLeft(tribool down, bool previousState); //call-in
	void init(SDL_Surface * Border);
	CSelectableComponent(Etype Type, int Sub, int Val, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL); //c-tor
	CSelectableComponent(const Component &c, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL); //c-tor
	~CSelectableComponent(); //d-tor
	virtual void show(SDL_Surface * to);
	void activate();
	void deactivate();
	void select(bool on);
	SDL_Surface * getImg(); //returns myBitmap
};
class CGarrisonInt;
class CGarrisonSlot : public CIntObject
{
public:
	CGarrisonInt *owner;
	const CCreature * creature; //creature in slot
	int count; //number of creatures
	int upg; //0 - up garrison, 1 - down garrison
	bool active; //TODO: comment me

	virtual void hover (bool on); //call-in
	const CArmedInstance * getObj();
	void clickRight(tribool down, bool previousState);
	void clickLeft(tribool down, bool previousState);
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, int Upg=0, const CCreature * Creature=NULL, int Count=0);
	~CGarrisonSlot(); //d-tor
};

class CGarrisonInt :public CIntObject
{
public:
	int interx; //space between slots
	Point garOffset, //offset between garrisons (not used if only one hero)
		surOffset; //offset between garrison position on the bg surface and position on the screen
	CGarrisonSlot *highlighted; //chosen slot
	std::vector<AdventureMapButton *> splitButtons; //may be empty if no buttons

	SDL_Surface *&sur; //bg surface
	int p2; //TODO: comment me
	bool ignoreEvent, update, active, splitting, pb, 
		smallIcons; //true - 32x32 imgs, false - 58x64
	bool removableUnits;

	const CCreatureSet *set1; //top set of creatures
	const CCreatureSet *set2; //bottom set of creatures

	std::vector<CGarrisonSlot*> *sup, *sdown; //slots of upper and lower garrison
	const CArmedInstance *oup, *odown; //upper and lower garrisons (heroes or towns)

	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void activeteSlots();
	void deactiveteSlots();
	void deleteSlots();
	void createSlots();
	void recreateSlots();

	void splitClick(); //handles click on split button
	void splitStacks(int am2); //TODO: comment me

	CGarrisonInt(int x, int y, int inx, const Point &garsOffset, SDL_Surface *&pomsur, const Point &SurOffset, const CArmedInstance *s1, const CArmedInstance *s2=NULL, bool _removableUnits = true, bool smallImgs = false); //c-tor
	~CGarrisonInt(); //d-tor
};

class CStatusBar
	: public CIntObject, public IStatusBar
{
public:
	SDL_Surface * bg; //background
	int middlex, middley; //middle of statusbar
	std::string current; //text currently printed

	CStatusBar(int x, int y, std::string name="ADROLLVR.bmp", int maxw=-1); //c-tor
	~CStatusBar(); //d-tor
	void print(const std::string & text); //prints text and refreshes statusbar
	void clear();//clears statusbar and refreshes
	void show(SDL_Surface * to); //shows statusbar (with current text)
	std::string getCurrent(); //getter for current
};

class CList : public CIntObject
{
public:
	SDL_Surface * bg; //background bitmap
	CDefHandler *arrup, *arrdo; //button arrows for scrolling list
	SDL_Surface *empty, *selection;
	SDL_Rect arrupp, arrdop; //positions of arrows
	int posw, posh; //position width/height
	int selected, //id of selected position, <0 if none
		from;
	const int SIZE; //size of list
	tribool pressed; //true=up; false=down; indeterminate=none

	CList(int Size = 5); //c-tor
	void clickLeft(tribool down, bool previousState);
	void activate();
	void deactivate();
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent)=0; //call-in
	virtual void genList()=0;
	virtual void select(int which)=0;
	virtual void draw(SDL_Surface * to)=0;
	virtual int size() = 0; //how many elements do we have
	void fixPos(); //scrolls list, so the selection will be visible
};
class CHeroList
	: public CList
{
public:
	CDefHandler *mobile, *mana; //mana and movement indicators
	int posmobx, posporx, posmanx, posmoby, pospory, posmany;

	std::vector<const CGHeroInstance *> &heroes; //points to LOCPLINT->wandering heroes 
	CHeroList(int Size); //c-tor
	int getPosOfHero(const CGHeroInstance* h); //hero's position on list
	void genList();
	void select(int which); //call-in
	void mouseMoved (const SDL_MouseMotionEvent & sEvent); //call-in
	void clickLeft(tribool down, bool previousState); //call-in
	void clickRight(tribool down, bool previousState); //call-in
	void hover (bool on); //call-in
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void updateHList(const CGHeroInstance *toRemove=NULL); //removes specific hero from the list or recreates it
	void updateMove(const CGHeroInstance* which); //draws move points bar
	void draw(SDL_Surface * to);
	void show(SDL_Surface * to);
	void init();
	int size(); //how many elements do we have
};

class CTownList
	: public CList
{
public:
	boost::function<void()> fun; //function called on selection change
	std::vector<const CGTownInstance*> items; //towns on list
	int posporx,pospory;

	CTownList(int Size, int x, int y, std::string arrupg, std::string arrdog); //c-tor
	~CTownList(); //d-tor
	void genList();
	void select(int which); //call-in
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);  //call-in
	void clickLeft(tribool down, bool previousState);  //call-in
	void clickRight(tribool down, bool previousState); //call-in
	void hover (bool on);  //call-in
	void keyPressed (const SDL_KeyboardEvent & key);  //call-in
	void draw(SDL_Surface * to);
	void show(SDL_Surface * to);
	int size(); //how many elements do we have
};

class CCreaturePic //draws picture with creature on background, use nextFrame=true to get animation
{
public:
	const CCreature *c; //which creature's picture
	bool big; //big => 100x130; !big => 100x120
	CCreatureAnimation *anim; //displayed animation
	CCreaturePic(const CCreature *cre, bool Big=true); //c-tor
	~CCreaturePic(); //d-tor
	int blitPic(SDL_Surface *to, int x, int y, bool nextFrame); //prints creature on screen
	SDL_Surface * getPic(bool nextFrame); //returns frame of animation
};

class CRecruitmentWindow : public CIntObject
{
public:
	static const int SPACE_BETWEEN = 8;
	static const int CREATURE_WIDTH = 102;
	static const int TOTAL_CREATURE_WIDTH = SPACE_BETWEEN + CREATURE_WIDTH;

	struct creinfo
	{
		SDL_Rect pos;
		CCreaturePic *pic; //creature's animation
		int ID, amount; //creature ID and available amount
		std::vector<std::pair<int,int> > res; //res_id - cost_per_unit
	};
	std::vector<int> amounts; //how many creatures we can afford
	std::vector<creinfo> creatures; //recruitable creatures
	boost::function<void(int,int)> recruit; //void (int ID, int amount) <-- call to recruit creatures
	CSlider *slider; //for selecting amount
	AdventureMapButton *max, *buy, *cancel;
	SDL_Surface *bitmap; //background
	CStatusBar *bar;
	int which; //which creature is active

	const CGDwelling *dwelling;
	int level;
	const CArmedInstance *dst;

	void close();
	void Max();
	void Buy();
	void Cancel();
	void sliderMoved(int to);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to){show(to);};
	void cleanCres();
	void initCres();
	CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const boost::function<void(int,int)> & Recruit); //creatures - pairs<creature_ID,amount> //c-tor
	~CRecruitmentWindow(); //d-tor
};

class CSplitWindow : public CIntObject
{
public:
	CGarrisonInt *gar;
	CSlider *slider;
	CCreaturePic *anim; //creature's animation
	AdventureMapButton *ok, *cancel;
	SDL_Surface *bitmap; //background
	int a1, a2, c; //TODO: comment me
	bool which; //which creature is selected
	int last; //0/1/2 - at least one creature must be in the src/dst/both stacks; -1 - no restrictions

	CSplitWindow(int cid, int max, CGarrisonInt *Owner, int Last = -1, int val=0); //c-tor; val - initial amount of second stack
	~CSplitWindow(); //d-tor
	void activate();
	void split();
	void close();
	void deactivate();
	void show(SDL_Surface * to);
	void clickLeft(tribool down, bool previousState); //call-in
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void sliderMoved(int to);
};

class CCreInfoWindow : public CIntObject
{
public:
	//bool active; //TODO: comment me
	int type;//0 - rclick popup; 1 - normal window
	SDL_Surface *bitmap; //background
	char anf; //animation counter
	std::string count; //creature count in text format

	boost::function<void()> dsm; //dismiss button callback
	CCreaturePic *anim; //related creature's animation
	CCreature *c; //related creature
	std::vector<SComponent*> upgResCost; //cost of upgrade (if not possible then empty)

	AdventureMapButton *dismiss, *upgrade, *ok;
	CCreInfoWindow(int Cid, int Type, int creatureCount, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui); //c-tor
	~CCreInfoWindow(); //d-tor
	void activate();
	void close();
	void clickRight(tribool down, bool previousState); //call-in
	void dismissF();
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void deactivate();
	void show(SDL_Surface * to);
};

class CLevelWindow : public CIntObject
{
public:
	int heroPortrait;
	SDL_Surface *bitmap; //background
	std::vector<CSelectableComponent *> comps; //skills to select
	AdventureMapButton *ok;
	boost::function<void(ui32)> cb;

	void close();
	CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback); //c-tor
	~CLevelWindow(); //d-tor
	void activate();
	void deactivate();
	void selectionChanged(unsigned to);
	void show(SDL_Surface * to);
};

class CMinorResDataBar : public CIntObject
{
public:
	SDL_Surface *bg; //background bitmap
	void show(SDL_Surface * to);
	CMinorResDataBar(); //c-tor
	~CMinorResDataBar(); //d-tor
};

class CMarketplaceWindow : public CIntObject
{
public:
	class CTradeableItem : public CIntObject
	{
	public:
		int type; //0 - res, 1 - artif big, 2 - artif small, 3 - player flag
		int id;
		bool left;
		CFunctionList<void()> callback;

		void activate();
		void deactivate();
		void show(SDL_Surface * to);
		void clickLeft(tribool down, bool previousState);
		SDL_Surface *getSurface();
		CTradeableItem(int Type, int ID, bool Left);
	};

	SDL_Surface *bg; //background
	std::vector<CTradeableItem*> left, right;
	std::vector<std::string> rSubs; //offer caption
	CTradeableItem *hLeft, *hRight; //highlighted items (NULL if no highlight)

	int mode,//0 - res<->res; 1 - res<->plauer; 2 - buy artifact; 3 - sell artifact
		r1, r2; //suggested amounts of traded resources
	AdventureMapButton *ok, *max, *deal;
	CSlider *slider; //for choosing amount to be exchanged

	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void setMax();
	void sliderMoved(int to);
	void makeDeal();
	void selectionChanged(bool side); //true == left
	CMarketplaceWindow(int Mode=0); //c-tor
	~CMarketplaceWindow(); //d-tor
	void setMode(int mode); //mode setter
	void clear();
};

class CSystemOptionsWindow : public CIntObject
{
private:
	SDL_Surface * background; //background of window
	AdventureMapButton *load, *save, *restart, *mainMenu, *quitGame, *backToMap; //load and restart are not used yet
	CHighlightableButtonsGroup * heroMoveSpeed;
	CHighlightableButtonsGroup * mapScrollSpeed;
	CHighlightableButtonsGroup * musicVolume, * effectsVolume;
public:
	CSystemOptionsWindow(const SDL_Rect & pos, CPlayerInterface * owner); //c-tor
	~CSystemOptionsWindow(); //d-tor

	//functions bound to buttons
	void bsavef(); //save game
	void bquitf(); //quit game
	void breturnf(); //return to game
	void bmainmenuf(); //return to main menu

	void pushSDLEvent(int type, int usercode);

	void activate();
	void deactivate();
	void show(SDL_Surface * to);
};

class CTavernWindow : public CIntObject
{
public:
	class HeroPortrait : public CIntObject
	{
	public:
		std::string hoverName;
		vstd::assigner<int,int> as;
		const CGHeroInstance *h;
		void activate();
		void deactivate();
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover (bool on);
		HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H);
		void show(SDL_Surface * to);
		char descr[100];		// "XXX is a level Y ZZZ with N artifacts"
	} h1, h2; //recruitable heroes

	SDL_Surface *bg; //background
	CStatusBar *bar; //tavern's internal status bar
	int selected;//0 (left) or 1 (right)
	int oldSelected;//0 (left) or 1 (right)

	AdventureMapButton *thiefGuild, *cancel, *recruit;

	CTavernWindow(const CGHeroInstance *H1, const CGHeroInstance *H2, const std::string &gossip); //c-tor
	~CTavernWindow(); //d-tor

	void recruitb();
	void close();
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
};

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, int > > texts; //<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously
public:
	std::string enteredText;
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key); //call-in

	void startEnteringText();
	void endEnteringText(bool printEnteredText);
	void refreshEnteredText();

	CInGameConsole(); //c-tor
};


class LClickableArea: public virtual CIntObject
{
public:
	virtual void clickLeft(tribool down, bool previousState);
	virtual void activate();
	virtual void deactivate();
};

class RClickableArea: public virtual CIntObject
{
public:
	virtual void clickRight(tribool down, bool previousState);
	virtual void activate();
	virtual void deactivate();
};

class LClickableAreaHero : public LClickableArea
{
public:
	int id;
	CHeroWindow * owner;
	virtual void clickLeft(tribool down, bool previousState);
};

class LRClickableAreaWText: public LClickableArea, public RClickableArea
{
public:
	std::string text, hoverText;
	virtual void activate();
	virtual void deactivate();
	virtual void clickLeft(tribool down, bool previousState);
	virtual void clickRight(tribool down, bool previousState);
	virtual void hover(bool on);
};

class LRClickableAreaWTextComp: public LClickableArea, public RClickableArea
{
public:
	std::string text, hoverText;
	int baseType;
	int bonus, type;
	virtual void activate();
	virtual void deactivate();
	virtual void clickLeft(tribool down, bool previousState);
	virtual void clickRight(tribool down, bool previousState);
	virtual void hover(bool on);
};

class CArtPlace: public LRClickableAreaWTextComp
{
private:
	bool active;
public:
	//bool spellBook, warMachine1, warMachine2, warMachine3, warMachine4,
	//	misc1, misc2, misc3, misc4, misc5, feet, lRing, rRing, torso,
	//	lHand, rHand, neck, shoulders, head; //my types
	ui16 slotID; //0   	head	1 	shoulders		2 	neck		3 	right hand		4 	left hand		5 	torso		6 	right ring		7 	left ring		8 	feet		9 	misc. slot 1		10 	misc. slot 2		11 	misc. slot 3		12 	misc. slot 4		13 	ballista (war machine 1)		14 	ammo cart (war machine 2)		15 	first aid tent (war machine 3)		16 	catapult		17 	spell book		18 	misc. slot 5		19+ 	backpack slots

	bool marked;
	CArtifactsOfHero * ourOwner;
	const CArtifact * ourArt;
	CArtPlace(const CArtifact * Art); //c-tor
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void select ();
	void deselect ();
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	bool fitsHere(const CArtifact * art); //returns true if given artifact can be placed here
	~CArtPlace(); //d-tor
};


class CArtifactsOfHero : public CIntObject
{
	const CGHeroInstance * curHero;

	std::vector<CArtPlace *> artWorn; // 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::vector<CArtPlace *> backpack; //hero's visible backpack (only 5 elements!)
	int backpackPos; //unmber of first art visible in backpack (in hero's vector)

public:
	struct SCommonPart
	{
		std::set<CArtifactsOfHero *> participants; // Needed to mark slots.
		const CArtifact * srcArtifact;    // Held artifact.
		const CArtifactsOfHero * srcAOH;    // Following two needed to uniquely identify the source.
		int srcSlotID;                      //
		const CArtifactsOfHero * destAOH; // For swapping. (i.e. changing what is held)
		int destSlotID;	                     // Needed to determine what kind of action was last taken in setHero
		const CArtifact * destArtifact;    // For swapping.
	} * commonInfo; //when we have more than one CArtifactsOfHero in one window with exchange possibility, we use this (eg. in exchange window); to be provided externally

	AdventureMapButton * leftArtRoll, * rightArtRoll;

	void activate();
	void deactivate();
	void show(SDL_Surface * to);

	void setHero(const CGHeroInstance * hero);
	void dispose(); //free resources not needed after closing windows and reset state
	void rollback();
	void scrollBackpack(int dir); //dir==-1 => to left; dir==1 => to right
	void markPossibleSlots (const CArtifact* art);
	void unmarkSlots ();
	void setSlotData (CArtPlace* artPlace, int slotID);
	void eraseSlotData (CArtPlace* artPlace, int slotID);

	CArtifactsOfHero(const SDL_Rect & position); //c-tor
	~CArtifactsOfHero(); //d-tor

	friend class CArtPlace;
};

class CGarrisonWindow : public CWindowWithGarrison
{
public:
	SDL_Surface *bg; //background surface
	AdventureMapButton *quit;

	void close();
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to){show(to);};
	CGarrisonWindow(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits); //c-tor
	~CGarrisonWindow(); //d-tor
};

class CExchangeWindow : public CWindowWithGarrison
{
	CStatusBar * ourBar; //internal statusbar

	SDL_Surface *bg; //background
	AdventureMapButton * quit, * questlogButton[2];

	std::vector<LRClickableAreaWTextComp *> secSkillAreas[2], primSkillAreas;

	LRClickableAreaWTextComp *morale[2], *luck[2];

public:

	const CGHeroInstance * heroInst[2];
	CArtifactsOfHero * artifs[2];

	void close();
	void activate();
	void deactivate();
	void show(SDL_Surface * to);

	void questlog(int whichHero); //questlog button callback; whichHero: 0 - left, 1 - right

	void prepareBackground(); //prepares or redraws bg

	CExchangeWindow(si32 hero1, si32 hero2); //c-tor
	~CExchangeWindow(); //d-tor
};

class CShipyardWindow : public CIntObject
{
public:
	CStatusBar *bar;
	SDL_Surface *bg; //background
	AdventureMapButton *build, *quit;

	unsigned char frame; //frame of the boat animation

	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	CShipyardWindow(const std::vector<si32> &cost, int state, const boost::function<void()> &onBuy);
	~CShipyardWindow();
};

class CPuzzleWindow : public CIntObject
{
private:
	SDL_Surface * background;
	AdventureMapButton * quitb;
	CResDataBar * resdatabar;

	std::vector<std::pair<SDL_Surface *, SPuzzleInfo *> > puzzlesToPullBack;
	ui8 animCount;

public:
	void activate();
	void deactivate();
	void show(SDL_Surface * to);

	CPuzzleWindow();
	~CPuzzleWindow();
};

class CShopWindow : public CIntObject
{
public:
	std::map<ui16, Component> available, chosen, bought;

	bool swapItem (ui16 which, bool choose);
	virtual void Buy() {};
};
class CArtMerchantWindow : public CShopWindow
{
public:
	void activate();
	void deactivate();	
	void show(SDL_Surface * to);
	void Buy();

	CArtMerchantWindow();
	~CArtMerchantWindow();
};
class CUniversityWindow : public CShopWindow
{};
class CAltarWindow : public CShopWindow
{};
class CRefugeeCampWindow : public CShopWindow
{};
class CWarMachineWindow : public CShopWindow
{};
class CFreelancersWindow : public CShopWindow
{};
class CHillFortWindow : public CIntObject //garrison dialog? shop?
{};
#endif //__GUICLASSES_H__
