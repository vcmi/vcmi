#include "../stdafx.h"
#include "CAbilityHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CDefHandler.h"

void CAbilityHandler::loadAbilities()
{
	std::string buf = CGI->bitmaph->getTextFile("SSTRAITS.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		loadToIt(dump,buf,it,3);
	}
	for (int i=0; i<SKILL_QUANTITY; i++)
	{
		CAbility * nab = new CAbility; //new skill, that will be read
		loadToIt(nab->name,buf,it,4);
		loadToIt(nab->basicText,buf,it,4);
		loadToIt(nab->advText,buf,it,4);
		loadToIt(nab->expText,buf,it,3);
		nab->idNumber = abilities.size();
		abilities.push_back(nab);
	}
	abils32 = CDefHandler::giveDef("SECSK32.DEF");
	abils44 = CDefHandler::giveDef("SECSKILL.DEF");
	abils82 = CDefHandler::giveDef("SECSK82.DEF");

	buf = CGI->bitmaph->getTextFile("SKILLLEV.TXT");
	it=0;
	for(int i=0; i<6; ++i)
	{
		std::string buffo;
		loadToIt(buffo,buf,it,3);
		levels.push_back(buffo);
	}
}