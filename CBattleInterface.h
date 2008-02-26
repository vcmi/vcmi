#pragma once
#include "global.h"
#include "CPlayerInterface.h"

class CCreatureSet;
class CGHeroInstance;
template <typename T> class AdventureMapButton;

class CBattleInterface : public IActivable, public IShowable
{
private:
	SDL_Surface * background, * menu;
	AdventureMapButton<CBattleInterface> * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
		* bWait, * bDefence, * bConsoleUp, * bConsoleDown;
public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2);
	~CBattleInterface();

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