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

#include "CWindowObject.h"
#include "../lib/ResourceSet.h"
#include "../widgets/Images.h"
#include "../widgets/IVideoHolder.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGObjectInstance;
class CGDwelling;
class IMarket;

VCMI_LIB_NAMESPACE_END

class CButton;
class LRClickableArea;
class CreatureCostBox;
class CCreaturePic;
class CMinorResDataBar;
class MoraleLuckBox;
class CHeroArea;
class CSlider;
class CComponentBox;
class CTextInput;
class CListBox;
class CLabelGroup;
class CGStatusBar;
class CTextBox;
class CGarrisonInt;
class CGarrisonSlot;
class CHeroArea;
class CAnimImage;
class CFilledTexture;
class IImage;
class VideoWidget;
class VideoWidgetOnce;
class GraphicalPrimitiveCanvas;
class TransparentFilledRectangle;
class CSecSkillPlace;

enum class EUserEvent;

/// Recruitment window where you can recruit creatures
class CRecruitmentWindow : public CStatusbarWindow
{
	class CCreatureCard : public CIntObject, public std::enable_shared_from_this<CCreatureCard>
	{
		CRecruitmentWindow * parent;
		std::shared_ptr<CCreaturePic> animation;
		bool selected;

	public:
		const CCreature * creature;
		si32 amount;

		void select(bool on);

		CCreatureCard(CRecruitmentWindow * window, const CCreature * crea, int totalAmount);

		void clickPressed(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;
		void showAll(Canvas & to) override;
	};

	std::function<void(CreatureID,int)> onRecruit; //void (int ID, int amount) <-- call to recruit creatures
	std::function<void()> onClose;

	int level;
	const CArmedInstance * dst;

	std::shared_ptr<CCreatureCard> selected;
	std::vector<std::shared_ptr<CCreatureCard>> cards;

	std::shared_ptr<CSlider> slider;
	std::shared_ptr<CButton> maxButton;
	std::shared_ptr<CButton> buyButton;
	std::shared_ptr<CButton> cancelButton;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> availableValue;
	std::shared_ptr<CLabel> toRecruitValue;
	std::shared_ptr<CLabel> availableTitle;
	std::shared_ptr<CLabel> toRecruitTitle;
	std::shared_ptr<CreatureCostBox> costPerTroopValue;
	std::shared_ptr<CreatureCostBox> totalCostValue;

	void select(std::shared_ptr<CCreatureCard> card);
	void buy();
	void sliderMoved(int to);

	void showAll(Canvas & to) override;
public:
	const CGDwelling * const dwelling;
	CRecruitmentWindow(const CGDwelling * Dwelling, int Level, const CArmedInstance * Dst, const std::function<void(CreatureID,int)> & Recruit, const std::function<void()> & onClose, int y_offset = 0);
	void availableCreaturesChanged();
	void close() override;
};

/// Split window where creatures can be split up into two single unit stacks
class CSplitWindow : public CWindowObject
{
	std::function<void(int, int)> callback;
	int leftAmount;
	int rightAmount;

	int leftMin;
	int rightMin;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CSlider> slider;
	std::shared_ptr<CCreaturePic> animLeft;
	std::shared_ptr<CCreaturePic> animRight;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CTextInput> leftInput;
	std::shared_ptr<CTextInput> rightInput;

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
	CSplitWindow(const CCreature * creature, std::function<void(int, int)> callback, int leftMin, int rightMin, int leftAmount, int rightAmount);
};

/// Raised up level window where you can select one out of two skills
class CLevelWindow : public CWindowObject
{
	std::shared_ptr<CHeroArea> portrait;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CLabel> mainTitle;
	std::shared_ptr<CLabel> levelTitle;
	std::shared_ptr<CAnimImage> skillIcon;
	std::shared_ptr<CLabel> skillValue;

	std::shared_ptr<CComponentBox> box; //skills to select
	std::function<void(ui32)> cb;

	int skillViewOffset = 0;
	std::shared_ptr<CButton> buttonLeft;
	std::shared_ptr<CButton> buttonRight;

	std::vector<SecondarySkill> skills;
	std::vector<SecondarySkill> sortedSkills;
	const CGHeroInstance * hero;

	void selectionChanged(unsigned to);
	void createSkillBox();

public:
	CLevelWindow(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, std::function<void(ui32)> callback);

	void close() override;
};

/// Town portal, castle gate window
class CObjectListWindow : public CWindowObject
{
	class CItem : public CIntObject
	{
		CObjectListWindow * parent;
		std::shared_ptr<CLabel> text;
		std::shared_ptr<CPicture> border;
		std::shared_ptr<CPicture> icon;
	public:
		const size_t index;
		CItem(CObjectListWindow * parent, size_t id, std::string text);

		void select(bool on);
		void clickPressed(const Point & cursorPosition) override;
		void clickDouble(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;
	};

	std::function<void(int)> onSelect;//called when OK button is pressed, returns id of selected item.
	std::shared_ptr<CIntObject> titleWidget;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> descr;
	std::vector<std::shared_ptr<IImage>> images;

	std::shared_ptr<CListBox> list;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CButton> exit;

	std::shared_ptr<CTextInput> searchBox;
	std::shared_ptr<TransparentFilledRectangle> searchBoxRectangle;
	std::shared_ptr<CLabel> searchBoxDescription;

	std::vector< std::pair<int, std::string> > items; //all items present in list
	std::vector< std::pair<int, std::string> > itemsVisible; //visible items present in list

	void init(std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, bool searchBoxEnabled, bool blue);
	void trimTextIfTooWide(std::string & text, bool preserveCountSuffix) const; // trim item's text to fit within window's width
	void itemsSearchCallback(const std::string & text);
	void exitPressed();
public:
	size_t selected;//index of currently selected item

	std::function<void()> onExit;//optional exit callback
	std::function<void(int)> onPopup;//optional popup callback
	std::function<void(int)> onClicked;//optional if clicked on item callback

	/// Callback will be called when OK button is pressed, returns id of selected item. initState = initially selected item
	/// Image can be nullptr
	///item names will be taken from map objects
	CObjectListWindow(const std::vector<int> &_items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection = 0, std::vector<std::shared_ptr<IImage>> images = {}, bool searchBoxEnabled = false, bool blue = false);
	CObjectListWindow(const std::vector<std::string> &_items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection = 0, std::vector<std::shared_ptr<IImage>> images = {}, bool searchBoxEnabled = false, bool blue = false);

	std::shared_ptr<CIntObject> genItem(size_t index);
	void elementSelected();//call callback and close this window
	void changeSelection(size_t which);
	void keyPressed(EShortcut key) override;
};

class CTavernWindow : public CStatusbarWindow
{
	std::function<void()> onWindowClosed;

public:
	class HeroPortrait : public CIntObject
	{
	public:
		std::string hoverName;
		std::string description; // "XXX is a level Y ZZZ with N artifacts"
		const CGHeroInstance * h;

		std::function<void()> onChoose;

		void clickPressed(const Point & cursorPosition) override;
		void clickDouble(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;
		void hover (bool on) override;
		HeroPortrait(int & sel, int id, int x, int y, const CGHeroInstance * H, std::function<void()> OnChoose = nullptr);

	private:
		int *_sel;
		const int _id;

		std::shared_ptr<CAnimImage> portrait;
	};

	class HeroSelector : public CWindowObject
	{
	public:
		std::shared_ptr<CFilledTexture> background;
		std::shared_ptr<CSlider> slider;

		const int MAX_LINES = 18;
		const int ELEM_PER_LINES = 16;

		HeroSelector(std::map<HeroTypeID, CGHeroInstance*> InviteableHeroes, std::function<void(CGHeroInstance*)> OnChoose);

	private:
		std::map<HeroTypeID, CGHeroInstance*> inviteableHeroes;
		std::function<void(CGHeroInstance*)> onChoose;

		std::vector<std::shared_ptr<CAnimImage>> portraits;
		std::vector<std::shared_ptr<LRClickableArea>> portraitAreas;

		void recreate();
		void sliderMove(int slidPos);
	};

	//recruitable heroes
	std::shared_ptr<HeroPortrait> h1;
	std::shared_ptr<HeroPortrait> h2; //recruitable heroes

	int selected;//0 (left) or 1 (right)
	int oldSelected;//0 (left) or 1 (right)

	std::shared_ptr<CButton> thiefGuild;
	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CButton> recruit;

	const CGObjectInstance * tavernObj;

	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> cost;
	std::shared_ptr<CLabel> heroesForHire;
	std::shared_ptr<CTextBox> heroDescription;
	std::shared_ptr<VideoWidget> videoPlayer;

	std::shared_ptr<CTextBox> rumor;
	
	std::shared_ptr<CLabel> inviteHero;
	std::shared_ptr<CAnimImage> inviteHeroImage;
	std::shared_ptr<LRClickableArea> inviteHeroImageArea;
	std::map<HeroTypeID, CGHeroInstance*> inviteableHeroes;
	CGHeroInstance* heroToInvite;
	void addInvite();

	CTavernWindow(const CGObjectInstance * TavernObj, const std::function<void()> & onWindowClosed);

	void close() override;
	void recruitb();
	void thievesguildb();
	void show(Canvas & to) override;
};

/// Here you can buy ships
class CShipyardWindow : public CStatusbarWindow
{
	std::shared_ptr<CPicture> bgWater;
	std::shared_ptr<CShowableAnim> bgShip;

	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> costLabel;

	std::shared_ptr<CAnimImage> woodPic;
	std::shared_ptr<CAnimImage> goldPic;
	std::shared_ptr<CLabel> woodCost;
	std::shared_ptr<CLabel> goldCost;

	std::shared_ptr<CButton> build;
	std::shared_ptr<CButton> quit;

public:
	CShipyardWindow(const TResources & cost, int state, BoatId boatType, const std::function<void()> & onBuy);
};

/// Creature transformer window
class CTransformerWindow : public CStatusbarWindow, public IGarrisonHolder
{
	class CItem : public CIntObject
	{
	public:
		int id;//position of creature in hero army
		bool left;//position of the item
		int size; //size of creature stack
		CTransformerWindow * parent;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CLabel> count;

		void move();
		void clickPressed(const Point & cursorPosition) override;
		void update();
		CItem(CTransformerWindow * parent, int size, int id);
	};

	const CArmedInstance * army;//object with army for transforming (hero or town)
	const CGHeroInstance * hero;//only if we have hero in town
	const IMarket * market;//market, town garrison is used if hero == nullptr

	std::shared_ptr<CLabel> titleLeft;
	std::shared_ptr<CLabel> titleRight;
	std::shared_ptr<CTextBox> helpLeft;
	std::shared_ptr<CTextBox> helpRight;

	std::vector<std::shared_ptr<CItem>> items;

	std::shared_ptr<CButton> all;
	std::shared_ptr<CButton> convert;
	std::shared_ptr<CButton> cancel;

	std::function<void()> onWindowClosed;
public:

	void makeDeal();
	void addAll();
	void close() override;
	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;
	CTransformerWindow(const IMarket * _market, const CGHeroInstance * _hero, const std::function<void()> & onWindowClosed);
};

class CUniversityWindow final : public CStatusbarWindow, public IMarketHolder
{
	class CItem final : public CIntObject
	{
		std::shared_ptr<CSecSkillPlace> skill;
		std::shared_ptr<CPicture> topBar;
		std::shared_ptr<CPicture> bottomBar;
		std::shared_ptr<CLabel> name;
		std::shared_ptr<CLabel> level;
	public:
		SecondarySkill ID;//id of selected skill
		CUniversityWindow * parent;

		void update();
		CItem(CUniversityWindow * _parent, int _ID, int X, int Y);
	};

	const CGHeroInstance * hero;
	const IMarket * market;

	std::vector<std::shared_ptr<CItem>> items;

	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CIntObject> titlePic;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CTextBox> clerkSpeech;

	std::function<void()> onWindowClosed;

public:
	CUniversityWindow(const CGHeroInstance * _hero, BuildingID building, const IMarket * _market, const std::function<void()> & onWindowClosed);

	void makeDeal(SecondarySkill skill);
	void close() override;

	// IMarketHolder impl
	void updateSecondarySkills() override;
};

/// Confirmation window for University
class CUnivConfirmWindow final : public CStatusbarWindow
{
	std::shared_ptr<CTextBox> clerkSpeech;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CLabel> level;
	std::shared_ptr<CAnimImage> icon;

	CUniversityWindow * owner;
	std::shared_ptr<CButton> confirm;
	std::shared_ptr<CButton> cancel;

	std::shared_ptr<CAnimImage> costIcon;
	std::shared_ptr<CLabel> cost;

	void makeDeal(SecondarySkill skill);

public:
	CUnivConfirmWindow(CUniversityWindow * PARENT, SecondarySkill SKILL, bool available);
};

/// Garrison window where you can take creatures out of the hero to place it on the garrison
class CGarrisonWindow : public CWindowObject, public IGarrisonHolder
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> banner;
	std::shared_ptr<CAnimImage> portrait;

	std::shared_ptr<CGarrisonInt> garr;

public:
	std::shared_ptr<CButton> quit;

	CGarrisonWindow(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits);

	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;
};

/// Hill fort is the building where you can upgrade units
class CHillFortWindow : public CStatusbarWindow, public IGarrisonHolder
{
private:

	enum class State { UNAFFORDABLE, ALREADY_UPGRADED, MAKE_UPGRADE, EMPTY, UNAVAILABLE };
	static constexpr std::size_t slotsCount = 7;
	//todo: configurable resource support
	static constexpr std::size_t resCount = 7;

	const CGObjectInstance * fort;
	const CGHeroInstance * hero;

	std::shared_ptr<CLabel> title;
	std::shared_ptr<CHeroArea> heroPic;

	std::array<std::shared_ptr<CAnimImage>, resCount> totalIcons;
	std::array<std::shared_ptr<CLabel>, resCount> totalLabels;

	std::array<std::shared_ptr<CButton>, slotsCount> upgrade;//upgrade single creature
	std::array<State, slotsCount + 1> currState;//current state of slot - to avoid calls to getState or updating buttons

	//there is a place for only 2 resources per slot
	std::array< std::array<std::shared_ptr<CAnimImage>, 2>, slotsCount> slotIcons;
	std::array< std::array<std::shared_ptr<CLabel>, 2>, slotsCount> slotLabels;

	std::shared_ptr<CButton> upgradeAll;
	std::shared_ptr<CButton> quit;

	std::shared_ptr<CGarrisonInt> garr;

	std::string getDefForSlot(SlotID slot);
	std::string getTextForSlot(SlotID slot);

	void makeDeal(SlotID slot);//-1 for upgrading all creatures
	State getState(SlotID slot);
public:
	CHillFortWindow(const CGHeroInstance * visitor, const CGObjectInstance * object);
	void updateGarrisons() override;//update buttons after garrison changes
	bool holdsGarrison(const CArmedInstance * army) override;
};

class CThievesGuildWindow : public CStatusbarWindow
{
	const CGObjectInstance * owner;

	std::shared_ptr<CButton> exitb;
	std::shared_ptr<CMinorResDataBar> resdatabar;

	std::vector<std::shared_ptr<CLabel>> rowHeaders;
	std::vector<std::shared_ptr<CAnimImage>> columnBackgrounds;
	std::vector<std::shared_ptr<CLabel>> columnHeaders;
	std::vector<std::shared_ptr<CAnimImage>> columnHeaderIcons;
	std::vector<std::shared_ptr<CAnimImage>> cells;

	std::vector<std::shared_ptr<CPicture>> banners;
	std::vector<std::shared_ptr<CAnimImage>> bestHeroes;
	std::vector<std::shared_ptr<CLabel>> primSkillHeaders;
	std::vector<std::shared_ptr<LRClickableArea>> primSkillHeadersArea;
	std::vector<std::shared_ptr<CLabel>> primSkillValues;
	std::vector<std::shared_ptr<CAnimImage>> bestCreatures;
	std::vector<std::shared_ptr<CLabel>> personalities;
public:
	CThievesGuildWindow(const CGObjectInstance * _owner);
};

class VideoWindow : public CWindowObject, public IVideoHolder
{
	std::shared_ptr<VideoWidgetOnce> videoPlayer;
	std::shared_ptr<CFilledTexture> backgroundAroundWindow;
	std::shared_ptr<GraphicalPrimitiveCanvas> blackBackground;
	bool showBackground;

	std::function<void(bool)> closeCb;

	void onVideoPlaybackFinished() override;
	void exit(bool skipped);
public:
	VideoWindow(const VideoPath & video, const ImagePath & rim, bool showBackground, float scaleFactor, const std::function<void(bool)> & closeCb);

	void clickPressed(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void notFocusedClick() override;
	void showAll(Canvas & to) override;
};
