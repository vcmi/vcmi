/*
 * GUIClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/GameConstants.h"
#include "../lib/ResourceSet.h"
#include "../lib/CConfigHandler.h"
#include "../widgets/CArtifactHolder.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"
#include "../windows/CWindowObject.h"

class CGDwelling;
class CreatureCostBox;
class IMarket;
class CCreaturePic;
class MoraleLuckBox;
class CHeroArea;
class CMinorResDataBar;
class CSlider;
class CComponentBox;
class CTextInput;
class CListBox;
class CLabelGroup;
class CToggleButton;
class CToggleGroup;
class CVolumeSlider;
class CGStatusBar;

/// Recruitment window where you can recruit creatures
class CRecruitmentWindow : public CWindowObject
{
	class CCreatureCard : public CIntObject
	{
		CRecruitmentWindow * parent;
		CCreaturePic *pic; //creature's animation
		bool selected;

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void showAll(SDL_Surface *to) override;
	public:
		const CCreature * creature;
		si32 amount;

		void select(bool on);

		CCreatureCard(CRecruitmentWindow * window, const CCreature *crea, int totalAmount);
	};

	std::function<void(CreatureID,int)> onRecruit; //void (int ID, int amount) <-- call to recruit creatures

	int level;
	const CArmedInstance *dst;

	CCreatureCard * selected;
	std::vector<CCreatureCard *> cards;

	CSlider *slider; //for selecting amount
	CButton *maxButton, *buyButton, *cancelButton;
	//labels for visible values
	CLabel * title;
	CLabel * availableValue;
	CLabel * toRecruitValue;
	CreatureCostBox * costPerTroopValue;
	CreatureCostBox * totalCostValue;

	void select(CCreatureCard * card);
	void buy();
	void sliderMoved(int to);

	void showAll(SDL_Surface *to) override;
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
	CButton *ok, *cancel;

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

	CLevelWindow(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, std::function<void(ui32)> callback);
	~CLevelWindow();

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
		void clickLeft(tribool down, bool previousState) override;
	};

	std::function<void(int)> onSelect;//called when OK button is pressed, returns id of selected item.
	CLabel * title;
	CLabel * descr;

	CListBox * list;
	CButton *ok, *exit;

	std::vector< std::pair<int, std::string> > items;//all items present in list

	void init(CIntObject * titlePic, std::string _title, std::string _descr);
	void exitPressed();
public:
	size_t selected;//index of currently selected item

	std::function<void()> onExit;//optional exit callback

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
	void keyPressed (const SDL_KeyboardEvent & key) override;
};

class CSystemOptionsWindow : public CWindowObject
{
private:
	CLabel *title;
	CLabelGroup *leftGroup;
	CLabelGroup *rightGroup;
	CButton *load, *save, *restart, *mainMenu, *quitGame, *backToMap; //load and restart are not used yet
	CToggleGroup * heroMoveSpeed;
	CToggleGroup * enemyMoveSpeed;
	CToggleGroup * mapScrollSpeed;
	CVolumeSlider * musicVolume, * effectsVolume;

	//CHighlightableButton * showPath;
	CToggleButton * showReminder;
	CToggleButton * quickCombat;
	CToggleButton * spellbookAnim;
	CToggleButton * fullscreen;

	CButton *gameResButton;
	CLabel *gameResLabel;

	SettingsListener onFullscreenChanged;

	//functions bound to buttons
	void bloadf(); //load game
	void bsavef(); //save game
	void bquitf(); //quit game
	void breturnf(); //return to game
	void brestartf(); //restart game
	void bmainmenuf(); //return to main menu

	void selectGameRes();
	void setGameRes(int index);
	void closeAndPushEvent(int eventType, int code = 0);

public:
	CSystemOptionsWindow();
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

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover (bool on) override;
		HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H);

	private:
		int *_sel;
		const int _id;

	} *h1, *h2; //recruitable heroes

	int selected;//0 (left) or 1 (right)
	int oldSelected;//0 (left) or 1 (right)

	CButton *thiefGuild, *cancel, *recruit;
	const CGObjectInstance *tavernObj;

	CTavernWindow(const CGObjectInstance *TavernObj);
	~CTavernWindow();

	void recruitb();
	void thievesguildb();
	void show(SDL_Surface * to) override;
};

class CExchangeWindow : public CWindowObject, public CWindowWithGarrison, public CWindowWithArtifacts
{
	CGStatusBar * ourBar; //internal statusbar

	CButton * quit, * questlogButton[2];

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

	CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID);
	~CExchangeWindow();
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
	CButton *build, *quit;

	CGStatusBar * statusBar;

	CShipyardWindow(const std::vector<si32> &cost, int state, int boatType, const std::function<void()> &onBuy);
};

/// Puzzle screen which gets uncovered when you visit obilisks
class CPuzzleWindow : public CWindowObject
{
private:
	int3 grailPos;

	CButton * quitb;

	std::vector<CPicture * > piecesToRemove;
	ui8 currentAlpha;

public:
	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;

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
		void clickLeft(tribool down, bool previousState) override;
		void update();
		CItem(CTransformerWindow * parent, int size, int id);
	};

	const CArmedInstance *army;//object with army for transforming (hero or town)
	const CGHeroInstance *hero;//only if we have hero in town
	const CGTownInstance *town;//market, town garrison is used if hero == nullptr
	std::vector<CItem*> items;

	CButton *all, *convert, *cancel;
	CGStatusBar *bar;
	void makeDeal();
	void addAll();
	void updateGarrisons() override;
	CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town);
};

class CUniversityWindow : public CWindowObject
{
	class CItem : public CAnimImage
	{
	public:
		int ID;//id of selected skill
		CUniversityWindow * parent;

		void showAll(SDL_Surface * to) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;
		int state();//0=can't learn, 1=learned, 2=can learn
		CItem(CUniversityWindow * _parent, int _ID, int X, int Y);
	};

public:
	const CGHeroInstance *hero;
	const IMarket * market;

	CPicture * green, * yellow, * red;//colored bars near skills
	std::vector<CItem*> items;

	CButton *cancel;
	CGStatusBar *bar;

	CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market);
};

/// Confirmation window for University
class CUnivConfirmWindow : public CWindowObject
{
public:
	CUniversityWindow * parent;
	CGStatusBar *bar;
	CButton *confirm, *cancel;

	CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available);
	void makeDeal(int skill);
};

/// Hill fort is the building where you can upgrade units
class CHillFortWindow : public CWindowObject, public CWindowWithGarrison
{
private:
	static const int slotsCount = 7;
	//todo: mithril support
	static const int resCount = 7;

	const CGObjectInstance * fort;
	const CGHeroInstance * hero;

	CGStatusBar * bar;
	CHeroArea * heroPic;//clickable hero image
	CButton * quit;//closes window
	CButton * upgradeAll;//upgrade all creatures

	std::array<CButton *, slotsCount> upgrade;//upgrade single creature
	std::array<int, slotsCount + 1> currState;//current state of slot - to avoid calls to getState or updating buttons

	//there is a place for only 2 resources per slot
	std::array< std::array<CAnimImage *, 2>, slotsCount> slotIcons;
	std::array< std::array<CLabel *, 2>, slotsCount> slotLabels;

	std::array<CAnimImage *, resCount> totalIcons;
	std::array<CLabel *, resCount> totalLabels;

	std::string getDefForSlot(SlotID slot);//return def name for this slot
	std::string getTextForSlot(SlotID slot);//return hover text for this slot
	void makeDeal(SlotID slot);//-1 for upgrading all creatures
	int getState(SlotID slot); //-1 = no creature 0=can't upgrade, 1=upgraded, 2=can upgrade
public:
	CHillFortWindow(const CGHeroInstance * visitor, const CGObjectInstance * object);
	void updateGarrisons() override;//update buttons after garrison changes
};

class CThievesGuildWindow : public CWindowObject
{
	const CGObjectInstance * owner;

	CGStatusBar * statusBar;
	CButton * exitb;
	CMinorResDataBar * resdatabar;

public:
	CThievesGuildWindow(const CGObjectInstance * _owner);
};
