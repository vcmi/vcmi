#ifndef __CCASTLEINTERFACE_H__
#define __CCASTLEINTERFACE_H__



#include "../global.h"
#include <SDL.h>
#include "GUIBase.h"
#include "../hch/CMusicBase.h"
//#include "boost/tuple/tuple.hpp"
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
class CCreaturePic;
class CMinorResDataBar;

/*
 * CCastleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CBuildingRect : public Hoverable, public MotionInterested, public ClickableL, public ClickableR//, public TimeInterested
{
public:
	bool moi; //motion interested is active
	int offset, max; //first and last animation frame
	Structure* str;
	CDefHandler* def;
	SDL_Surface* border;
	SDL_Surface* area;
	CBuildingRect(Structure *Str); //c-tor
	~CBuildingRect(); //d-tor
	void activate();
	void deactivate();
	bool operator<(const CBuildingRect & p2) const;
	void hover(bool on);
	void clickLeft (boost::logic::tribool down);
	void clickRight (boost::logic::tribool down);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
};
class CHeroGSlot : public ClickableL, public ClickableR, public Hoverable
{
public:
	CCastleInterface *owner;
	const CGHeroInstance *hero;
	int upg; //0 - up garrison, 1 - down garrison
	bool highlight; //indicates id the slot is highlighted

	void hover (bool on);
	void clickRight (boost::logic::tribool down);
	void clickLeft(boost::logic::tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h,CCastleInterface * Owner); //c-tor
	~CHeroGSlot(); //d-tor
};

class CCastleInterface : public CWindowWithGarrison
{
public:
	SDL_Rect pos; //why not inherit this member from CIntObject ?
	bool showing; //indicates if interface is active
	CBuildingRect * hBuild; //highlighted building
	SDL_Surface * townInt;
	SDL_Surface * cityBg;
	const CGTownInstance * town;
	CStatusBar * statusbar;
	CResDataBar *resdatabar;
	unsigned char animval, count;

	CDefEssential *bars, //0 - yellow, 1 - green, 2 - red, 3 - gray
		*status; //0 - already, 1 - can't, 2 - lack of resources
	CDefHandler *hall,*fort;
	CDefEssential* bicons; //150x70 buildings imgs
	CTownList * townlist;

	CHeroGSlot hslotup, hslotdown;
	AdventureMapButton *exit;
	AdventureMapButton *split;

	musicBase::musicID musicID;

	std::vector<CBuildingRect*> buildings; //building id, building def, structure struct, border, filling

	CCastleInterface(const CGTownInstance * Town); //c-tor
	~CCastleInterface(); //d-tor
	void townChange();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void buildingClicked(int building);
	void enterMageGuild();
	CRecruitmentWindow * showRecruitmentWindow(int building);
	void enterHall();
	void close();
	void splitF();
	void activate();
	void deactivate();
	void addBuilding(int bid);
	void removeBuilding(int bid);
	void recreateBuildings();
};
class CHallInterface : public IShowActivable
{
public:
	CMinorResDataBar * resdatabar;
	SDL_Rect pos;

	class CBuildingBox : public Hoverable, public ClickableL, public ClickableR
	{
	public:
		int BID;
		int state;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
		//(-1) - forbidden in this town, 0 - possible, 1 - lack of res, 2 - requirements/buildings per turn limit, (3) - already exists
		void hover(bool on);
		void clickLeft (boost::logic::tribool down);
		void clickRight (boost::logic::tribool down);
		void show(SDL_Surface * to);
		void activate();
		void deactivate();
		CBuildingBox(int id); //c-tor
		CBuildingBox(int id, int x, int y); //c-tor
		~CBuildingBox(); //d-tor
	};

	class CBuildWindow: public IShowActivable, public ClickableR
	{
	public:
		int tid, bid, state; //town id, building id, state
		bool mode; // 0 - normal (with buttons), 1 - r-click popup
		SDL_Surface * bitmap; //main window bitmap, with blitted res/text, without buttons/subtitle in "statusbar"
		AdventureMapButton *buy, *cancel;

		void activate();
		void deactivate();
		std::string getTextForState(int state);
		void clickRight (boost::logic::tribool down);
		void show(SDL_Surface * to);
		void Buy();
		void close();
		CBuildWindow(int Tid, int Bid, int State, bool Mode); //c-tor
		~CBuildWindow(); //d-tor
	};

	std::vector< std::vector<CBuildingBox*> >boxes;

	AdventureMapButton *exit;

	SDL_Surface * bg; //background


	CHallInterface(CCastleInterface * owner); //c-tor
	~CHallInterface(); //d-tor
	void close();
	void show(SDL_Surface * to);
	void activate();
	void deactivate();
};

class CFortScreen : public IShowActivable, public CIntObject
{
	class RecArea : public ClickableL, public ClickableR
	{
	public:
		int bid;
		RecArea(int BID):bid(BID){}; //c-tor
		void clickLeft (boost::logic::tribool down);
		void clickRight (boost::logic::tribool down);
		void activate();
		void deactivate();
	};
public:
	CMinorResDataBar * resdatabar;
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
	void activate();
	void deactivate();
};

class CMageGuildScreen : public IShowActivable, public CIntObject
{
public:	
	class Scroll : public ClickableL, public Hoverable, public ClickableR
	{
	public:
		CSpell *spell;

		Scroll(CSpell *Spell):spell(Spell){};
		void clickLeft (boost::logic::tribool down);
		void clickRight (boost::logic::tribool down);
		void hover(bool on);
		void activate();
		void deactivate();
	};
	std::vector<std::vector<SDL_Rect> > positions;

	SDL_Surface *bg;
	CDefEssential *scrolls, *scrolls2;
	AdventureMapButton *exit;
	std::vector<Scroll> spells;
	CMinorResDataBar * resdatabar;


	CMageGuildScreen(CCastleInterface * owner); //c-tor
	~CMageGuildScreen(); //d-tor
	void close();
	void show(SDL_Surface * to);
	void activate();
	void deactivate();
};

class CBlacksmithDialog : public IShowActivable, public CIntObject
{
public:
	AdventureMapButton *buy, *cancel;
	SDL_Surface *bmp; //background

	CBlacksmithDialog(bool possible, int creMachineID, int aid, int hid); //c-tor
	~CBlacksmithDialog(); //d-tor
	void close();
	void show(SDL_Surface * to);
	void activate();
	void deactivate();
};

#endif // __CCASTLEINTERFACE_H__
