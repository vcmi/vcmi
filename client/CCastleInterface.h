#ifndef __CCASTLEINTERFACE_H__
#define __CCASTLEINTERFACE_H__

#include "../global.h"
#include "CAnimation.h"
#include "GUIBase.h"

class AdventureMapButton;
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
class CStatusBar;
class CTextBox;
class CTownList;
struct Structure;

/*
 * CCastleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Building "button"
class CBuildingRect : public CShowableAnim
{
public:
	CCastleBuildings * parent;
	const CGTownInstance * town;
	const Structure* str;
	SDL_Surface* border;
	SDL_Surface* area;
	
	unsigned int stateCounter;//For building construction - current stage in animation
	
	CBuildingRect(CCastleBuildings * Par, const CGTownInstance *Town, const Structure *Str); //c-tor
	~CBuildingRect(); //d-tor
	bool operator<(const CBuildingRect & p2) const;
	void hover(bool on);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void show(SDL_Surface *to);
	void showAll(SDL_Surface *to);
};

/// Dwelling info box - right-click screen for dwellings
class CDwellingInfoBox : public CIntObject
{
	CPicture *background;
	CLabel *title;
	CCreaturePic *animation;
	CLabel *available;
	CLabel *costPerTroop;
	
	std::vector<CPicture *> resPicture;
	std::vector<CLabel *> resAmount;
public:
	CDwellingInfoBox(int centerX, int centerY, const CGTownInstance *Town, int level);
	void clickRight(tribool down, bool previousState);
};

/// Hero army slot
class CHeroGSlot : public CIntObject
{
public:
	CCastleInterface *owner;
	const CGHeroInstance *hero;
	int upg; //0 - up garrison, 1 - down garrison
	bool highlight; //indicates id the slot is highlighted

	void setHighlight(bool on);

	void hover (bool on);
	void clickLeft(tribool down, bool previousState);
	void deactivate();
	void showAll(SDL_Surface * to);
	CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h,CCastleInterface * Owner); //c-tor
	~CHeroGSlot(); //d-tor
};

/// Class for town screen management (town background and structures)
class CCastleBuildings : public CIntObject
{
	struct AnimRule
	{
		int townID, buildID;
		int toCheck;
		size_t firstA, lastA;
		size_t firstB, lastB;
	};
	
	CPicture *background;
	//List of buildings for each group
	std::map< int, std::vector<const Structure*> > groups;
	//Vector with all blittable buildings
	std::vector<CBuildingRect*> buildings;

	const CGTownInstance * town;

	const CGHeroInstance* getHero();//Select hero for buildings usage
	void checkRules();//Check animation rules (special anims for Shipyard and Mana Vortex)

	void enterBlacksmith(int ArtifactID);//support for blacksmith + ballista yard
	void enterBuilding(int building);//for buildings with simple description + pic left-click messages
	void enterCastleGate();
	void enterFountain(int building);//Rampart's fountains
	void enterMagesGuild();
	void enterTownHall();

	void openMagesGuild();
	void openTownHall();

public:
	CBuildingRect * selectedBuilding;

	CCastleBuildings(const CGTownInstance* town);
	~CCastleBuildings();

	void enterDwelling(int level);

	void buildingClicked(int building);
	void addBuilding(int building);
	void removeBuilding(int building);//FIXME: not tested!!!
	
	void show(SDL_Surface *to);
	void showAll(SDL_Surface *to);
};

/// Creature info window
class CCreaInfo : public CIntObject
{
	const CGTownInstance * town;
	const CCreature *creature;
	int level;
	
	CAnimImage *picture;
	CLabel * label;

	int AddToString(std::string from, std::string & to, int numb);
	
public:
	CCreaInfo(int posX, int posY, const CGTownInstance *Town, int Level);
	
	void hover(bool on);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
};

/// Town hall and fort icons for town screen
class CTownInfo : public CIntObject
{
	const CGTownInstance *town;
	const CBuilding *building;
	CAnimImage *picture;
public:
	//if (townHall) hall-capital else fort - castle
	CTownInfo(int posX, int posY, const CGTownInstance* town, bool townHall);
	
	void hover(bool on);
	void clickRight(tribool down, bool previousState);
};

/// Class which manages the castle window
class CCastleInterface : public CWindowWithGarrison
{
	CLabel *title;
	CLabel *income;
	CAnimImage *icon;

	CPicture * panel;
	CResDataBar *resdatabar;
	CGStatusBar * statusbar;

	CTownInfo *hall, *fort;
	CTownList * townlist;

	AdventureMapButton *exit;
	AdventureMapButton *split;

	std::vector<CCreaInfo*> creainfo;//small icons of creatures (bottom-left corner);

public:
	//TODO: remove - currently used only in dialog messages
	CDefEssential* bicons; //150x70 buildings imgs

	//TODO: move to private
	const CGTownInstance * town;
	CHeroGSlot *heroSlotUp, *heroSlotDown;
	CCastleBuildings *builds;

	CCastleInterface(const CGTownInstance * Town, int listPos = 1); //c-tor
	~CCastleInterface();

	void castleTeleport(int where);
	void townChange();
	void keyPressed(const SDL_KeyboardEvent & key);
	void splitClicked(); //for hero meeting (splitting stacks is handled by garrison int)
	void showAll(SDL_Surface *to);
	void close();
	void addBuilding(int bid);
	void removeBuilding(int bid);
	void recreateIcons();
};

/// Hall window where you can build things
class CHallInterface : public CIntObject
{
	/// Building box from town hall (building icon + subtitle)
	class CBuildingBox : public CIntObject
	{
		const CGTownInstance * town;
		const CBuilding * building;

		unsigned int state;//Buildings::EBuildStructure enum

		CAnimImage * picture;
		CAnimImage * panel;
		CAnimImage * icon;
		CLabel * label;

	public:
		CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building);
		void hover(bool on);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
	};
	const CGTownInstance * town;
	
	std::vector< std::vector<CBuildingBox*> >boxes;
	CPicture *background;
	CLabel *title;
	CGStatusBar *statusBar;
	CMinorResDataBar * resdatabar;
	AdventureMapButton *exit;

public:
	CHallInterface(const CGTownInstance * Town); //c-tor
	void close();
};

///  Window where you can decide to buy a building or not
class CBuildWindow: public CIntObject
{
	const CGTownInstance *town;
	const CBuilding *building;
	int state; //state - same as CHallInterface::CBuildingBox::state
	bool mode; // 0 - normal (with buttons), 1 - r-click popup

	CPicture *background;
	CAnimImage *buildingPic;
	AdventureMapButton *buy;
	AdventureMapButton *cancel;

	CLabel * title;
	CTextBox * buildingDescr;
	CTextBox * buildingState;
	CGStatusBar *statusBar;

	std::vector<CPicture *> resPicture;
	std::vector<CLabel *> resAmount;

	std::string getTextForState(int state);
	void buyFunc();
	void close();

public:
	void clickRight(tribool down, bool previousState);
	CBuildWindow(const CGTownInstance *Town, const CBuilding * building, int State, bool Mode); //c-tor
	~CBuildWindow(); //d-tor
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
	void hover(bool on);
};

/// The fort screen where you can afford units
class CFortScreen : public CIntObject
{
	class RecruitArea : public CIntObject
	{
		const CGTownInstance *town;
		int level;

		std::string hoverText;
		CLabel * creatureName;
		CLabel * dwellingName;
		CLabel * availableCount;

		std::vector<LabeledValue*> values;
		CPicture *icons;
		CAnimImage * buildingPic;
		CCreaturePic *creatureAnim;

	public:
		RecruitArea(int posX, int posY, const CGTownInstance *town, int buildingID, int level);
		
		void creaturesChanged();
		void hover(bool on);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
	};
	
	CPicture *background;
	CLabel *title;
	std::vector<RecruitArea*> recAreas;
	CMinorResDataBar * resdatabar;
	CGStatusBar *statusBar;
	AdventureMapButton *exit;

public:
	CFortScreen(const CGTownInstance * town); //c-tor

	void creaturesChanged();
	void close();
};

/// The mage guild screen where you can see which spells you have
class CMageGuildScreen : public CIntObject
{
	class Scroll : public CIntObject
	{
		const CSpell *spell;
		CAnimImage *image;

	public:
		Scroll(Point position, const CSpell *Spell);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover(bool on);
	};
	CPicture *background;
	CPicture *window;
	AdventureMapButton *exit;
	std::vector<Scroll *> spells;
	CMinorResDataBar * resdatabar;
	CGStatusBar *statusBar;

	void close();

public:
	CMageGuildScreen(CCastleInterface * owner);
};

/// The blacksmith window where you can buy available in town war machine
class CBlacksmithDialog : public CIntObject
{
	AdventureMapButton *buy, *cancel;
	CPicture *background;
	CPicture *animBG;
	CCreatureAnim * anim;
	CPicture * gold;
	CLabel * title;
	CLabel * costText;
	CLabel * costValue;
	CGStatusBar *statusBar;

	void close();

public:
	CBlacksmithDialog(bool possible, int creMachineID, int aid, int hid);
};

#endif // __CCASTLEINTERFACE_H__
