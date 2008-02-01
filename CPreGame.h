#ifndef CPREGAME_H
#define CPREGAME_H
#include "SDL.h"
#include "StartInfo.h"
#include "hch\CSemiDefHandler.h"
#include "hch\CSemiLodHandler.h"
#include "hch\CPreGameTextHandler.h" 
#include "CMessage.h"
#include "map.h"
#include "hch\CMusicHandler.h"
class CPreGame;
extern CPreGame * CPG;

typedef void(CPreGame::*ttt)();
template <class T=ttt> class CGroup;
template <class T=ttt> class CPoinGroup ;


struct HighButton
{
	int ID;
	int type;
	SDL_Rect pos;
	CDefHandler* imgs;
	int state;
	HighButton( SDL_Rect Pos, CDefHandler* Imgs, bool Sel=false, int id=-1)
		{type=0;imgs=Imgs;selectable=Sel;selected=false;state=0;pos=Pos;ID=id;highlightable=false;};
	HighButton(){state=0;}
	bool selectable, selected;
	bool highlightable, highlighted;
	virtual void show();
	virtual void press(bool down=true);
	virtual	void hover(bool on=true)=0;
	virtual void select(bool on=true)=0;
};
template <class T=ttt> struct Button: public HighButton
{
	CGroup<T> * ourGroup;
	Button( SDL_Rect Pos, T Fun,CDefHandler* Imgs, bool Sel=false, CGroup<T>* gr=NULL, int id=-1)
		:HighButton(Pos,Imgs,Sel,id),ourGroup(gr),fun(Fun){type=1;};
	Button(){ourGroup=NULL;type=1;};
	T fun;
	virtual	void hover(bool on=true);
	virtual void select(bool on=true);
};	
template <class T=ttt> struct SetrButton: public Button<T>
{
	int key, * poin;
	virtual void press(bool down=true);
	SetrButton(){type=1;selectable=selected=false;state=0;highlightable=false;}
};
template<class T=CPreGame>  class Slider
{ //
public:
	bool vertical; // false means horizontal
	SDL_Rect pos; // position
	Button<void(Slider::*)()> up, down, //or left/right
		slider; 
	int positionsAmnt, capacity;// capacity - amount of positions dispplayed at once
	int whereAreWe; // first displayed thing
	bool moving;
	void(T::*fun)(int);
	void clickDown(int x, int y, bool bzgl=true);
	void clickUp(int x, int y, bool bzgl=true);
	void mMove(int x, int y, bool bzgl=true);
	void moveUp();
	void moveDown();
	void deactivate();
	void activate();
	Slider(int x, int y, int h, int amnt, int cap, bool ver);
	void updateSlid();
	void handleIt(SDL_Event sev);

};
//template<class T=void(CPreGame::*)(int)>
template<class T=ttt>  struct IntBut: public Button<T>
{
public:
	int key;
	int * what;
	IntBut(){type=2;fun=NULL;highlightable=false;};
	void set(){*what=key;};
};
template<class T=ttt>  struct IntSelBut: public Button<T>
{
public:
	CPoinGroup<T> * ourPoinGroup;
	int key;
	IntSelBut(){};
	IntSelBut( SDL_Rect Pos, T Fun,CDefHandler* Imgs, bool Sel=false, CPoinGroup<T>* gr=NULL, int My=-1)
		: Button(Pos,Fun,Imgs,Sel,gr),key(My){ourPoinGroup=gr;};
	void select(bool on=true) {(*this).Button::select(on);ourPoinGroup->setYour(this);CPG->printRating();}
};
template <class T> class CPoinGroup :public CGroup<T>
{
public:
	int * gdzie; //where (po polsku, bo by by³o s³owo kluczowe :/)
	void setYour(IntSelBut<T> * your){*gdzie=your->key;};
};
template <class T> class CGroup
{
public:
	Button<T> * selected;
	int type; // 1=sinsel
	CGroup():selected(NULL),type(0){};
};

class PreGameTab
{
public:
	bool showed;
	virtual void init()=0;
	virtual void show()=0;
	virtual void hide()=0;
	PreGameTab();
};
class RanSel : public PreGameTab
{
	Button<> horcpl[9], horcte[9], conpl[9], conte[8], water[4], monster[4], //last is random
			size[4], twoLevel, showRand;
	CGroup<> *Ghorcpl, *Ghorcte, *Gconpl, *Gconte, *Gwater, *Gmonster, *Gsize;
};
class Options : public PreGameTab
{
	bool inited;
	struct OptionSwitch:public HighButton
	{
		void hover(bool on=true){};
		void select(bool on=true){};
		OptionSwitch( SDL_Rect Pos, CDefHandler* Imgs, bool Left, int Which)
			:HighButton(Pos,Imgs,false,7),left(Left),which(Which)
			{selectable=false;highlightable=false;}
		void press(bool down=true);
		bool left;
		int playerID;
		int serialID;
		int which; //-1=castle;0=hero;1=bonus
	};
	struct PlayerFlag:public HighButton
	{
		int color;
		PlayerFlag(SDL_Rect Pos, CDefHandler* Imgs, int Color)
			:HighButton(Pos,Imgs,false,7),color(Color)
			{selectable=false;highlightable=true;}
		void hover(bool on=true);
		void press(bool down=true);
		void select(bool on=true){};
	};
	struct PlayerOptions
	{
		PlayerOptions(int serial, int player);
		Ecolor color;
		PlayerFlag flag;
		//SDL_Surface * bg;
		OptionSwitch Cleft, Cright, Hleft, Hright, Bleft, Bright;
		int nr;
	};
public:
	Slider<> * turnLength;
	SDL_Surface * bg,
		* rHero, * rCastle, * nHero, * nCastle;
	std::vector<SDL_Surface*> bgs;
	std::vector<CDefHandler*> flags;
	CDefHandler //* castles, * heroes, * bonus,
		* left, * right,
		* bonuses;
	std::vector<PlayerOptions*> poptions;
	void show();
	void hide();
	void init();
	void showIcon (int what, int nr, bool abs); //what: -1=castle, 0=hero, 1=bonus, 2=all; abs=true -> nr is absolute
	Options(){inited=showed=false;};
	~Options();
};
class MapSel : public PreGameTab
{
public:
	ESortBy sortBy;
	SDL_Surface * bg;
	int selected; //selected map
	CDefHandler * Dtypes, * Dvic; 
	CDefHandler *Dsizes, * Dloss,
		* sFlags;
	std::vector<Mapa*> scenList;
	std::vector<SDL_Surface*> scenImgs;
	//int current;
	std::vector<CMapInfo> ourMaps;
	IntBut<> small, medium, large, xlarge, all;
	SetrButton<> nrplayer, mapsize, type, name, viccon, loscon;
	Slider<>  *slid, *descslid;
	int sizeFilter;
	int whichWL(int nr);
	int countWL();
	void show();
	void hide();
	void init();
	std::string gdiff(std::string ss);
	void printMaps(int from,int to=18, int at=0, bool abs=false);
	void select(int which, bool updateMapsList=true);
	void moveByOne(bool up);
	void printSelectedInfo();
	void printFlags();
	MapSel();
	~MapSel();
};
class ScenSel
{
public:
	bool listShowed;
	//RanSel ransel;
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
	std::string playerName;
	int playerColor;
	HighButton * highlighted;
	PreGameTab* currentTab;
	StartInfo ret;
	bool run;
	bool first; //hasn't we showed the scensel
	std::vector<Slider<> *> interested;
	CMusicHandler * mush;
	std::vector<HighButton *> btns;
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
	Options * ourOptions;
	std::string map; //selected map
	CPreGame(); //c-tor
	std::string buttonText(int which);
	menuItems * currentItems();
	void(CPreGame::*handleOther)(SDL_Event&);
	void scenHandleEv(SDL_Event& sEvent);
	void begin(){run=false;ret.difficulty=ourScenSel->selectedDiff;};
	void quitAskBox();
	void quit(){exit(0);};  
	void initScenSel(); 
	void showScenSel();  
	void showScenList(); 
	void initOptions();
	void showOptions();  
	void initNewMenu(); 
	void showNewMenu();  
	void showMainMenu();  
	StartInfo runLoop(); // runs mainloop of PreGame
	void initMainMenu(); //loads components for main menu
	void highlightButton(int which, int on); //highlights one from 5 main menu buttons
	void showCenBox (std::string data); //
	void showAskBox (std::string data, void(*f1)(),void(*f2)());
	void hideBox ();
	void printRating();
	void printMapsFrom(int from);
	void setTurnLength(int on);
	void sortMaps();
};

#endif //CPREGAME_H
