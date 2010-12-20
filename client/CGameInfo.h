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


//a class for non-mechanical client GUI classes
class CClientState
{
public:
	CSoundHandler * soundh;
	CMusicHandler * musich;
	CConsoleHandler * consoleh;
	CCursorHandler * curh;
	CVideoPlayer * videoh;
};

struct Mapa;

/*
	CGameInfo class
	for allowing different functions for accessing game informations
*/
class CGameInfo
{
	ConstTransitivePtr<CGameState> state; //don't touch it in client's code
public:
	ConstTransitivePtr<CArtHandler> arth;
	ConstTransitivePtr<CHeroHandler> heroh;
	ConstTransitivePtr<CCreatureHandler> creh;
	ConstTransitivePtr<CSpellHandler> spellh;
	ConstTransitivePtr<CObjectHandler> objh;
	ConstTransitivePtr<CDefObjInfoHandler> dobjinfo;
	CGeneralTextHandler * generaltexth;
	CMapHandler * mh;
	ConstTransitivePtr<CBuildingHandler> buildh;
	CTownHandler * townh;
	//CTownHandler * townh;

	void setFromLib();

	friend class CClient;
	friend void initVillagesCapitols(Mapa * map);

	CGameInfo();
};

//	
// public:
// 	
// 	ConstTransitivePtr<CGeneralTextHandler> generaltexth;




#endif // __CGAMEINFO_H__
