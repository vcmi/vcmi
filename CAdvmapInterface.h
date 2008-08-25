#ifndef CADVENTUREMAPINTERFACE_H
#define CADVENTUREMAPINTERFACE_H
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
	//TODO flagged buildings
	std::string statusbarTxt, rcText;

	CMinimap(bool draw=true);
	void draw();
	void redraw(int level=-1);// (level==-1) => redraw all levels
	void updateRadar();

	void clickRight (tribool down);
	void clickLeft (tribool down);
	void hover (bool on);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but don't deletes)
	void hideTile(int3 pos); //puts FoW
	void showTile(int3 pos); //removes FoW
};
class CTerrainRect
	:  public ClickableL, public ClickableR, public Hoverable, public virtual CIntObject, public KeyInterested,
	public MotionInterested
{
public:
	int tilesw, tilesh;
	int3 curHoveredTile;

	CDefHandler * arrows;
	CTerrainRect();
	CPath * currentPath;
	void activate();
	void deactivate();
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover(bool on);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void keyPressed (SDL_KeyboardEvent & key);
	void show();
	void showPath();
	int3 whichTileIsIt(int x, int y); //x,y are cursor position
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
class CAdvMapInt : public CMainInterface //adventure map interface
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

	struct CurrentSelection
	{
		int type; //0 - hero, 1 - town
		const void* selected;
		CurrentSelection(); //ctor
	} selection;

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

	void selectionChanged();
	void centerOn(int3 on);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down, CIntObject * client);


};
#endif //CADVENTUREMAPINTERFACE_H
