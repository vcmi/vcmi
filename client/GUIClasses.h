#pragma once

#include "CAnimation.h"
#include "../lib/FunctionList.h"
#include "../lib/ResourceSet.h"
#include "../lib/CConfigHandler.h"
#include "../lib/GameConstants.h"
#include "gui/CIntObject.h"
#include "gui/CIntObjectClasses.h"
#include "../lib/GameConstants.h"
#include "gui/CArtifactHolder.h"
#include "gui/CComponent.h"
#include "gui/CGarrisonInt.h"

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

struct ArtifactLocation;
class CStackBasicDescriptor;
class CBonusSystemNode;
class CArtifact;
class CDefEssential;
class CAdventureMapButton;
class CHighlightableButtonsGroup;
class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
class CStack;
class CComponent;
class CCreature;
struct SDL_Surface;
struct CPath;
class CCreatureAnim;
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
struct Component;
class CArmedInstance;
class CGTownInstance;
class StackState;
class CPlayerInterface;
class CHeroWindow;
class CArtifact;
class CArtifactsOfHero;
class CCreatureArtifactSet;
class CResDataBar;
struct SPuzzleInfo;
class CGGarrison;
class CStackInstance;
class IMarket;
class CTextBox;
class CArtifactInstance;
class IBonusBearer;
class CArtPlace;
class CAnimImage;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

/// text + comp. + ok button
class CInfoWindow : public CSimpleWindow
{ //window able to delete its components when closed
	bool delComps; //whether comps will be deleted

public:
	typedef std::vector<std::pair<std::string,CFunctionList<void()> > > TButtonsInfo;
	typedef std::vector<CComponent*> TCompsInfo;
	QueryID ID; //for identification
	CTextBox *text;
	std::vector<CAdventureMapButton *> buttons;
	std::vector<CComponent*> components;
	CSlider *slider;

	void setDelComps(bool DelComps);
	virtual void close();

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void sliderMoved(int to);

	CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo &comps = TCompsInfo(), const TButtonsInfo &Buttons = TButtonsInfo(), bool delComps = true); //c-tor
	CInfoWindow(); //c-tor
	~CInfoWindow(); //d-tor

	//use only before the game starts! (showYesNoDialog in LOCPLINT must be used then)
	static void showInfoDialog( const std::string & text, const std::vector<CComponent*> *components, bool DelComps = true, PlayerColor player = PlayerColor(1));
	static void showOkDialog(const std::string & text, const std::vector<CComponent*> *components, const boost::function<void()> & onOk, bool delComps = true, PlayerColor player = PlayerColor(1));
	static void showYesNoDialog( const std::string & text, const std::vector<CComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps = true, PlayerColor player = PlayerColor(1));
	static CInfoWindow *create(const std::string &text, PlayerColor playerID = PlayerColor(1), const std::vector<CComponent*> *components = nullptr, bool DelComps = false);

	/// create text from title and description: {title}\n\n description
	static std::string genText(std::string title, std::string description);
};

/// popup displayed on R-click
class CRClickPopup : public CIntObject
{
public:
	virtual void close();
	void clickRight(tribool down, bool previousState);

	CRClickPopup();
	virtual ~CRClickPopup(); //d-tor

	static CIntObject* createInfoWin(Point position, const CGObjectInstance * specific);
	static void createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps = CInfoWindow::TCompsInfo());
	static void createAndPush(const std::string &txt, CComponent * component);
	static void createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment = BOTTOMRIGHT);
};

/// popup displayed on R-click
class CRClickPopupInt : public CRClickPopup
{
public:
	IShowActivatable *inner;
	bool delInner;

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	CRClickPopupInt(IShowActivatable *our, bool deleteInt); //c-tor
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
	CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free=false); //c-tor
	CInfoPopup(SDL_Surface * Bitmap = nullptr, bool Free = false); //default c-tor

	void init(int x, int y);
	~CInfoPopup(); //d-tor
};

/// popup on adventure map for town\hero objects
class CInfoBoxPopup : public CWindowObject
{
	Point toScreen(Point pos);
public:
	CInfoBoxPopup(Point position, const CGTownInstance * town);
	CInfoBoxPopup(Point position, const CGHeroInstance * hero);
	CInfoBoxPopup(Point position, const CGGarrison * garr);
};

/// component selection window
class CSelWindow : public CInfoWindow
{ //warning - this window deletes its components by closing!
public:
	void selectionChange(unsigned to);
	void madeChoice(); //looks for selected component and calls callback
	CSelWindow(const std::string& text, PlayerColor player, int charperline ,const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, QueryID askID); //c-tor
	CSelWindow(){}; //c-tor
	//notification - this class inherits important destructor from CInfoWindow
};


////////////////////////////////////////////////////////////////////////////////

/// base class for hero/town/garrison tooltips
class CArmyTooltip : public CIntObject
{
	void init(const InfoAboutArmy &army);
public:
	CArmyTooltip(Point pos, const InfoAboutArmy &army);
	CArmyTooltip(Point pos, const CArmedInstance * army);
};

/// Class for hero tooltip. Does not have any background!
/// background for infoBox: ADSTATHR
/// background for tooltip: HEROQVBK
class CHeroTooltip : public CArmyTooltip
{
	void init(const InfoAboutHero &hero);
public:
	CHeroTooltip(Point pos, const InfoAboutHero &hero);
	CHeroTooltip(Point pos, const CGHeroInstance * hero);
};

/// Class for town tooltip. Does not have any background!
/// background for infoBox: ADSTATCS
/// background for tooltip: TOWNQVBK
class CTownTooltip : public CArmyTooltip
{
	void init(const InfoAboutTown &town);
public:
	CTownTooltip(Point pos, const InfoAboutTown &town);
	CTownTooltip(Point pos, const CGTownInstance * town);
};

/// draws picture with creature on background, use Animated=true to get animation
class CCreaturePic : public CIntObject
{
private:
	CPicture *bg;
	CCreatureAnim *anim; //displayed animation

public:
	CCreaturePic(int x, int y, const CCreature *cre, bool Big=true, bool Animated=true); //c-tor
};

/// Recruitment window where you can recruit creatures
class CRecruitmentWindow : public CWindowObject
{
	class CCreatureCard : public CIntObject
	{
		CRecruitmentWindow * parent;
		CCreaturePic *pic; //creature's animation
		bool selected;

		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void showAll(SDL_Surface *to);
	public:
		const CCreature * creature;
		si32 amount;

		void select(bool on);

		CCreatureCard(CRecruitmentWindow * window, const CCreature *crea, int totalAmount);
	};

	/// small class to display creature costs
	class CCostBox : public CIntObject
	{
		std::map<int, std::pair<CLabel *, CAnimImage * > > resources;
	public:
		//res - resources to show
		void set(TResources res);
		//res - visible resources
		CCostBox(Rect position, std::string title);
		void createItems(TResources res);
	};

	std::function<void(CreatureID,int)> onRecruit; //void (int ID, int amount) <-- call to recruit creatures

	int level;
	const CArmedInstance *dst;

	CCreatureCard * selected;
	std::vector<CCreatureCard *> cards;

	CSlider *slider; //for selecting amount
	CAdventureMapButton *maxButton, *buyButton, *cancelButton;
	//labels for visible values
	CLabel * title;
	CLabel * availableValue;
	CLabel * toRecruitValue;
	CCostBox * costPerTroopValue;
	CCostBox * totalCostValue;

	void select(CCreatureCard * card);
	void buy();
	void sliderMoved(int to);

	void showAll(SDL_Surface *to);
public:
	const CGDwelling * const dwelling;
	CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const std::function<void(CreatureID,int)> & Recruit, int y_offset = 0); //creatures - pairs<creature_ID,amount> //c-tor
	void availableCreaturesChanged();
};

/// Split window where creatures can be split up into two single unit stacks
class CSplitWindow : public CWindowObject
{
	std::function<void(int, int)> callback;
	int leftAmount;
	int rightAmount;

	int leftMin;
	int rightMin;

	CSlider *slider;
	CCreaturePic *animLeft, *animRight; //creature's animation
	CAdventureMapButton *ok, *cancel;

	CTextInput *leftInput, *rightInput;
	void setAmountText(std::string text, bool left);
	void setAmount(int value, bool left);
	void sliderMoved(int value);
	void apply();

public:
	/**
	 * creature - displayed creature
	 * callback(leftAmount, rightAmount) - function to call on close
	 * leftMin, rightMin - minimal amount of creatures in each stack
	 * leftAmount, rightAmount - amount of creatures in each stack
	 */
	CSplitWindow(const CCreature * creature, std::function<void(int, int)> callback,
	             int leftMin, int rightMin, int leftAmount, int rightAmount);
};

/// Raised up level windowe where you can select one out of two skills
class CLevelWindow : public CWindowObject
{
	CComponentBox * box; //skills to select
	std::function<void(ui32)> cb;

	void selectionChanged(unsigned to);
public:

	CLevelWindow(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, std::function<void(ui32)> callback); //c-tor
	~CLevelWindow(); //d-tor

};

/// Resource bar like that at the bottom of the adventure map screen
class CMinorResDataBar : public CIntObject
{
public:
	SDL_Surface *bg; //background bitmap
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	CMinorResDataBar(); //c-tor
	~CMinorResDataBar(); //d-tor
};

/// Town portal, castle gate window
class CObjectListWindow : public CWindowObject
{
	class CItem : public CIntObject
	{
		CObjectListWindow *parent;
		CLabel *text;
		CPicture *border;
	public:
		const size_t index;
		CItem(CObjectListWindow *parent, size_t id, std::string text);

		void select(bool on);
		void clickLeft(tribool down, bool previousState);
	};

	std::function<void(int)> onSelect;//called when OK button is pressed, returns id of selected item.
	CLabel * title;
	CLabel * descr;

	CListBox * list;
	CIntObject * titleImage;//title image (castle gate\town portal picture)
	CAdventureMapButton *ok, *exit;

	std::vector< std::pair<int, std::string> > items;//all items present in list

	void init(CIntObject * titlePic, std::string _title, std::string _descr);
public:
	size_t selected;//index of currently selected item
	/// Callback will be called when OK button is pressed, returns id of selected item. initState = initially selected item
	/// Image can be nullptr
	///item names will be taken from map objects
	CObjectListWindow(const std::vector<int> &_items, CIntObject * titlePic, std::string _title, std::string _descr,
                      std::function<void(int)> Callback);
	CObjectListWindow(const std::vector<std::string> &_items, CIntObject * titlePic, std::string _title, std::string _descr,
                      std::function<void(int)> Callback);

	CIntObject *genItem(size_t index);
	void elementSelected();//call callback and close this window
	void changeSelection(size_t which);
	void keyPressed (const SDL_KeyboardEvent & key);
};

class CSystemOptionsWindow : public CWindowObject
{
private:
	CLabel *title;
	CLabelGroup *leftGroup;
	CLabelGroup *rightGroup;
	CAdventureMapButton *load, *save, *restart, *mainMenu, *quitGame, *backToMap; //load and restart are not used yet
	CHighlightableButtonsGroup * heroMoveSpeed;
	CHighlightableButtonsGroup * mapScrollSpeed;
	CHighlightableButtonsGroup * musicVolume, * effectsVolume;

	//CHighlightableButton * showPath;
	CHighlightableButton * showReminder;
	CHighlightableButton * quickCombat;
	CHighlightableButton * spellbookAnim;
	CHighlightableButton * newCreatureWin;
	CHighlightableButton * fullscreen;

	CAdventureMapButton *gameResButton;
	CLabel *gameResLabel;

	SettingsListener onFullscreenChanged;

	void setMusicVolume( int newVolume );
	void setSoundVolume( int newVolume );
	void setHeroMoveSpeed( int newSpeed );
	void setMapScrollingSpeed( int newSpeed );

	//functions bound to buttons
	void bloadf(); //load game
	void bsavef(); //save game
	void bquitf(); //quit game
	void breturnf(); //return to game
	void brestartf(); //restart game
	void bmainmenuf(); //return to main menu

	//functions for checkboxes
	void toggleReminder(bool on);
	void toggleQuickCombat(bool on);
	void toggleSpellbookAnim(bool on);
	void toggleCreatureWin(bool on);
	void toggleFullscreen(bool on);

	void selectGameRes();
	void setGameRes(int index);
	void closeAndPushEvent(int eventType, int code = 0);

public:
	CSystemOptionsWindow(); //c-tor
};

class CTavernWindow : public CWindowObject
{
public:
	class HeroPortrait : public CIntObject
	{
	public:
		std::string hoverName;
		std::string description; // "XXX is a level Y ZZZ with N artifacts"
		const CGHeroInstance *h;

		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover (bool on);
		HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H);

	private:
		int *_sel;
		const int _id;

	} *h1, *h2; //recruitable heroes

	CGStatusBar *bar; //tavern's internal status bar
	int selected;//0 (left) or 1 (right)
	int oldSelected;//0 (left) or 1 (right)

	CAdventureMapButton *thiefGuild, *cancel, *recruit;
	const CGObjectInstance *tavernObj;

	CTavernWindow(const CGObjectInstance *TavernObj); //c-tor
	~CTavernWindow(); //d-tor

	void recruitb();
	void thievesguildb();
	void show(SDL_Surface * to);
};

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, int > > texts; //list<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously
public:
	std::string enteredText;
	void show(SDL_Surface * to);
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key); //call-in

#ifndef VCMI_SDL1
	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;
#endif // VCMI_SDL1

	void startEnteringText();
	void endEnteringText(bool printEnteredText);
	void refreshEnteredText();

	CInGameConsole(); //c-tor
};

class MoraleLuckBox : public LRClickableAreaWTextComp
{
	CAnimImage *image;
public:
	bool morale; //true if morale, false if luck
	bool small;

	void set(const IBonusBearer *node);

	MoraleLuckBox(bool Morale, const Rect &r, bool Small=false);
};

/// Opens hero window by left-clicking on it
class CHeroArea: public CIntObject
{
	const CGHeroInstance * hero;
public:

	CHeroArea(int x, int y, const CGHeroInstance * _hero);

	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);
};

/// Opens town screen by left-clicking on it
class LRClickableAreaOpenTown: public LRClickableAreaWTextComp
{
public:
	const CGTownInstance * town;
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	LRClickableAreaOpenTown();
};

class CExchangeWindow : public CWindowObject, public CWindowWithGarrison, public CWindowWithArtifacts
{
	CGStatusBar * ourBar; //internal statusbar

	CAdventureMapButton * quit, * questlogButton[2];

	std::vector<LRClickableAreaWTextComp *> secSkillAreas[2], primSkillAreas;

	MoraleLuckBox *morale[2], *luck[2];

	LRClickableAreaWText *specialty[2];
	LRClickableAreaWText *experience[2];
	LRClickableAreaWText *spellPoints[2];
	CHeroArea *portrait[2];

public:

	const CGHeroInstance* heroInst[2];
	CArtifactsOfHero * artifs[2];

	void questlog(int whichHero); //questlog button callback; whichHero: 0 - left, 1 - right

	void prepareBackground(); //prepares or redraws bg

	CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID); //c-tor
	~CExchangeWindow(); //d-tor
};

/// Here you can buy ships
class CShipyardWindow : public CWindowObject
{
public:
	CGStatusBar *bar;
	CPicture *bgWater;

	CLabel *title;
	CLabel *costLabel;

	CAnimImage *woodPic, *goldPic;
	CLabel *woodCost, *goldCost;

	CAnimImage *bgShip;
	CAdventureMapButton *build, *quit;

	CGStatusBar * statusBar;

	CShipyardWindow(const std::vector<si32> &cost, int state, int boatType, const std::function<void()> &onBuy);
};

/// Puzzle screen which gets uncovered when you visit obilisks
class CPuzzleWindow : public CWindowObject
{
private:
	int3 grailPos;

	CAdventureMapButton * quitb;

	std::vector<CPicture * > piecesToRemove;
	ui8 currentAlpha;

public:
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);

	CPuzzleWindow(const int3 &grailPos, double discoveredRatio);
};

/// Creature transformer window
class CTransformerWindow : public CWindowObject, public CGarrisonHolder
{
public:
	class CItem : public CIntObject
	{
	public:
		int id;//position of creature in hero army
		bool left;//position of the item
		int size; //size of creature stack
		CTransformerWindow * parent;
		CAnimImage *icon;

		void move();
		void clickLeft(tribool down, bool previousState);
		void update();
		CItem(CTransformerWindow * parent, int size, int id);
	};

	const CArmedInstance *army;//object with army for transforming (hero or town)
	const CGHeroInstance *hero;//only if we have hero in town
	const CGTownInstance *town;//market, town garrison is used if hero == nullptr
	std::vector<CItem*> items;

	CAdventureMapButton *all, *convert, *cancel;
	CGStatusBar *bar;
	void makeDeal();
	void addAll();
	void updateGarrisons();
	CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town); //c-tor
};

class CUniversityWindow : public CWindowObject
{
	class CItem : public CAnimImage
	{
	public:
		int ID;//id of selected skill
		CUniversityWindow * parent;

		void showAll(SDL_Surface * to);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover(bool on);
		int state();//0=can't learn, 1=learned, 2=can learn
		CItem(CUniversityWindow * _parent, int _ID, int X, int Y);
	};

public:
	const CGHeroInstance *hero;
	const IMarket * market;

	CPicture * green, * yellow, * red;//colored bars near skills
	std::vector<CItem*> items;

	CAdventureMapButton *cancel;
	CGStatusBar *bar;

	CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market); //c-tor
};

/// Confirmation window for University
class CUnivConfirmWindow : public CWindowObject
{
public:
	CUniversityWindow * parent;
	CGStatusBar *bar;
	CAdventureMapButton *confirm, *cancel;

	CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available); //c-tor
	void makeDeal(int skill);
};

/// Hill fort is the building where you can upgrade units
class CHillFortWindow : public CWindowObject, public CWindowWithGarrison
{
public:

	int slotsCount;//=7;
	CGStatusBar * bar;
	CDefEssential *resources;
	CHeroArea *heroPic;//clickable hero image
	CAdventureMapButton *quit,//closes window
	                   *upgradeAll,//upgrade all creatures
	                   *upgrade[7];//upgrade single creature

	const CGObjectInstance * fort;
	const CGHeroInstance * hero;
	std::vector<int> currState;//current state of slot - to avoid calls to getState or updating buttons
	std::vector<TResources> costs;// costs [slot ID] [resource ID] = resource count for upgrade
	TResources totalSumm; // totalSum[resource ID] = value

	CHillFortWindow(const CGHeroInstance *visitor, const CGObjectInstance *object); //c-tor

	void showAll (SDL_Surface *to);
	std::string getDefForSlot(SlotID slot);//return def name for this slot
	std::string getTextForSlot(SlotID slot);//return hover text for this slot
	void makeDeal(SlotID slot);//-1 for upgrading all creatures
	int getState(SlotID slot); //-1 = no creature 0=can't upgrade, 1=upgraded, 2=can upgrade
	void updateGarrisons();//update buttons after garrison changes
};

class CThievesGuildWindow : public CWindowObject
{
	const CGObjectInstance * owner;

	CGStatusBar * statusBar;
	CAdventureMapButton * exitb;
	CMinorResDataBar * resdatabar;

public:
	CThievesGuildWindow(const CGObjectInstance * _owner);
};
