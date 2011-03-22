#ifndef __CCASTLEINTERFACE_H__
#define __CCASTLEINTERFACE_H__

#include "../global.h"
#include "CAnimation.h"
#include "GUIBase.h"

class CGTownInstance;
class CTownHandler;
class CHallInterface;
struct Structure;
class CSpell;
class AdventureMapButton;
class CResDataBar;
class CStatusBar;
class CTownList;
class CRecruitmentWindow;
class CTransformerWindow;
class CPicture;
class CCreaturePic;
class CMinorResDataBar;
class CCastleBuildings;

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
	const Structure* str;
	SDL_Surface* border;
	SDL_Surface* area;
	
	unsigned int stateCounter;//For building construction - current stage in animation
	
	CBuildingRect(CCastleBuildings * Par, const Structure *Str); //c-tor
	~CBuildingRect(); //d-tor
	bool operator<(const CBuildingRect & p2) const;
	void hover(bool on);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void show(SDL_Surface *to);
	void showAll(SDL_Surface *to);
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
	void show(SDL_Surface * to);
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

/// Huge class which manages the castle window
class CCastleInterface : public CWindowWithGarrison
{
	/// Creature info window
	class CCreaInfo : public CIntObject
	{
	public:
		int crid,level;
		CCreaInfo(int CRID, int LVL); //c-tor
		~CCreaInfo();//d-tor
		int AddToString(std::string from, std::string & to, int numb);
		void hover(bool on);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void show(SDL_Surface * to);
	};
	/// Icons from town screen with castle\town hall images
	class CTownInfo : public CIntObject
	{
	public:
		int bid;//typeID
		CDefHandler * pic;
		CTownInfo(int BID); //c-tor
		~CTownInfo();//d-tor
		void hover(bool on);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void show(SDL_Surface * to);
	};

public:
	CCastleBuildings *builds;
	SDL_Surface * townInt;
	const CGTownInstance * town;
	CStatusBar * statusbar;
	CResDataBar *resdatabar;
	int winMode;//0=right-click popup, 1 = normal, 2 = town hall only, 3 = fort only;

	CDefEssential *bars, //0 - yellow, 1 - green, 2 - red, 3 - gray
		*status; //0 - already, 1 - can't, 2 - lack of resources
	CTownInfo *hall,*fort,*market;
	CDefEssential* bicons; //150x70 buildings imgs
	CTownList * townlist;

	CHeroGSlot hslotup, hslotdown;
	AdventureMapButton *exit;
	AdventureMapButton *split;

	std::vector<CCreaInfo*> creainfo;//small icons of creatures (bottom-left corner);

	CCastleInterface(const CGTownInstance * Town, int listPos = 1); //c-tor
	~CCastleInterface(); //d-tor

	void castleTeleport(int where);
	void townChange();
	void keyPressed(const SDL_KeyboardEvent & key);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void splitClicked(); //for hero meeting (splitting stacks is handled by garrison int)
	void close();
	void activate();
	void deactivate();
	void addBuilding(int bid);
	void removeBuilding(int bid);
	void recreateIcons();
};

/// Hall window where you can build things
class CHallInterface : public CIntObject
{
public:
	CMinorResDataBar * resdatabar;

	/// Building box from town hall (building icon + subtitle)
	class CBuildingBox : public CIntObject
	{
	public:
		int BID;
		int state;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - already builded today, 6 - cannot afford, 7 - build, 8 - lack of requirements
		//(-1) - forbidden in this town, 0 - possible, 1 - lack of res, 2 - requirements/buildings per turn limit, (3) - already exists
		void hover(bool on);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void show(SDL_Surface * to);
		CBuildingBox(int id); //c-tor
		CBuildingBox(int id, int x, int y); //c-tor
		~CBuildingBox(); //d-tor
	};

	///  Window where you can decide to buy a building or not
	class CBuildWindow: public CIntObject
	{
	public:
		int tid, bid, state; //town id, building id, state
		bool mode; // 0 - normal (with buttons), 1 - r-click popup
		SDL_Surface * bitmap; //main window bitmap, with blitted res/text, without buttons/subtitle in "statusbar"
		AdventureMapButton *buy, *cancel;

		void activate();
		void deactivate();
		std::string getTextForState(int state);
		void clickRight(tribool down, bool previousState);
		void show(SDL_Surface * to);
		void Buy();
		void close();
		CBuildWindow(int Tid, int Bid, int State, bool Mode); //c-tor
		~CBuildWindow(); //d-tor
	};

	std::vector< std::vector<CBuildingBox*> >boxes;

	AdventureMapButton *exit;

	SDL_Surface * bg; //background
	int bid;//building ID

	CHallInterface(CCastleInterface * owner); //c-tor
	~CHallInterface(); //d-tor
	void close();
	void showAll(SDL_Surface * to);
	void activate();
	void deactivate();
};

/// The fort screen where you can afford units
class CFortScreen : public CIntObject
{
	class RecArea : public CIntObject
	{
	public:
		int level;
		RecArea(int LEVEL):level(LEVEL){used = LCLICK | RCLICK;}; //c-tor
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
	};
public:
	CMinorResDataBar * resdatabar;
	int fortSize;
	AdventureMapButton *exit;
	SDL_Surface * bg;
	std::vector<Rect> positions;
	std::vector<RecArea*> recAreas;
	std::vector<CCreaturePic*> crePics;

	CFortScreen(CCastleInterface * owner); //c-tor

	void draw( CCastleInterface * owner, bool first);
	~CFortScreen(); //d-tor
	void close();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void activate();
	void deactivate();
};

/// The mage guild screen where you can see which spells you have
class CMageGuildScreen : public CIntObject
{
public:
	class Scroll : public CIntObject
	{
	public:
		const CSpell *spell;

		Scroll(const CSpell *Spell);
		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover(bool on);
	};
	std::vector<std::vector<SDL_Rect> > positions;

	CPicture *bg;
	AdventureMapButton *exit;
	std::vector<Scroll *> spells;
	CMinorResDataBar * resdatabar;


	CMageGuildScreen(CCastleInterface * owner); //c-tor
	~CMageGuildScreen(); //d-tor
	void close();

};

/// The blacksmith window where you can buy available in town war machine
class CBlacksmithDialog : public CIntObject
{
public:
	AdventureMapButton *buy, *cancel;
	CPicture *bmp; //background
	CPicture *animBG;
	CCreatureAnim * anim;

	CBlacksmithDialog(bool possible, int creMachineID, int aid, int hid); //c-tor
	~CBlacksmithDialog(); //d-tor
	void close();
};

#endif // __CCASTLEINTERFACE_H__
