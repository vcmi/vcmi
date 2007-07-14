#ifndef CGAMEINFO_H
#define CGAMEINFO_H

#include "CSpellHandler.h"
#include "CAbilityHandler.h"
#include "CCreaturehandler.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CAmbarCendamo.h"
#include "CBuildingHandler.h"
#include "CObjectHandler.h"
#include "CMusicHandler.h"
#include "CSemiLodHandler.h"
#include "CDefObjInfoHandler.h"
#include "CLodHandler.h"
#include "CGeneralTextHandler.h"
#include "SDL.h"
#include <vector>

/*
	CGameInfo class
	for allowing different functions for modifying game informations
*/
class CGameInfo
{
public:
	static CGameInfo * mainObj; //pointer to main CGameInfo object
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CAbilityHandler * abilh;
	CSpellHandler * spellh;
	CAmbarCendamo * ac;
	CBuildingHandler * buildh;
	CObjectHandler * objh;
	CMusicHandler * mush;
	CSemiLodHandler * sspriteh;
	CDefObjInfoHandler * dobjinfo;
	CLodHandler * spriteh;
	CLodHandler * bitmaph;
	CGeneralTextHandler * generaltexth;
	std::vector<SDL_Color> playerColors;
	SDL_Color neutralColor;
};

#endif //CGAMEINFO_H