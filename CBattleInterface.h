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

class CBattleHex : public Hoverable
{
public:
	unsigned int myNumber;
	bool accesible;
	//CStack * ourStack;
	bool hovered;
	static std::pair<int, int> getXYUnitAnim(int hexNum, bool attacker); //returns (x, y) of left top corner of animation
	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
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
	SDL_Surface * cellBorder, * cellShade;
	CCreatureSet * army1, * army2; //fighting armies
	CGHeroInstance * attackingHeroInstance, * defendingHeroInstance;
	std::vector< CCreatureAnimation * > creAnims; //animations of creatures from fighting armies (order like in BattleInfo's stacks)

	CCallback * cb; //our callback for getting info about different things
	const std::vector< CStack* > & stacks; //fighting stacks
public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CCallback * callback, CGHeroInstance *hero1, CGHeroInstance *hero2, const std::vector< CStack* > & stcks); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	bool printCellBorders; //if true, cell borders will be printed
	CBattleHex bfield[187]; //11 lines, 17 hexes on each
	std::vector< CBattleObstacle * > obstacles; //vector of obstacles on the battlefield

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
};
