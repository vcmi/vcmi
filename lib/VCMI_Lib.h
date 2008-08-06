#ifndef VCMI_LIB_H
#define VCMI_LIB_H
#include "../global.h"

class CLodHandler;
//class CMapHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
//class CAbilityHandler;
//class CSpellHandler;
//class CAmbarCendamo;
//class CPreGameTextHandler;
class CBuildingHandler;
class CObjectHandler;
//class CMusicHandler;
//class CSemiLodHandler;
class CDefObjInfoHandler;
class CTownHandler;
//class CGeneralTextHandler;
//class CConsoleHandler;
//class CPathfinder;
//class CCursorHandler;
//class CScreenHandler;
//class CGameState;
//class CMapHandler;
//class CGameInterface;
//class CPreGame;
//class CDefHandler;

class LibClasses
{
public:
	//CGameState * state;
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	//CAbilityHandler * abilh;
	//CSpellHandler * spellh;
	//CMapHandler * mh;
	//CPreGameTextHandler * preth;
	CBuildingHandler * buildh;
	CObjectHandler * objh;
	//CMusicHandler * mush;
	//CSemiLodHandler * sspriteh;
	CDefObjInfoHandler * dobjinfo;
	CTownHandler * townh;
	//CGeneralTextHandler * generaltexth;
	//CConsoleHandler * consoleh;
	//CPathfinder * pathf;
	//CCursorHandler * curh;
	//CScreenHandler * screenh;
	//int localPlayer;
	//std::vector<CGameInterface *> playerint;
	//std::vector<SDL_Color> playerColors;
	//SDL_Color neutralColor;
	//StartInfo scenarioOps;
};

extern DLL_EXPORT LibClasses * VLC;


DLL_EXPORT void initDLL(CLodHandler *b);

#endif //VCMI_LIB_H
