#ifndef CADVENTUREMAPINTERFACE_H
#define CADVENTUREMAPINTERFACE_H

#include "SDL.h"
#include "CDefHandler.h"
#include "SDL_Extensions.h"
#include "CGameInterface.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include <boost/logic/tribool.hpp>
#define CGI (CGameInfo::mainObj)
using namespace boost::logic;
using namespace CSDL_Ext;
class AdventureMapButton 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public CButtonBase
{
public:
	std::string name; //for status bar 
	std::string helpBox; //for right-click help
	char key; //key shortcut
	void (CAdvMapInt::*function)(); //function in CAdvMapInt called when this button is pressed, different for each button

	void clickRight (tribool down);
	void clickLeft (tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but don't deletes)

	AdventureMapButton(); //c-tor
	AdventureMapButton( std::string Name, std::string HelpBox, void(CAdvMapInt::*Function)(), int x, int y, std::string defName, bool activ=false );//c-tor

};
/*****************************/
class CList 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public virtual CIntObject
{
	SDL_Surface * bg;
	//arrow up, arrow down
	int posw, posh; //position width/height

	void clickLeft(tribool down);
	void activate(); 
	void deactivate();
	virtual void select(int which)=0;
};
class CHeroList 
	: public CList
{
	void select(int which);
	void clickRight(tribool down);
};
class CTownList 
	: public CList
{
	void select(int which);
	void clickRight(tribool down);
};
class CResourceBar
	:public ClickableR, public CIntObject
{
	SDL_Surface * bg;
	void clickRight(tribool down);
	void refresh();
};
class CDataBar
	:public ClickableR, public CIntObject
{
	SDL_Surface * bg;
	void clickRight(tribool down);
	void refresh();
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
	: public ClickableL, public ClickableR, public Hoverable, public CIntObject
{
public:
	SDL_Surface * radar; //radar.def
	SDL_Surface * terrainMap;
	SDL_Surface * undTerrainMap; //underground

	//TODO flagged buildings

	bool underground; 

};
class CTerrainRect
	:  public ClickableL, public ClickableR, public Hoverable, public CIntObject, public KeyInterested
{
public:
	void activate(); 
	void deactivate();
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover(bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void show();
};
/*****************************/
class CAdvMapInt //adventure map interface
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
	bool updateScreen ;
	unsigned char anim, animValHitCount; //animation frame



	SDL_Surface * bg;
	AdventureMapButton kingOverview,//- kingdom overview
		undeground,//- underground switch
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

	void show(); //shows and activates adv. map interface
	void update(); //redraws terrain


};
#endif //CADVENTUREMAPINTERFACE_H