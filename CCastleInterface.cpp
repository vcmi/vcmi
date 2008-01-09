#include "stdafx.h"
#include "CCastleInterface.h"
#include "hch/CObjectHandler.h"
#include "CGameInfo.h"
#include "hch/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "hch/CTownHandler.h"
#include "AdventureMapButton.h"
std::string getBgName(int type) //TODO - co z tym zrobiæ?
{
	switch (type)
	{
	case 0:
		return "TBCSBACK.bmp";
	case 1:
		return "TBRMBACK.bmp";
	case 2:
		return "TBTWBACK.bmp";
	case 3:
		return "TBINBACK.bmp";
	case 4:
		return "TBNCBACK.bmp";
	case 5:
		return "TBDNBACK.bmp";
	case 6:
		return "TBSTBACK.bmp";
	case 7:
		return "TBFRBACK.bmp";
	case 8:
		return "TBELBACK.bmp";
	default:
		throw new std::exception("std::string getBgName(int type): invalid type");
	}
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, bool Activate)
{
	town = Town;
	townInt = CGI->bitmaph->loadBitmap("TOWNSCRN.bmp");
	cityBg = CGI->bitmaph->loadBitmap(getBgName(town->subID));
	hall = CGI->spriteh->giveDef("ITMTL.DEF");
	fort = CGI->spriteh->giveDef("ITMCL.DEF");
	CSDL_Ext::blueToPlayersAdv(townInt,LOCPLINT->playerID);
	exit = new AdventureMapButton<CCastleInterface>(CGI->townh->tcommands[8],"",&CCastleInterface::close,744,544,"TSBTNS.DEF",this,Activate);
	exit->bitmapOffset = 4;
	if(Activate)
	{
		activate();
		show();
	}
}
CCastleInterface::~CCastleInterface()
{
	SDL_FreeSurface(townInt);
	SDL_FreeSurface(cityBg);
	delete exit;
	delete hall;
	delete fort;
}
void CCastleInterface::close()
{
	deactivate();
	LOCPLINT->castleInt = NULL;
	LOCPLINT->adventureInt->show();
	delete this;
}
void CCastleInterface::show()
{
	blitAt(cityBg,0,0);
	blitAt(townInt,0,374);
	LOCPLINT->adventureInt->resdatabar.draw();

	int pom;
	if(town->builtBuildings.find(9)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(8)!=town->builtBuildings.end())
		pom = 1;
	else if(town->builtBuildings.find(7)!=town->builtBuildings.end())
		pom = 0;
	else pom = 3;
	blitAt(fort->ourImages[pom].bitmap,122,413);

	if(town->builtBuildings.find(13)!=town->builtBuildings.end())
		pom = 3;
	else if(town->builtBuildings.find(12)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(11)!=town->builtBuildings.end())
		pom = 1;
	else pom = 0;
	blitAt(hall->ourImages[pom].bitmap,80,413);

	CSDL_Ext::printAt(town->name,85,389,GEOR13,zwykly);
	char temp[10];
	itoa(town->income,temp,10);
	CSDL_Ext::printAtMiddle(temp,195,442,GEOR13,zwykly);
}
void CCastleInterface::activate()
{
}
void CCastleInterface::deactivate()
{
	exit->deactivate();
}