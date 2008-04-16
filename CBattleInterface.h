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

class CBattleInterface;

class CBattleHex : public Hoverable, public MotionInterested, public ClickableL
{
public:
	unsigned int myNumber;
	bool accesible;
	//CStack * ourStack;
	bool hovered, strictHovered;
	CBattleInterface * myInterface; //interface that owns me
	static std::pair<int, int> getXYUnitAnim(int hexNum, bool attacker, CCreature * creature); //returns (x, y) of left top corner of animation
	static signed char mutualPosition(int hex1, int hex2); //returns info about mutual position of given hexes (-1 - they distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(boost::logic::tribool down);
	CBattleHex();
};

class CBattleObstacle
{
	std::vector<int> lockedHexes;
};

class CBattleConsole : public IShowable, public CIntObject
{
private:
	std::vector< std::string > texts; //a place where texts are stored
	int lastShown; //las shown line of text
public:
	CBattleConsole(); //c-tor
	~CBattleConsole(); //d-tor
	void show(SDL_Surface * to = 0);
	bool addText(std::string text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void eraseText(unsigned int pos); //erases added text at position pos
	void changeTextAt(std::string text, unsigned int pos); //if we have more than pos texts, pos-th is changed to given one
	void scrollUp(unsigned int by = 1); //scrolls console up by 'by' positions
	void scrollDown(unsigned int by = 1); //scrolls console up by 'by' positions
};

class CBattleInterface : public IActivable, public IShowable
{
private:
	SDL_Surface * background, * menu, * amountBasic, * amountNormal;
	AdventureMapButton<CBattleInterface> * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
		* bWait, * bDefence, * bConsoleUp, * bConsoleDown;
	CBattleConsole * console;
	CBattleHero * attackingHero, * defendingHero;
	CCreatureSet * army1, * army2; //fighting armies
	CGHeroInstance * attackingHeroInstance, * defendingHeroInstance;
	std::map< int, CCreatureAnimation * > creAnims; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map< int, bool > creDir; // <creatureID, if false reverse creature's animation>
	unsigned char animCount;
	int activeStack; //number of active stack; -1 - no one
	void showRange(SDL_Surface * to, int ID); //show helper funtion ot mark range of a unit

public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	bool printCellBorders; //if true, cell borders will be printed
	CBattleHex bfield[187]; //11 lines, 17 hexes on each
	std::vector< CBattleObstacle * > obstacles; //vector of obstacles on the battlefield
	static SDL_Surface * cellBorder, * cellShade;
	BattleAction * givenCommand; //true if we have i.e. moved current unit

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
	void show(SDL_Surface * to = NULL);
	bool reverseCreature(int number, int hex); //reverses animation of given creature playing animation of reversing

	//call-ins
	void newStack(CStack stack); //new stack appeared on battlefield
	void stackRemoved(CStack stack); //stack disappeared from batlefiled
	void stackActivated(int number); //active stack has been changed
	void stackMoved(int number, int destHex, bool startMoving, bool endMoving); //stack with id number moved to destHex
	void stackAttacking(int ID, int dest); //called when stack with id ID is attacking something on hex dest
	void turnEnded(); //caled when current unit cannot get new orders
	void hexLclicked(int whichOne); //hex only call-in
};
