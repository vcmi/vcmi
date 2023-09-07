/*
 * CCastleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../widgets/Images.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBuilding;
class CGTownInstance;
class CSpell;
struct CStructure;
class CGHeroInstance;
class CCreature;

VCMI_LIB_NAMESPACE_END

class CButton;
class CCastleBuildings;
class CCreaturePic;
class CGStatusBar;
class CLabel;
class CMinorResDataBar;
class CPicture;
class CResDataBar;
class CTextBox;
class CTownList;
class CGarrisonInt;
class CComponent;
class CComponentBox;

/// Building "button"
class CBuildingRect : public CShowableAnim
{
	std::string getSubtitle();
public:
	enum EBuildingCreationAnimationPhases : uint32_t
	{
		BUILDING_APPEAR_TIMEPOINT = 500, //500 msec building appears: 0->100% transparency
		BUILDING_WHITE_BORDER_TIMEPOINT = 900, //400 msec border glows from white to yellow
		BUILDING_YELLOW_BORDER_TIMEPOINT = 1100, //200 msec border glows from yellow to normal (dark orange)
		BUILD_ANIMATION_FINISHED_TIMEPOINT = 2100, // 1000msec once border is back to yellow nothing happens (this stage is basically removed by HD Mod)

		BUILDING_FRAME_TIME = 150 // confirmed H3 timing: 150 ms for each building animation frame
	};

	/// returns building associated with this structure
	const CBuilding * getBuilding();

	CCastleBuildings * parent;
	const CGTownInstance * town;
	const CStructure* str;
	std::shared_ptr<IImage> border;
	std::shared_ptr<IImage> area;

	ui32 stateTimeCounter;//For building construction - current stage in animation

	CBuildingRect(CCastleBuildings * Par, const CGTownInstance *Town, const CStructure *Str);
	bool operator<(const CBuildingRect & p2) const;
	void hover(bool on) override;
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void mouseMoved (const Point & cursorPosition, const Point & lastUpdateDistance) override;
	bool receiveEvent(const Point & position, int eventType) const override;
	void tick(uint32_t msPassed) override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
};

/// Dwelling info box - right-click screen for dwellings
class CDwellingInfoBox : public CWindowObject
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CCreaturePic> animation;
	std::shared_ptr<CLabel> available;
	std::shared_ptr<CLabel> costPerTroop;

	std::vector<std::shared_ptr<CAnimImage>> resPicture;
	std::vector<std::shared_ptr<CLabel>> resAmount;
public:
	CDwellingInfoBox(int centerX, int centerY, const CGTownInstance * Town, int level);
	~CDwellingInfoBox();
};

class HeroSlots;
/// Hero icon slot
class CHeroGSlot : public CIntObject
{
	std::shared_ptr<CAnimImage> portrait;
	std::shared_ptr<CAnimImage> flag;
	std::shared_ptr<CAnimImage> selection; //selection border. nullptr if not selected

	HeroSlots * owner;
	const CGHeroInstance * hero;
	int upg; //0 - up garrison, 1 - down garrison

public:
	CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, HeroSlots * Owner);
	~CHeroGSlot();

	bool isSelected() const;

	void setHighlight(bool on);
	void set(const CGHeroInstance * newHero);

	void hover (bool on) override;
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void deactivate() override;
};

/// Two hero slots that can interact with each other
class HeroSlots : public CIntObject
{
public:
	bool showEmpty;
	const CGTownInstance * town;

	std::shared_ptr<CGarrisonInt> garr;
	std::shared_ptr<CHeroGSlot> garrisonedHero;
	std::shared_ptr<CHeroGSlot> visitingHero;

	HeroSlots(const CGTownInstance * town, Point garrPos, Point visitPos, std::shared_ptr<CGarrisonInt> Garrison, bool ShowEmpty);
	~HeroSlots();

	void splitClicked(); //for hero meeting only (splitting stacks is handled by garrison int)
	void update();
	void swapArmies(); //exchange garrisoned and visiting heroes or move hero to\from garrison
};

/// Class for town screen management (town background and structures)
class CCastleBuildings : public CIntObject
{
	std::shared_ptr<CPicture> background;
	//List of buildings and structures that can represent them
	std::map<BuildingID, std::vector<const CStructure *> > groups;
	// actual IntObject's visible on screen
	std::vector<std::shared_ptr<CBuildingRect>> buildings;

	const CGTownInstance * town;

	const CGHeroInstance* getHero();//Select hero for buildings usage

	void enterBlacksmith(ArtifactID artifactID);//support for blacksmith + ballista yard
	void enterBuilding(BuildingID building);//for buildings with simple description + pic left-click messages
	void enterCastleGate();
	void enterFountain(const BuildingID & building, BuildingSubID::EBuildingSubID subID, BuildingID upgrades);//Rampart's fountains
	void enterMagesGuild();
	void enterTownHall();

	void openMagesGuild();
	void openTownHall();

	void recreate();
public:
	CBuildingRect * selectedBuilding;

	CCastleBuildings(const CGTownInstance * town);
	~CCastleBuildings();

	void enterDwelling(int level);
	void enterToTheQuickRecruitmentWindow();

	void buildingClicked(BuildingID building, BuildingSubID::EBuildingSubID subID = BuildingSubID::NONE, BuildingID upgrades = BuildingID::NONE);
	void addBuilding(BuildingID building);
	void removeBuilding(BuildingID building);//FIXME: not tested!!!
};

/// Creature info window
class CCreaInfo : public CIntObject
{
	const CGTownInstance * town;
	const CCreature * creature;
	int level;
	bool showAvailable;

	std::shared_ptr<CAnimImage> picture;
	std::shared_ptr<CLabel> label;

	std::string genGrowthText();

public:
	CCreaInfo(Point position, const CGTownInstance * Town, int Level, bool compact=false, bool _showAvailable=false);

	void update();
	void hover(bool on) override;
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	bool getShowAvailable();
};

/// Town hall and fort icons for town screen
class CTownInfo : public CIntObject
{
	const CGTownInstance * town;
	const CBuilding * building;
public:
	std::shared_ptr<CAnimImage> picture;
	//if (townHall) hall-capital else fort - castle
	CTownInfo(int posX, int posY, const CGTownInstance * town, bool townHall);

	void hover(bool on) override;
	void showPopupWindow(const Point & cursorPosition) override;
};

/// Class which manages the castle window
class CCastleInterface : public CStatusbarWindow, public IGarrisonHolder
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> income;
	std::shared_ptr<CAnimImage> icon;

	std::shared_ptr<CPicture> panel;
	std::shared_ptr<CResDataBar> resdatabar;

	std::shared_ptr<CTownInfo> hall;
	std::shared_ptr<CTownInfo> fort;

	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> split;
	std::shared_ptr<CButton> fastArmyPurchase;

	std::vector<std::shared_ptr<CCreaInfo>> creainfo;//small icons of creatures (bottom-left corner);

public:
	std::shared_ptr<CTownList> townlist;

	//TODO: move to private
	const CGTownInstance * town;
	std::shared_ptr<HeroSlots> heroes;
	std::shared_ptr<CCastleBuildings> builds;

	std::shared_ptr<CGarrisonInt> garr;

	//from - previously selected castle (if any)
	CCastleInterface(const CGTownInstance * Town, const CGTownInstance * from = nullptr);
	~CCastleInterface();

	virtual void updateGarrisons() override;

	void castleTeleport(int where);
	void townChange();
	void keyPressed(EShortcut key) override;

	void close();
	void addBuilding(BuildingID bid);
	void removeBuilding(BuildingID bid);
	void recreateIcons();
	void creaturesChangedEventHandler();
};

/// Hall window where you can build things
class CHallInterface : public CStatusbarWindow
{
	class CBuildingBox : public CIntObject
	{
		const CGTownInstance * town;
		const CBuilding * building;

		EBuildingState state;

		std::shared_ptr<CAnimImage> header;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CAnimImage> mark;
		std::shared_ptr<CLabel> name;
	public:
		CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building);
		void hover(bool on) override;
		void clickPressed(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;
	};
	const CGTownInstance * town;

	std::vector<std::vector<std::shared_ptr<CBuildingBox>>> boxes;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CMinorResDataBar> resdatabar;
	std::shared_ptr<CButton> exit;

public:
	CHallInterface(const CGTownInstance * Town);
};

///  Window where you can decide to buy a building or not
class CBuildWindow: public CStatusbarWindow
{
	const CGTownInstance * town;
	const CBuilding * building;

	std::shared_ptr<CAnimImage> icon;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<CTextBox> stateText;
	std::shared_ptr<CComponentBox> cost;

	std::shared_ptr<CButton> buy;
	std::shared_ptr<CButton> cancel;

	std::string getTextForState(EBuildingState state);
	void buyFunc();
public:
	CBuildWindow(const CGTownInstance *Town, const CBuilding * building, EBuildingState State, bool rightClick);
};

//Small class to display
class LabeledValue : public CIntObject
{
	std::string hoverText;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CLabel> value;
	void init(std::string name, std::string descr, int min, int max);

public:
	LabeledValue(Rect size, std::string name, std::string descr, int min, int max);
	LabeledValue(Rect size, std::string name, std::string descr, int val);
	void hover(bool on) override;
};

/// The fort screen where you can afford units
class CFortScreen : public CStatusbarWindow
{
	class RecruitArea : public CIntObject
	{
		const CGTownInstance * town;
		int level;

		std::string hoverText;
		std::shared_ptr<CLabel> availableCount;

		std::vector<std::shared_ptr<LabeledValue>> values;
		std::shared_ptr<CPicture> icons;
		std::shared_ptr<CAnimImage> buildingIcon;
		std::shared_ptr<CLabel> buildingName;

		const CCreature * getMyCreature();
		const CBuilding * getMyBuilding();
	public:
		RecruitArea(int posX, int posY, const CGTownInstance *town, int level);

		void creaturesChangedEventHandler();
		void hover(bool on) override;
		void clickPressed(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;

	};
	std::shared_ptr<CLabel> title;
	std::vector<std::shared_ptr<RecruitArea>> recAreas;
	std::shared_ptr<CMinorResDataBar> resdatabar;
	std::shared_ptr<CButton> exit;

	ImagePath getBgName(const CGTownInstance * town);

public:
	CFortScreen(const CGTownInstance * town);

	void creaturesChangedEventHandler();
};

/// The mage guild screen where you can see which spells you have
class CMageGuildScreen : public CStatusbarWindow
{
	class Scroll : public CIntObject
	{
		const CSpell * spell;
		std::shared_ptr<CAnimImage> image;

	public:
		Scroll(Point position, const CSpell *Spell);
		void clickPressed(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;
		void hover(bool on) override;
	};
	std::shared_ptr<CPicture> window;
	std::shared_ptr<CButton> exit;
	std::vector<std::shared_ptr<Scroll>> spells;
	std::vector<std::shared_ptr<CAnimImage>> emptyScrolls;

	std::shared_ptr<CMinorResDataBar> resdatabar;

public:
	CMageGuildScreen(CCastleInterface * owner, const ImagePath & image);
};

/// The blacksmith window where you can buy available in town war machine
class CBlacksmithDialog : public CStatusbarWindow
{
	std::shared_ptr<CButton> buy;
	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CPicture> animBG;
	std::shared_ptr<CCreatureAnim> anim;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> costIcon;
	std::shared_ptr<CLabel> costText;
	std::shared_ptr<CLabel> costValue;

public:
	CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid);
};
