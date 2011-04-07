#ifndef __CADVMAPINTERFACE_H__
#define __CADVMAPINTERFACE_H__
#include <typeinfo>
#include "../global.h"
#include "SDL.h"
#include <map>
#include "AdventureMapButton.h"
#include "GUIClasses.h"
class CDefHandler;
class CCallback;
struct CGPath;
class CAdvMapInt;
class CGHeroInstance;
class CGTownInstance;
class CHeroWindow;
class CSpell;
class IShipyard;

/*****************************/

/*
 * CAdcmapInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Adventure options dialogue where you can view the world, dig, play the replay of the last turn,...
class CAdventureOptions : public CIntObject
{
public:
	CPicture *bg;
	AdventureMapButton *exit, *viewWorld, *puzzle, *dig, *scenInfo, *replay;

	CAdventureOptions();
	~CAdventureOptions();
	static void showScenarioInfo();
};
	 
/// Minimap which is displayed at the right upper corner of adventure map
class CMinimap : public CIntObject
{
public:
	SDL_Surface * temps;
	std::map<int,SDL_Color> colors;
	std::map<int,SDL_Color> colorsBlocked;
	std::vector<SDL_Surface *> map, FoW, flObjs; //one bitmap for each level (terrain, Fog of War, flaggable objects)
	std::string statusbarTxt, rcText;

	CMinimap(bool draw=true);
	~CMinimap();
	void show(SDL_Surface * to);
	void draw(SDL_Surface * to);
	void redraw(int level=-1);// (level==-1) => redraw all levels
	void initMap(int level=-1);// (level==-1) => redraw all levels
	void initFoW(int level=-1);// (level==-1) => redraw all levels
	void initFlaggableObjs(int level=-1);// (level==-1) => redraw all levels

	void updateRadar();

	void clickRight(tribool down, bool previousState);
	void clickLeft(tribool down, bool previousState);
	void hover (bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but don't deletes)
	void hideTile(const int3 &pos); //puts FoW
	void showTile(const int3 &pos); //removes FoW
	void showVisibleTiles(int level=-1);// (level==-1) => redraw all levels
};

/// Holds information about which tiles of the terrain are shown/not shown at the screen
class CTerrainRect
	:  public CIntObject
{
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)

	CTerrainRect();
	~CTerrainRect();
	CGPath * currentPath;
	void activate();
	void deactivate();
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void show(SDL_Surface * to);
	void showPath(const SDL_Rect * extRect, SDL_Surface * to);
	int3 whichTileIsIt(const int & x, const int & y); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
};

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar
	: public CIntObject
{
public:
	SDL_Surface * bg;
	std::vector<std::pair<int,int> > txtpos;
	std::string datetext;

	void clickRight(tribool down, bool previousState);
	void activate();
	void deactivate();
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw(SDL_Surface * to);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
};

/// Info box which shows next week/day information, hold the current date
class CInfoBar : public CIntObject
{
	CDefHandler *day, *week1, *week2, *week3, *week4;
	SComponent * current;
	int pom;
	SDL_Surface *selInfoWin; //info box for selection
	CDefHandler * getAnim(int mode);
public:
	int mode;
	const CGHeroInstance * curSel;

	CInfoBar();
	~CInfoBar();
	void newDay(int Day); //start showing new day/week animation
	void showComp(SComponent * comp, int time=5000);
	void tick();
	void showAll(SDL_Surface * to); // if specific==0 function draws info about selected hero/town
	void blitAnim(int mode);//0 - day, 1 - week

	void show(SDL_Surface * to);
	void activate();
	void deactivate();
	void updateSelection(const CGObjectInstance *obj);
};

/// That's a huge class which handles general adventure map actions and 
/// shows the right menu(questlog, spellbook, end turn,..) from where you 
/// can get to the towns and heroes. 
class CAdvMapInt : public CIntObject
{
public:
	CAdvMapInt();
	~CAdvMapInt();

	int3 position; //top left corner of visible map part
	int player;


	enum{LEFT=1, RIGHT=2, UP=4, DOWN=8};
	ui8 scrollingDir; //uses enum: LEFT RIGHT, UP, DOWN

	enum{NA, INGAME, WAITING} state;

	bool updateScreen, updateMinimap ;
	unsigned char anim, animValHitCount; //animation frame
	unsigned char heroAnim, heroAnimValHitCount; //animation frame

	SDL_Surface * bg;
	std::vector<CDefHandler *> gems;
	CMinimap minimap;
	CStatusBar statusbar;

	AdventureMapButton kingOverview,//- kingdom overview
		underground,//- underground switch
		questlog,//- questlog
		sleepWake, //- sleep/wake hero
		moveHero, //- move hero
		spellbook,//- spellbook
		advOptions, //- adventure options
		sysOptions,//- system options
		nextHero, //- next hero
		endTurn;//- end turn

	CTerrainRect terrain; //visible terrain
	CResDataBar resdatabar;
	CHeroList heroList;
	CTownList townList;
	CInfoBar infoBar;

	const CSpell *spellBeingCasted; //NULL if none

	const CArmedInstance *selection; //currently selected town/hero

	//functions bound to buttons
	void fshowOverview();
	void fswitchLevel();
	void fshowQuestlog();
	void fsleepWake();
	void fmoveHero();
	void fshowSpellbok();
	void fadventureOPtions();
	void fsystemOptions();
	void fnextHero();
	void fendTurn();

	void activate();
	void deactivate();

	void show(SDL_Surface * to); //redraws terrain
	void showAll(SDL_Surface * to); //shows and activates adv. map interface

	void select(const CArmedInstance *sel, bool centerView = true);
	void selectionChanged();
	void centerOn(int3 on);
	void centerOn(const CGObjectInstance *obj);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down);
	void keyPressed(const SDL_KeyboardEvent & key);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	bool isActive();

	void setPlayer(int Player);
	void startHotSeatWait(int Player);
	void startTurn();
	void tileLClicked(const int3 &mp);
	void tileHovered(const int3 &tile);
	void tileRClicked(const int3 &mp);
	void enterCastingMode(const CSpell * sp);
	void leaveCastingMode(bool cast = false, int3 dest = int3(-1, -1, -1));
	const CGHeroInstance * curHero() const;
	const CGTownInstance * curTown() const;
	const IShipyard * ourInaccessibleShipyard(const CGObjectInstance *obj) const; //checks if obj is our ashipyard and cursor is 0,0 -> returns shipyard or NULL else
};

extern CAdvMapInt *adventureInt;

#endif // __CADVMAPINTERFACE_H__
