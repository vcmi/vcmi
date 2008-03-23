#pragma once
#include "global.h"
#include "CPlayerInterface.h"

class CCreatureSet;
class CGHeroInstance;
class CDefHandler;
class CStack;
class CCallback;
template <typename T> class AdventureMapButton;

class CBattleHero : public IShowable, public CIntObject
{
public:
	bool flip; //false if it's attacking hero, true otherwise
	CDefHandler * dh, *flag; //animation and flag
	int phase; //stage of animation
	int image; //frame of animation
	unsigned char flagAnim, flagAnimCount; //for flag animation
	void show(SDL_Surface * to); //prints next frame of animation to to
	CBattleHero(std::string defName, int phaseG, int imageG, bool filpG, unsigned char player); //c-tor
	~CBattleHero(); //d-tor
};

class CBattleHex : public Hoverable, public MotionInterested
{
public:
	unsigned int myNumber;
	bool accesible;
	//CStack * ourStack;
	bool hovered, strictHovered;
	static std::pair<int, int> getXYUnitAnim(int hexNum, bool attacker); //returns (x, y) of left top corner of animation
	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	CBattleHex();
};

class CBattleObstacle
{
	std::vector<int> lockedHexes;
};

class CBattleInterface : public IActivable, public IShowable
{
private:
	SDL_Surface * background, * menu;
	AdventureMapButton<CBattleInterface> * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
		* bWait, * bDefence, * bConsoleUp, * bConsoleDown;
	CBattleHero * attackingHero, * defendingHero;
	CCreatureSet * army1, * army2; //fighting armies
	CGHeroInstance * attackingHeroInstance, * defendingHeroInstance;
	std::map< int, CCreatureAnimation * > creAnims; //animations of creatures from fighting armies (order like in BattleInfo's stacks)
	unsigned char animCount;
	int activeStack; //number of active stack; -1 - no one
	void showRange(SDL_Surface * to, int initialPlace, int radius); //show helper funtion ot mark range of a unit

public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	bool printCellBorders; //if true, cell borders will be printed
	CBattleHex bfield[187]; //11 lines, 17 hexes on each
	std::vector< CBattleObstacle * > obstacles; //vector of obstacles on the battlefield
	static SDL_Surface * cellBorder, * cellShade;

	//button handle funcs:
	void bOptionsf();
	void bSurrenderf();
	void bFleef();
	void bAutofightf();
	void bSpellf();
	void bWaitf();
	void bDefencef();
	void bConsoleUpf();
	void bConsoleDownf();
	//end of button handle funcs
	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate();
	void deactivate();
	void show(SDL_Surface * to);

	//call-ins
	void newStack(CStack stack); //new stack appeared on battlefield
	void stackRemoved(CStack stack); //stack disappeared from batlefiled
	void stackActivated(int number); //active stack has been changed
	void stackMoved(int number, int destHex); //stack with id number moved to destHex
};
