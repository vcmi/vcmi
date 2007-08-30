#include "../stdafx.h"
#include "CTownHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <sstream>

CTownHandler::CTownHandler()
{
	smallIcons = CGI->spriteh->giveDef("ITPA.DEF");
}
CTownHandler::~CTownHandler()
{
	delete smallIcons;
}
void CTownHandler::loadNames()
{
	std::istringstream ins;
	ins.str(CGI->bitmaph->getTextFile("TOWNTYPE.TXT"));
	while (!ins.eof())
	{
		CTown town;
		ins >> town.name;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3; 
		if (town.name.length())
			towns.push_back(town);
	}
}
SDL_Surface * CTownHandler::getPic(int ID, bool fort, bool builded)
{
	if (ID==-1)
		return smallIcons->ourImages[0].bitmap;
	else if (ID==-2)
		return smallIcons->ourImages[1].bitmap;
	else if (ID==-3)
		return smallIcons->ourImages[2+F_NUMBER*4].bitmap;
	else if (ID>F_NUMBER || ID<-3)
		throw new std::exception("Invalid ID");
	else
	{
		int pom = 3;
		if(!fort)
			pom+=F_NUMBER*2;
		pom += ID*2;
		if (!builded)
			pom--;
		return smallIcons->ourImages[pom].bitmap;
	}
}