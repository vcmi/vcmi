#ifndef __CGAMEINFO_H__
#define __CGAMEINFO_H__
#include "../global.h"
#include "../lib/ConstTransitivePtr.h"


/*
 * CGameInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CMapHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CBuildingHandler;
class CObjectHandler;
class CSoundHandler;
class CMusicHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;
class CConsoleHandler;
class CCursorHandler;
class CGameState;
class CVideoPlayer;

struct Mapa;

/*
	CGameInfo class
	for allowing different functions for accessing game informations
*/
class CGameInfo
{
	CGameState * state; //don't touch it in client's code
public:
	ConstTransitivePtr<CArtHandler> arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CObjectHandler * objh;
	CDefObjInfoHandler * dobjinfo;
	CGeneralTextHandler * generaltexth;
	CMapHandler * mh;
	CBuildingHandler * buildh;
	mutable CSoundHandler * soundh;
	mutable CMusicHandler * musich;
	CTownHandler * townh;
	//CTownHandler * townh;
	mutable CConsoleHandler * consoleh;
	mutable CCursorHandler * curh;
	mutable CVideoPlayer * videoh;

	void setFromLib();

	friend class CClient;
	friend void initVillagesCapitols(Mapa * map);

	CGameInfo();
};

//	ConstTransitivePtr<CGameState> state; //don't touch it in client's code
// public:
// 	
// 	ConstTransitivePtr<CHeroHandler> heroh;
// 	ConstTransitivePtr<CCreatureHandler> creh;
// 	ConstTransitivePtr<CSpellHandler> spellh;
// 	ConstTransitivePtr<CObjectHandler> objh;
// 	ConstTransitivePtr<CDefObjInfoHandler> dobjinfo;
// 	ConstTransitivePtr<CGeneralTextHandler> generaltexth;




#endif // __CGAMEINFO_H__
