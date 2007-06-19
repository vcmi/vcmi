#include "SDL.h"
#include "CSemiDefHandler.h"
#include "CSemiLodHandler.h"
#include "CPreGameTextHandler.h" 
#include "CMessage.h"
#include "map.h"
class CPreGame;
extern CPreGame * CPG;
class ScenSel
{
public:
	SDL_Surface * background, *scenInf, *scenList, *randMap, *options ;
	Button<> bScens, bOptions, bRandom, bBegin, bBack;
	IntSelBut<>	bEasy, bNormal, bHard, bExpert, bImpossible;
	Button<> * pressed;
	CPoinGroup<> * difficulty;
	std::vector<Mapa> maps;
	void genScenList();
	int selectedDiff;
	~ScenSel(){delete difficulty;};
} ;
class CPreGame
{
public:	
	CSemiLodHandler * slh ;
	std::vector<Button<> *> btns;
	CPreGameTextHandler * preth ;
	SDL_Rect * currentMessage;	
	SDL_Surface * behindCurMes;
	CSemiDefHandler *ok, *cancel;
	enum EState { //where are we?
		mainMenu, newGame, loadGame, ScenarioList
	} state;
	struct menuItems { 
		SDL_Surface * background;
		CSemiDefHandler *newGame, *loadGame, *highScores,*credits, *quit;
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
	void highlightButton(int which, int on);
	void showCenBox (std::string data);
	void showAskBox (std::string data, void(*f1)(),void(*f2)());
	void hideBox ();
};