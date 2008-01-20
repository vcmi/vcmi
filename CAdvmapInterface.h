#ifndef CADVENTUREMAPINTERFACE_H
#define CADVENTUREMAPINTERFACE_H
#include <typeinfo>
#include "global.h"
#include "SDL.h"
#include "CPlayerInterface.h"
#include <map>
class CDefHandler;
class CCallback;
class CTownInstance;
class CPath; 
class CAdvMapInt;
class CGHeroInstance;
class CGTownInstance;
class CHeroWindow;
template <typename T=CAdvMapInt>
class AdventureMapButton 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public CButtonBase
{
public:
	std::string name; //for status bar 
	std::string helpBox; //for right-click help
	char key; //key shortcut
	T* owner;
	void (T::*function)(); //function in CAdvMapInt called when this button is pressed, different for each button

	void clickRight (tribool down);
	void clickLeft (tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but doesn't delete)

	AdventureMapButton(); //c-tor
	AdventureMapButton( std::string Name, std::string HelpBox, void(T::*Function)(), int x, int y, std::string defName, T* Owner, bool activ=false,  std::vector<std::string> * add = NULL );//c-tor
};
/*****************************/
class CList 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public virtual CIntObject, public MotionInterested
{
public:
	SDL_Surface * bg;
	CDefHandler *arrup, *arrdo;
	SDL_Surface *empty, *selection; 
	SDL_Rect arrupp, arrdop; //positions of arrows
	int posw, posh; //position width/height
	int selected, //id of selected position, <0 if none
		from; 
	tribool pressed; //true=up; false=down; indeterminate=none

	void clickLeft(tribool down);
	void activate(); 
	void deactivate();
	virtual void mouseMoved (SDL_MouseMotionEvent & sEvent)=0;
	virtual void genList()=0;
	virtual void select(int which)=0;
	virtual void draw()=0;
};
class CHeroList 
	: public CList
{
public:
	CDefHandler *mobile, *mana;
	std::vector<std::pair<const CGHeroInstance*, CPath *> > items;
	int posmobx, posporx, posmanx, posmoby, pospory, posmany;

	CHeroList();
	void genList();
	void select(int which);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void updateHList();
	void updateMove(const CGHeroInstance* which); //draws move points bar
	void redrawAllOne(int which);
	void draw();
	void init();
};
class CTownList 
	: public CList
{
public: 
	std::vector<const CGTownInstance*> items;
	int posporx,pospory;

	CTownList();
	void genList();
	void select(int which);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void draw();
};
class CStatusBar
	: public CIntObject
{
public:
	SDL_Surface * bg; //background
	int middlex, middley; //middle of statusbar
	std::string current; //text currently printed

	CStatusBar(int x, int y); //c-tor
	~CStatusBar(); //d-tor
	void print(std::string text); //prints text and refreshes statusbar
	void clear();//clears statusbar and refreshes
	void show(); //shows statusbar (with current text)
};
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
class CAdvMapInt : public IActivable //adventure map interface
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
	AdventureMapButton<> kingOverview,//- kingdom overview
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

	void show(); //shows and activates adv. map interface
	void hide(); //deactivates advmap interface
	void update(); //redraws terrain

	void centerOn(int3 on);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down, CIntObject * client);


};
#endif //CADVENTUREMAPINTERFACE_H