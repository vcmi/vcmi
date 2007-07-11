#ifndef CPREGAME_H
#define CPREGAME_H

#include "SDL.h"
#include "CSemiDefHandler.h"
#include "CSemiLodHandler.h"
#include "CPreGameTextHandler.h" 
#include "CMessage.h"
#include "map.h"
#include "CMusicHandler.h"
class CPreGame;
extern CPreGame * CPG;
struct RanSel
{
	Button<> horcpl[9], horcte[9], conpl[9], conte[8], water[4], monster[4], //last is random
			size[4], twoLevel, showRand;
	CGroup<> *Ghorcpl, *Ghorcte, *Gconpl, *Gconte, *Gwater, *Gmonster, *Gsize;
};
class MapSel
{
public:
	bool showed;
	SDL_Surface * bg;
	int selected;
	CDefHandler * Dtypes, * Dvic; 
	CDefHandler *Dsizes, * Dloss;
	std::vector<Mapa*> scenList;
	std::vector<SDL_Surface*> scenImgs;
	int current;
	std::vector<CMapInfo> ourMaps;
	IntBut<> small, medium, large, xlarge, all;
	Button<> nrplayer, mapsize, type, name, viccon, loscon;
	Slider<>  *slid, *descslid;
	int sizeFilter;
	int whichWL(int nr);
	int countWL();
	void draw();
	void init();
	std::string gdiff(std::string ss);
	void printMaps(int from,int to=18, int at=0, bool abs=false);
	void select(int which);
	void moveByOne(bool up);
	void printSelectedInfo();
	MapSel();
	~MapSel();
};
class ScenSel
{
public:
	bool listShowed;
	RanSel ransel;
	MapSel mapsel;
	SDL_Surface * background, *scenInf, *scenList, *randMap, *options ;
	Button<> bScens, bOptions, bRandom, bBegin, bBack;
	IntSelBut<>	bEasy, bNormal, bHard, bExpert, bImpossible;
	Button<> * pressed;
	CPoinGroup<> * difficulty;
	std::vector<Mapa> maps;
	int selectedDiff;
	void initRanSel();
	void showRanSel();
	void hideRanSel();
	void genScenList();
	~ScenSel(){delete difficulty;};
} ;
class CPreGame
{
public:	
	bool run;
	std::vector<Slider<> *> interested;
	CMusicHandler * mush;
	CSemiLodHandler * slh ;
	std::vector<Button<> *> btns;
	CPreGameTextHandler * preth ;
	SDL_Rect * currentMessage;	
	SDL_Surface * behindCurMes;
	CDefHandler *ok, *cancel;
	enum EState { //where are we?
		mainMenu, newGame, loadGame, ScenarioList
	} state;
	struct menuItems { 
		SDL_Surface * background, *bgAd;
		CDefHandler *newGame, *loadGame, *highScores,*credits, *quit;
		SDL_Rect lNewGame, lLoadGame, lHighScores, lCredits, lQuit;
		ttt fNewGame, fLoadGame, fHighScores, fCredits, fQuit;
		int highlighted;//0=none; 1=new game; 2=load game; 3=high score; 4=credits; 5=quit
	} * ourMainMenu, * ourNewMenu;
	ScenSel * ourScenSel;
	std::string map; //selected map
	std::vector<CSemiLodHandler *> handledLods; 
	CPreGame(); //c-tor
	std::string buttonText(int which);
	menuItems * currentItems();
	void(CPreGame::*handleOther)(SDL_Event&);
	void scenHandleEv(SDL_Event& sEvent);
	void begin(){run=false;};
	void quitAskBox();
	void quit(){exit(0);};  
	void initScenSel(); 
	void showScenSel();  
	void showScenList(); 
	void showOptions();  
	void initNewMenu(); 
	void showNewMenu();  
	void showMainMenu();  
	void runLoop(); // runs mainloop of PreGame
	void initMainMenu(); //loads components for main menu
	void highlightButton(int which, int on); //highlights one from 5 main menu buttons
	void showCenBox (std::string data); //
	void showAskBox (std::string data, void(*f1)(),void(*f2)());
	void hideBox ();
	void printMapsFrom(int from);
};

#endif //CPREGAME_H
