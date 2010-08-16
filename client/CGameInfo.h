#ifndef __CGAMEINFO_H__
#define __CGAMEINFO_H__
#include "../global.h"


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

class Mapa;

/*
	CGameInfo class
	for allowing different functions for accessing game informations
*/
class CGameInfo
{
	/*const*/ CGameState * state; //don't touch it in client's code
public:
	/*const*/ CArtHandler * arth;
	/*const*/ CHeroHandler * heroh;
	/*const*/ CCreatureHandler * creh;
	/*const*/ CSpellHandler * spellh;
	/*const*/ CMapHandler * mh;
	/*const*/ CBuildingHandler * buildh;
	/*const*/ CObjectHandler * objh;
	CSoundHandler * soundh;
	CMusicHandler * musich;
	/*const*/ CDefObjInfoHandler * dobjinfo;
	/*const*/ CTownHandler * townh;
	/*const*/ CGeneralTextHandler * generaltexth;
	CConsoleHandler * consoleh;
	CCursorHandler * curh;
	CVideoPlayer * videoh;

	void setFromLib();

	friend class CClient;
	friend void initVillagesCapitols(Mapa * map);

	CGameInfo();
};






#endif // __CGAMEINFO_H__
