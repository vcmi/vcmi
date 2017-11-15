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
	CLabel *title;
	CCreaturePic *animation;
	CLabel *available;
	CLabel *costPerTroop;

	std::vector<CAnimImage *> resPicture;
	std::vector<CLabel *> resAmount;
public:
	CDwellingInfoBox(int centerX, int centerY, const CGTownInstance *Town, int level);
};

class HeroSlots;
/// Hero icon slot
class CHeroGSlot : public CIntObject
{
public:
	HeroSlots *owner;
	const CGHeroInstance *hero;
	int upg; //0 - up garrison, 1 - down garrison

	CAnimImage *image;
	CAnimImage *selection; //selection border. nullptr if not selected

	void setHighlight(bool on);
	void set(const CGHeroInstance *newHero);

	void hover (bool on) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void deactivate() override;
	CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, HeroSlots * Owner);
	~CHeroGSlot();
};

/// Two hero slots that can interact with each other
class HeroSlots : public CIntObject
{
public:
	bool showEmpty;
	const CGTownInstance * town;

	CGarrisonInt *garr;
	CHeroGSlot * garrisonedHero;
	CHeroGSlot * visitingHero;

	HeroSlots(const CGTownInstance * town, Point garrPos, Point visitPos, CGarrisonInt *Garrison, bool ShowEmpty);

	void splitClicked(); //for hero meeting only (splitting stacks is handled by garrison int)
	void update();
	void swapArmies(); //exchange garrisoned and visiting heroes or move hero to\from garrison
};

/// Class for town screen management (town background and structures)
class CCastleBuildings : public CIntObject
{
	CPicture *background;
	//List of buildings and structures that can represent them
	std::map< BuildingID, std::vector<const CStructure*> > groups;
	// actual IntObject's visible on screen
	std::vector< CBuildingRect * > buildings;

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

	CCastleBuildings(const CGTownInstance* town);
	~CCastleBuildings();

	void enterDwelling(int level);
	void enterToTheQuickRecruitmentWindow();

	void buildingClicked(BuildingID building);
	void addBuilding(BuildingID building);
	void removeBuilding(BuildingID building);//FIXME: not tested!!!

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

/// Creature info window
class CCreaInfo : public CIntObject
{
	const CGTownInstance * town;
	const CCreature *creature;
	int level;
	bool showAvailable;

	CAnimImage *picture;
	CLabel * label;

	int AddToString(std::string from, std::string & to, int numb);
	std::string genGrowthText();

public:
	CCreaInfo(Point position, const CGTownInstance *Town, int Level, bool compact=false, bool showAvailable=false);

	void update();
	void hover(bool on) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
};

/// Town hall and fort icons for town screen
class CTownInfo : public CIntObject
{
	const CGTownInstance *town;
	const CBuilding *building;
public:
	CAnimImage * picture;
	//if (townHall) hall-capital else fort - castle
	CTownInfo(int posX, int posY, const CGTownInstance* town, bool townHall);

	void hover(bool on) override;
	void clickRight(tribool down, bool previousState) override;
};

/// Class which manages the castle window
class CCastleInterface : public CWindowObject, public CWindowWithGarrison
{
	CLabel *title;
	CLabel *income;
	CAnimImage *icon;

	CPicture * panel;
	CResDataBar *resdatabar;
	CGStatusBar * statusbar;

	CTownInfo *hall, *fort;

	CButton *exit;
	CButton *split;
	CButton * fastArmyPurhase;

	std::vector<CCreaInfo*> creainfo;//small icons of creatures (bottom-left corner);

public:
	CTownList * townlist;

	//TODO: move to private
	const CGTownInstance * town;
	HeroSlots *heroes;
	CCastleBuildings *builds;

	//from - previously selected castle (if any)
	CCastleInterface(const CGTownInstance * Town, const CGTownInstance * from = nullptr);
	~CCastleInterface();

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
	/// Building box from town hall (building icon + subtitle)
	class CBuildingBox : public CIntObject
	{
		const CGTownInstance * town;
		const CBuilding * building;

		ui32 state;//Buildings::EBuildStructure enum
	public:
		CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building);
		void hover(bool on) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
	};
	const CGTownInstance * town;

	std::vector< std::vector<CBuildingBox*> >boxes;
	CLabel *title;
	CGStatusBar *statusBar;
	CMinorResDataBar * resdatabar;
	CButton *exit;

public:
	CHallInterface(const CGTownInstance * Town);
};

///  Window where you can decide to buy a building or not
class CBuildWindow: public CWindowObject
{
	const CGTownInstance *town;
	const CBuilding *building;

	std::string getTextForState(int state);
	void buyFunc();
public:
	CBuildWindow(const CGTownInstance *Town, const CBuilding * building, int State, bool rightClick);
};

//Small class to display
class LabeledValue : public CIntObject
{
	std::string hoverText;
	CLabel *name;
	CLabel *value;
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
		const CGTownInstance *town;
		int level;

		std::string hoverText;
		CLabel * availableCount;

		std::vector<LabeledValue*> values;
		CPicture *icons;

		const CCreature * getMyCreature();
		const CBuilding * getMyBuilding();
	public:
		RecruitArea(int posX, int posY, const CGTownInstance *town, int level);

		void creaturesChanged();
		void hover(bool on) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
	};
	CLabel *title;
	std::vector<RecruitArea*> recAreas;
	CMinorResDataBar * resdatabar;
	CGStatusBar *statusBar;
	CButton *exit;

	std::string getBgName(const CGTownInstance *town);

public:
	CFortScreen(const CGTownInstance * town);

	void creaturesChanged();
};

/// The mage guild screen where you can see which spells you have
class CMageGuildScreen : public CWindowObject
{
	class Scroll : public CIntObject
	{
		const CSpell *spell;
		CAnimImage *image;

	public:
		Scroll(Point position, const CSpell *Spell);
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;
	};
	CPicture *window;
	CButton *exit;
	std::vector<Scroll *> spells;
	CMinorResDataBar * resdatabar;
	CGStatusBar *statusBar;

public:
	CMageGuildScreen(CCastleInterface * owner,std::string image);
};

/// The blacksmith window where you can buy available in town war machine
class CBlacksmithDialog : public CWindowObject
{
	CButton *buy, *cancel;
	CPicture *animBG;
	CCreatureAnim * anim;
	CLabel * title;
	CLabel * costText;
	CLabel * costValue;
	CGStatusBar *statusBar;

public:
	CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid);
};
