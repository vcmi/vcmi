#include "stdafx.h"
#include "CGameInfo.h"
#include "lib/VCMI_Lib.h"

CGameInfo * CGI;

CGameInfo::CGameInfo()
{
	mh = NULL;
	state = NULL;
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