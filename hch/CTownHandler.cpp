#include "../stdafx.h"
#include "CTownHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <sstream> 
#include "CGeneralTextHandler.h"
CTownHandler::CTownHandler()
{
	smallIcons = CGI->spriteh->giveDef("ITPA.DEF");
}
CTownHandler::~CTownHandler()
{
	delete smallIcons;
	//todo - delete structures info
}
void CTownHandler::loadNames()
{
	std::istringstream ins, names;
	ins.str(CGI->bitmaph->getTextFile("TOWNTYPE.TXT"));
	names.str(CGI->bitmaph->getTextFile("TOWNNAME.TXT"));
	int si=0;
	while (!ins.eof())
	{
		CTown town;
		ins >> town.name;
		char bufname[50];
		for (int i=0; i<NAMES_PER_TOWN; i++)
		{
			names.getline(bufname,50);
			town.names.push_back(std::string(bufname));
		}
		town.typeID=si++;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3; 
		if (town.name.length())
			towns.push_back(town);
	}

	std::string  strs = CGI->bitmaph->getTextFile("TCOMMAND.TXT");
	int itr=0;
	while(itr<strs.length()-1)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strs, itr, 3);
		tcommands.push_back(tmp);
	}

	
	std::ifstream of("config/buildings.txt");
	while(!of.eof())
	{
		Structure *vinya = new Structure;
		of >> vinya->townID;
		of >> vinya->ID;
		of >> vinya->defName;
		of >> vinya->x;
		of >> vinya->y;
		structures[vinya->townID][vinya->ID] = vinya;
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

int CTownHandler::getTypeByDefName(std::string name)
{
	//TODO
	return 0;
}

CTownInstance::CTownInstance()
  :pos(-1,-1,-1)
{
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	town=NULL;
}

int CTownInstance::getSightDistance() const //TODO: finish
{
	return 10;
}
