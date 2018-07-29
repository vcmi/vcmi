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

#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"

class CButton;
class CBuilding;
class CCastleBuildings;
class CCreaturePic;
class CGStatusBar;
class CGTownInstance;
class CLabel;
class CMinorResDataBar;
class CPicture;
class CResDataBar;
class CSpell;
class CTextBox;
class CTownList;
struct CStructure;
class CGHeroInstance;
class CGarrisonInt;
class CCreature;
class CComponent;
class CComponentBox;

/// Building "button"
class CBuildingRect : public CShowableAnim
{
	std::string getSubtitle();
public:
	/// returns building associated with this structure
	const CBuilding * getBuilding();

	CCastleBuildings * parent;
	const CGTownInstance * town;
	const CStructure* str;
	SDL_Surface* border;
	SDL_Surface* area;

	ui32 stateCounter;//For building construction - current stage in animation

	CBuildingRect(CCastleBuildings * Par, const CGTownInstance *Town, const CStructure *Str);
	~CBuildingRect();
	bool operator<(const CBuildingRect & p2) const;
	void hover(bool on) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
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
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
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
	void enterFountain(BuildingID building);//Rampart's fountains
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

	void buildingClicked(BuildingID building);
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
	CCreaInfo(Point position, const CGTownInstance * Town, int Level, bool compact=false, bool showAvailable=false);

	void update();
	void hover(bool on) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
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
	void clickRight(tribool down, bool previousState) override;
};

/// Class which manages the castle window
class CCastleInterface : public CWindowObject, public CGarrisonHolder
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> income;
	std::shared_ptr<CAnimImage> icon;

	std::shared_ptr<CPicture> panel;
	std::shared_ptr<CResDataBar> resdatabar;
	std::shared_ptr<CGStatusBar> statusbar;

	std::shared_ptr<CTownInfo> hall;
	std::shared_ptr<CTownInfo> fort;

	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> split;
	std::shared_ptr<CButton> fastArmyPurhase;

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
	void keyPressed(const SDL_KeyboardEvent & key) override;
	void close();
	void addBuilding(BuildingID bid);
	void removeBuilding(BuildingID bid);
	void recreateIcons();
};

/// Hall window where you can build things
class CHallInterface : public CWindowObject
{
	class CBuildingBox : public CIntObject
	{
		const CGTownInstance * town;
		const CBuilding * building;

		ui32 state;//Buildings::EBuildStructure enum

		std::shared_ptr<CAnimImage> header;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CAnimImage> mark;
		std::shared_ptr<CLabel> name;
	public:
		CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building);
		void hover(bool on) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
	};
	const CGTownInstance * town;

	std::vector<std::vector<std::shared_ptr<CBuildingBox>>> boxes;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CMinorResDataBar> resdatabar;
	std::shared_ptr<CGStatusBar> statusbar;
	std::shared_ptr<CButton> exit;

public:
	CHallInterface(const CGTownInstance * Town);
};

///  Window where you can decide to buy a building or not
class CBuildWindow: public CWindowObject
{
	const CGTownInstance * town;
	const CBuilding * building;

	std::shared_ptr<CAnimImage> icon;
	std::shared_ptr<CGStatusBar> statusbar;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<CTextBox> stateText;
	std::shared_ptr<CComponentBox> cost;

	std::shared_ptr<CButton> buy;
	std::shared_ptr<CButton> cancel;

	std::string getTextForState(int state);
	void buyFunc();
public:
	CBuildWindow(const CGTownInstance *Town, const CBuilding * building, int State, bool rightClick);
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
class CFortScreen : public CWindowObject
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

		void creaturesChanged();
		void hover(bool on) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
	};
	std::shared_ptr<CLabel> title;
	std::vector<std::shared_ptr<RecruitArea>> recAreas;
	std::shared_ptr<CMinorResDataBar> resdatabar;
	std::shared_ptr<CGStatusBar> statusbar;
	std::shared_ptr<CButton> exit;

	std::string getBgName(const CGTownInstance * town);

public:
	CFortScreen(const CGTownInstance * town);

	void creaturesChanged();
};

/// The mage guild screen where you can see which spells you have
class CMageGuildScreen : public CWindowObject
{
	class Scroll : public CIntObject
	{
		const CSpell * spell;
		std::shared_ptr<CAnimImage> image;

	public:
		Scroll(Point position, const CSpell *Spell);
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;
	};
	std::shared_ptr<CPicture> window;
	std::shared_ptr<CButton> exit;
	std::vector<std::shared_ptr<Scroll>> spells;
	std::vector<std::shared_ptr<CAnimImage>> emptyScrolls;

	std::shared_ptr<CMinorResDataBar> resdatabar;
	std::shared_ptr<CGStatusBar> statusbar;

public:
	CMageGuildScreen(CCastleInterface * owner,std::string image);
};

/// The blacksmith window where you can buy available in town war machine
class CBlacksmithDialog : public CWindowObject
{
	std::shared_ptr<CButton> buy;
	std::shared_ptr<CButton> cancel;
	std::shared_ptr<CPicture> animBG;
	std::shared_ptr<CCreatureAnim> anim;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> costIcon;
	std::shared_ptr<CLabel> costText;
	std::shared_ptr<CLabel> costValue;
	std::shared_ptr<CGStatusBar> statusbar;

public:
	CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid);
};
