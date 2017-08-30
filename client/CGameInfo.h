/*
 * CGameInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once


#include "../lib/ConstTransitivePtr.h"

class CModHandler;
class CMapHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CSkillHandler;
class CBuildingHandler;
class CObjectHandler;
class CSoundHandler;
class CMusicHandler;
class CObjectClassesHandler;
class CTownHandler;
class CGeneralTextHandler;
class CConsoleHandler;
class CCursorHandler;
class CGameState;
class IMainVideoPlayer;

class CMap;


//a class for non-mechanical client GUI classes
class CClientState
{
public:
	CSoundHandler * soundh;
	CMusicHandler * musich;
	CConsoleHandler * consoleh;
	CCursorHandler * curh;
	IMainVideoPlayer * videoh;
};
extern CClientState * CCS;

/// CGameInfo class
/// for allowing different functions for accessing game informations
class CGameInfo
{
public:
	ConstTransitivePtr<CModHandler> modh; //public?
	ConstTransitivePtr<CArtHandler> arth;
	ConstTransitivePtr<CHeroHandler> heroh;
	ConstTransitivePtr<CCreatureHandler> creh;
	ConstTransitivePtr<CSpellHandler> spellh;
	ConstTransitivePtr<CSkillHandler> skillh;
	ConstTransitivePtr<CObjectHandler> objh;
	ConstTransitivePtr<CObjectClassesHandler> objtypeh;
	CGeneralTextHandler * generaltexth;
	CMapHandler * mh;
	CTownHandler * townh;

	void setFromLib();

	friend class CClient;

	CGameInfo();
};
extern const CGameInfo* CGI;
