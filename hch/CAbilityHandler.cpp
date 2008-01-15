#include "../stdafx.h"
#include "CAbilityHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CDefHandler.h"

void CAbilityHandler::loadAbilities()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("SSTRAITS.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		CGeneralTextHandler::loadToIt(dump,buf,it,3);
	}
	for (int i=0; i<SKILL_QUANTITY; i++)
	{
		CAbility * nab = new CAbility; //new skill, that will be read
		CGeneralTextHandler::loadToIt(nab->name,buf,it,4);
		CGeneralTextHandler::loadToIt(nab->basicText,buf,it,4);
		CGeneralTextHandler::loadToIt(nab->advText,buf,it,4);
		CGeneralTextHandler::loadToIt(nab->expText,buf,it,3);
		nab->idNumber = abilities.size();
		abilities.push_back(nab);
	}
	abils32 = CGI->spriteh->giveDef("SECSK32.DEF");
	abils44 = CGI->spriteh->giveDef("SECSKILL.DEF");
	abils82 = CGI->spriteh->giveDef("SECSK82.DEF");

	buf = CGameInfo::mainObj->bitmaph->getTextFile("SKILLLEV.TXT");
	it=0;
	for(int i=0; i<6; ++i)
	{
		std::string buffo;
		CGeneralTextHandler::loadToIt(buffo,buf,it,3);
		levels.push_back(buffo);
	}
}