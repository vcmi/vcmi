/*
 * CGameInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameInfo.h"
#include "../lib/CSkillHandler.h"
#include "../lib/CGeneralTextHandler.h"

#include "../lib/VCMI_Lib.h"

const CGameInfo * CGI; //game info for general use
CClientState * CCS = nullptr;

CGameInfo::CGameInfo()
{
	generaltexth = nullptr;
	mh = nullptr;
	townh = nullptr;
}

void CGameInfo::setFromLib()
{
	modh = VLC->modh;
	generaltexth = VLC->generaltexth;
	arth = VLC->arth;
	creh = VLC->creh;
	townh = VLC->townh;
	heroh = VLC->heroh;
	objh = VLC->objh;
	spellh = VLC->spellh;
	skillh = VLC->skillh;
	objtypeh = VLC->objtypeh;
}
