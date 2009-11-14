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

class CAdventureOptions : public CIntObject
{
public:
	CPicture *bg;
	AdventureMapButton *exit, *viewWorld, *puzzle, *dig, *scenInfo, *replay;

	CAdventureOptions();
	~CAdventureOptions();
	static void showScenarioInfo();
	static void showPuzzleMap();
};
	 

class CMinimap : public CIntObject
{
public:
	SDL_Surface * radar;
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
class CTerrainRect
	:  public CIntObject
{
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)

	CDefHandler * arrows;
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
};
class CInfoBar : public CIntObject
{
public:
	CDefHandler *day, *week1, *week2, *week3, *week4;
	SComponent * current;
	int mode;
	int pom;

	CInfoBar();
	~CInfoBar();
	void newDay(int Day); //start showing new day/week animation
	void showComp(SComponent * comp, int time=5000);
	void tick();
	void draw(SDL_Surface * to, const CGObjectInstance * specific=NULL); // if specific==0 function draws info about selected hero/town
	void blitAnim(int mode);//0 - day, 1 - week
	CDefHandler * getAnim(int mode);
	void show(SDL_Surface * to);
	void activate();
	void deactivate();
};
/*****************************/
class CAdvMapInt : public CIntObject //adventure map interface
{
public:
	CAdvMapInt(int Player);
	~CAdvMapInt();

	int3 position; //top left corner of visible map part
	int player, active;


	enum{LEFT=1, RIGHT=2, UP=4, DOWN=8};
	ui8 scrollingDir; //uses enum: LEFT RIGHT, UP, DOWN

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

	CHeroWindow * heroWindow;

	const CArmedInstance *selection; //currently selected town/hero
	std::map<const CGHeroInstance *, CGPath> paths; //maps hero => selected path in adventure map

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

	void select(const CArmedInstance *sel);
	void selectionChanged();
	void centerOn(int3 on);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down, CIntObject * client);
	void keyPressed(const SDL_KeyboardEvent & key);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
};
#endif // __CADVMAPINTERFACE_H__
