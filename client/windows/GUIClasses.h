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
#include "../lib/GameConstants.h"
#include "../lib/ResourceSet.h"
#include "../lib/CConfigHandler.h"
#include "../lib/int3.h"
#include "../widgets/CWindowWithArtifacts.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGDwelling;
class IMarket;

VCMI_LIB_NAMESPACE_END

class CreatureCostBox;
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
class CVolumeSlider;
class CGStatusBar;
class CTextBox;
class CResDataBar;
class CHeroWithMaybePickedArtifact;

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

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void showAll(SDL_Surface * to) override;
	};

	std::function<void(CreatureID,int)> onRecruit; //void (int ID, int amount) <-- call to recruit creatures

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

	void showAll(SDL_Surface * to) override;
public:
	const CGDwelling * const dwelling;
	CRecruitmentWindow(const CGDwelling * Dwelling, int Level, const CArmedInstance * Dst, const std::function<void(CreatureID,int)> & Recruit, int y_offset = 0);
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
	std::shared_ptr<CAnimImage> portrait;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CLabel> mainTitle;
	std::shared_ptr<CLabel> levelTitle;
	std::shared_ptr<CAnimImage> skillIcon;
	std::shared_ptr<CLabel> skillValue;

	std::shared_ptr<CComponentBox> box; //skills to select
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
		CObjectListWindow * parent;
		std::shared_ptr<CLabel> text;
		std::shared_ptr<CPicture> border;
	public:
		const size_t index;
		CItem(CObjectListWindow * parent, size_t id, std::string text);

		void select(bool on);
		void clickLeft(tribool down, bool previousState) override;
		void onDoubleClick() override;
	};

	std::function<void(int)> onSelect;//called when OK button is pressed, returns id of selected item.
	std::shared_ptr<CIntObject> titleWidget;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> descr;

	std::shared_ptr<CListBox> list;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CButton> exit;

	std::vector< std::pair<int, std::string> > items;//all items present in list

	void init(std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr);
	void exitPressed();
public:
	size_t selected;//index of currently selected item

	std::function<void()> onExit;//optional exit callback

	/// Callback will be called when OK button is pressed, returns id of selected item. initState = initially selected item
	/// Image can be nullptr
	///item names will be taken from map objects
	CObjectListWindow(const std::vector<int> &_items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection = 0);
	CObjectListWindow(const std::vector<std::string> &_items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection = 0);

	std::shared_ptr<CIntObject> genItem(size_t index);
	void elementSelected();//call callback and close this window
	void changeSelection(size_t which);
	void keyPressed(const SDL_Keycode & key) override;
};

class CTavernWindow : public CStatusbarWindow
{
public:
	class HeroPortrait : public CIntObject
	{
	public:
		std::string hoverName;
		std::string description; // "XXX is a level Y ZZZ with N artifacts"
		const CGHeroInstance * h;

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover (bool on) override;
		HeroPortrait(int & sel, int id, int x, int y, const CGHeroInstance * H);

	private:
		int *_sel;
		const int _id;

		std::shared_ptr<CAnimImage> portrait;
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
	std::shared_ptr<CTextBox> rumor;

	CTavernWindow(const CGObjectInstance * TavernObj);
	~CTavernWindow();

	void recruitb();
	void thievesguildb();
	void show(SDL_Surface * to) override;
};

class CCallback;
class CExchangeWindow;

struct HeroArtifact
{
	const CGHeroInstance * hero;
	const CArtifactInstance * artifact;
	ArtifactPosition artPosition;

	HeroArtifact(const CGHeroInstance * hero, const CArtifactInstance * artifact, ArtifactPosition artPosition)
		:hero(hero), artifact(artifact), artPosition(artPosition)
	{
	}
};

class CExchangeController
{
private:
	const CGHeroInstance * left;
	const CGHeroInstance * right;
	std::shared_ptr<CCallback> cb;
	CExchangeWindow * view;

public:
	CExchangeController(CExchangeWindow * view, ObjectInstanceID hero1, ObjectInstanceID hero2);
	std::function<void()> onMoveArmyToRight();
	std::function<void()> onSwapArmy();
	std::function<void()> onMoveArmyToLeft();
	std::function<void()> onSwapArtifacts();
	std::function<void()> onMoveArtifactsToLeft();
	std::function<void()> onMoveArtifactsToRight();
	std::function<void()> onMoveStackToLeft(SlotID slotID);
	std::function<void()> onMoveStackToRight(SlotID slotID);

private:
	void moveArmy(bool leftToRight);
	void moveArtifacts(bool leftToRight);
	void moveArtifact(const CGHeroInstance * source, const CGHeroInstance * target, ArtifactPosition srcPosition);
	void moveStack(const CGHeroInstance * source, const CGHeroInstance * target, SlotID sourceSlot);
};

class CExchangeWindow : public CStatusbarWindow, public CGarrisonHolder, public CWindowWithArtifacts
{
	std::array<std::shared_ptr<CHeroWithMaybePickedArtifact>, 2> herosWArt;

	std::array<std::shared_ptr<CLabel>, 2> titles;
	std::vector<std::shared_ptr<CAnimImage>> primSkillImages;//shared for both heroes
	std::array<std::vector<std::shared_ptr<CLabel>>, 2> primSkillValues;
	std::array<std::vector<std::shared_ptr<CAnimImage>>, 2> secSkillIcons;
	std::array<std::shared_ptr<CAnimImage>, 2> specImages;
	std::array<std::shared_ptr<CAnimImage>, 2> expImages;
	std::array<std::shared_ptr<CLabel>, 2> expValues;
	std::array<std::shared_ptr<CAnimImage>, 2> manaImages;
	std::array<std::shared_ptr<CLabel>, 2> manaValues;
	std::array<std::shared_ptr<CAnimImage>, 2> portraits;

	std::vector<std::shared_ptr<LRClickableAreaWTextComp>> primSkillAreas;
	std::array<std::vector<std::shared_ptr<LRClickableAreaWTextComp>>, 2> secSkillAreas;

	std::array<std::shared_ptr<CHeroArea>, 2> heroAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> specialtyAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> experienceAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> spellPointsAreas;

	std::array<std::shared_ptr<MoraleLuckBox>, 2> morale;
	std::array<std::shared_ptr<MoraleLuckBox>, 2> luck;

	std::shared_ptr<CButton> quit;
	std::array<std::shared_ptr<CButton>, 2> questlogButton;

	std::shared_ptr<CGarrisonInt> garr;
	std::shared_ptr<CButton> moveAllGarrButtonLeft;
	std::shared_ptr<CButton> echangeGarrButton;
	std::shared_ptr<CButton> moveAllGarrButtonRight;
	std::shared_ptr<CButton> moveArtifactsButtonLeft;
	std::shared_ptr<CButton> echangeArtifactsButton;
	std::shared_ptr<CButton> moveArtifactsButtonRight;
	std::vector<std::shared_ptr<CButton>> moveStackLeftButtons;
	std::vector<std::shared_ptr<CButton>> moveStackRightButtons;
	CExchangeController controller;

public:
	std::array<const CGHeroInstance *, 2> heroInst;
	std::array<std::shared_ptr<CArtifactsOfHeroMain>, 2> artifs;

	void updateGarrisons() override;

	void questlog(int whichHero); //questlog button callback; whichHero: 0 - left, 1 - right

	void updateWidgets();

	const CGarrisonSlot * getSelectedSlotID() const;

	CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID);
};

/// Here you can buy ships
class CShipyardWindow : public CStatusbarWindow
{
	std::shared_ptr<CPicture> bgWater;
	std::shared_ptr<CAnimImage> bgShip;

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
class CTransformerWindow : public CStatusbarWindow, public CGarrisonHolder
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
		void clickLeft(tribool down, bool previousState) override;
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
public:

	void makeDeal();
	void addAll();
	void updateGarrisons() override;
	CTransformerWindow(const IMarket * _market, const CGHeroInstance * _hero);
};

class CUniversityWindow : public CStatusbarWindow
{
	class CItem : public CIntObject
	{
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CAnimImage> topBar;
		std::shared_ptr<CAnimImage> bottomBar;
		std::shared_ptr<CLabel> name;
		std::shared_ptr<CLabel> level;
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

	const CGHeroInstance * hero;
	const IMarket * market;

	std::shared_ptr<CAnimation> bars;

	std::vector<std::shared_ptr<CItem>> items;

	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CIntObject> titlePic;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CTextBox> clerkSpeech;

public:
	CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market);

	void makeDeal(int skill);
};

/// Confirmation window for University
class CUnivConfirmWindow : public CStatusbarWindow
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

	void makeDeal(int skill);

public:
	CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available);
};

/// Garrison window where you can take creatures out of the hero to place it on the garrison
class CGarrisonWindow : public CWindowObject, public CGarrisonHolder
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> banner;
	std::shared_ptr<CAnimImage> portrait;

	std::shared_ptr<CGarrisonInt> garr;

public:
	std::shared_ptr<CButton> quit;

	CGarrisonWindow(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits);

	void updateGarrisons() override;
};

/// Hill fort is the building where you can upgrade units
class CHillFortWindow : public CStatusbarWindow, public CGarrisonHolder
{
private:
	static const int slotsCount = 7;
	//todo: mithril support
	static const int resCount = 7;

	const CGObjectInstance * fort;
	const CGHeroInstance * hero;

	std::shared_ptr<CLabel> title;
	std::shared_ptr<CHeroArea> heroPic;

	std::array<std::shared_ptr<CAnimImage>, resCount> totalIcons;
	std::array<std::shared_ptr<CLabel>, resCount> totalLabels;

	std::array<std::shared_ptr<CButton>, slotsCount> upgrade;//upgrade single creature
	std::array<int, slotsCount + 1> currState;//current state of slot - to avoid calls to getState or updating buttons

	//there is a place for only 2 resources per slot
	std::array< std::array<std::shared_ptr<CAnimImage>, 2>, slotsCount> slotIcons;
	std::array< std::array<std::shared_ptr<CLabel>, 2>, slotsCount> slotLabels;

	std::shared_ptr<CButton> upgradeAll;
	std::shared_ptr<CButton> quit;

	std::shared_ptr<CGarrisonInt> garr;

	std::string getDefForSlot(SlotID slot);
	std::string getTextForSlot(SlotID slot);

	void makeDeal(SlotID slot);//-1 for upgrading all creatures
	int getState(SlotID slot); //-1 = no creature 0=can't upgrade, 1=upgraded, 2=can upgrade
public:
	CHillFortWindow(const CGHeroInstance * visitor, const CGObjectInstance * object);
	void updateGarrisons() override;//update buttons after garrison changes
};

class CThievesGuildWindow : public CStatusbarWindow
{
	const CGObjectInstance * owner;

	std::shared_ptr<CButton> exitb;
	std::shared_ptr<CMinorResDataBar> resdatabar;

	std::vector<std::shared_ptr<CLabel>> rowHeaders;
	std::vector<std::shared_ptr<CAnimImage>> columnBackgrounds;
	std::vector<std::shared_ptr<CLabel>> columnHeaders;
	std::vector<std::shared_ptr<CAnimImage>> cells;

	std::vector<std::shared_ptr<CPicture>> banners;
	std::vector<std::shared_ptr<CAnimImage>> bestHeroes;
	std::vector<std::shared_ptr<CTextBox>> primSkillHeaders;
	std::vector<std::shared_ptr<CLabel>> primSkillValues;
	std::vector<std::shared_ptr<CAnimImage>> bestCreatures;
	std::vector<std::shared_ptr<CLabel>> personalities;
public:
	CThievesGuildWindow(const CGObjectInstance * _owner);
};

