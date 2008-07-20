#ifndef CGAMEINFO_H
#define CGAMEINFO_H

#include "StartInfo.h"
#include "SDL.h"
#include "CPreGame.h"

#include <vector>


class CMapHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CAbilityHandler;
class CSpellHandler;
class CAmbarCendamo;
class CPreGameTextHandler;
class CBuildingHandler;
class CObjectHandler;
class CMusicHandler;
class CSemiLodHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CLodHandler;
class CGeneralTextHandler;
class CConsoleHandler;
class CPathfinder;
class CCursorHandler;
class CScreenHandler;
class CGameState;
class CMapHandler;
class CGameInterface;
class CPreGame;
class CDefHandler;
/*
	CGameInfo class
	for allowing different functions for modifying game informations
*/
class CGameInfo
{
public:
	static CGameInfo * mainObj; //pointer to main CGameInfo object
	CGameState * state;
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CAbilityHandler * abilh;
	CSpellHandler * spellh;
	CMapHandler * mh;
	CAmbarCendamo * ac;
	CPreGameTextHandler * preth;
	CBuildingHandler * buildh;
	CObjectHandler * objh;
	CMusicHandler * mush;
	CSemiLodHandler * sspriteh;
	CDefObjInfoHandler * dobjinfo;
	CTownHandler * townh;
	CLodHandler * spriteh;
	CLodHandler * bitmaph;
	CGeneralTextHandler * generaltexth;
	CConsoleHandler * consoleh;
	CPathfinder * pathf;
	CCursorHandler * curh;
	CScreenHandler * screenh;
	int localPlayer;
	std::vector<CGameInterface *> playerint;
	std::vector<SDL_Color> playerColors;
	std::vector<CDefHandler *> playerColorInfo; //gems from adventure map interface
	SDL_Color neutralColor;
	StartInfo scenarioOps;
};

#endif //CGAMEINFO_H
