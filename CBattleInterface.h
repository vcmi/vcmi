#pragma once
#include "global.h"
#include "CPlayerInterface.h"
class CCreatureSet;
class CGHeroInstance;
class CBattleInterface : public IActivable
{
public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2);
	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate();
	void deactivate();
};