#include "../stdafx.h"
#include "CGameInfo.h"
#include "../lib/VCMI_Lib.h"

/*
 * CGameInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const CGameInfo * CGI; //game info for general use
CClientState * CCS;

CGameInfo::CGameInfo()
{
	mh = NULL;
}

void CGameInfo::setFromLib()
{
	generaltexth = VLC->generaltexth;
	arth = VLC->arth;
	creh = VLC->creh;
	townh = VLC->townh;
	heroh = VLC->heroh;
	objh = VLC->objh;
	spellh = VLC->spellh;
	dobjinfo = VLC->dobjinfo;
	buildh = VLC->buildh;
}