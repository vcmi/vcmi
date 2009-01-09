#ifndef __CADVMAPINTERFACE_H__
#define __CADVMAPINTERFACE_H__
#include <typeinfo>
#include "global.h"
#include "SDL.h"
#include "CPlayerInterface.h"
#include <map>
#include "AdventureMapButton.h"
class CDefHandler;
class CCallback;
struct CPath;
class CAdvMapInt;
class CGHeroInstance;
class CGTownInstance;
class CHeroWindow;
/*****************************/
class CMinimap
	: public ClickableL, public ClickableR, public Hoverable, public MotionInterested, public virtual CIntObject
{
public:
	SDL_Surface * radar;
	SDL_Surface * temps;
	std::map<int,SDL_Color> colors;
	std::map<int,SDL_Color> colorsBlocked;
	std::vector<SDL_Surface *> map, FoW; //one bitmap for each level (terrain, Fog of War)
	std::string statusbarTxt, rcText;

	CMinimap(bool draw=true);
	void draw();
	void redraw(int level=-1);// (level==-1) => redraw all levels
	void updateRadar();

	void clickRight (tribool down);
	void clickLeft (tribool down);
	void hover (bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but don't deletes)
	void hideTile(const int3 &pos); //puts FoW
	void showTile(const int3 &pos); //removes FoW
};
class CTerrainRect
	:  public ClickableL, public ClickableR, public Hoverable, public MotionInterested
{
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)

	CDefHandler * arrows;
	CTerrainRect();
	CPath * currentPath;
	void activate();
	void deactivate();
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover(bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void show();
	void showPath(const SDL_Rect * extRect);
	int3 whichTileIsIt(const int & x, const int & y); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
};
class CResDataBar
	:public ClickableR, public virtual CIntObject
{
public:
	SDL_Surface * bg;
	std::vector<std::pair<int,int> > txtpos;
	std::string datetext;

	void clickRight (tribool down);
	void activate();
	void deactivate();
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw();
};
class CInfoBar
	:public virtual CIntObject, public TimeInterested
{
public:
	CDefHandler *day, *week1, *week2, *week3, *week4;
	SComponent * current;
	int mode;
	int pom;

	CInfoBar();
	~CInfoBar();
	void newDay(int Day);
	void showComp(SComponent * comp, int time=5000);
	void tick();
	void draw(const CGObjectInstance * specific=NULL); // if specific==0 function draws info about selected hero/town
	void blitAnim(int mode);//0 - day, 1 - week
	CDefHandler * getAnim(int mode);
};
/*****************************/
class CAdvMapInt : public CMainInterface, public KeyInterested //adventure map interface
{
public:
	CAdvMapInt(int Player);
	~CAdvMapInt();

	int3 position; //top left corner of visible map part
	int player;

	std::vector<CDefHandler *> gems;

	bool scrollingLeft ;
	bool scrollingRight ;
	bool scrollingUp ;
	bool scrollingDown ;
	bool updateScreen, updateMinimap ;
	unsigned char anim, animValHitCount; //animation frame
	unsigned char heroAnim, heroAnimValHitCount; //animation frame

	CMinimap minimap;


	SDL_Surface * bg;
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
	//CHeroList herolist;

	CTerrainRect terrain; //visible terrain

	CStatusBar statusbar;
	CResDataBar resdatabar;

	CHeroList heroList;
	CTownList townList;
	CInfoBar infoBar;

	CHeroWindow * heroWindow;

	const CArmedInstance *selection;

	//fuctions binded to buttons
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

	void show(SDL_Surface * to=NULL); //shows and activates adv. map interface
	void hide(); //deactivates advmap interface
	void update(); //redraws terrain

	void select(const CArmedInstance *sel);
	void selectionChanged();
	void centerOn(int3 on);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down, CIntObject * client);
	void keyPressed(const SDL_KeyboardEvent & key);


};
#endif // __CADVMAPINTERFACE_H__
