#include "stdafx.h"
#include "CCastleInterface.h"
#include "hch/CObjectHandler.h"
#include "CGameInfo.h"
#include "hch/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "hch/CTownHandler.h"
#include "AdventureMapButton.h"
#include <sstream>
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
	bigTownPic =  CGI->spriteh->giveDef("ITPT.DEF");
	flag =  CGI->spriteh->giveDef("CREST58.DEF");
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
	delete bigTownPic;
	delete flag;
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

	//draw fort icon
	if(town->builtBuildings.find(9)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(8)!=town->builtBuildings.end())
		pom = 1;
	else if(town->builtBuildings.find(7)!=town->builtBuildings.end())
		pom = 0;
	else pom = 3;
	blitAt(fort->ourImages[pom].bitmap,122,413);

	//draw ((village/town/city) hall)/capitol icon
	if(town->builtBuildings.find(13)!=town->builtBuildings.end())
		pom = 3;
	else if(town->builtBuildings.find(12)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(11)!=town->builtBuildings.end())
		pom = 1;
	else pom = 0;
	blitAt(hall->ourImages[pom].bitmap,80,413);

	//draw creatures icons and their growths
	for(int i=0;i<CREATURES_PER_TOWN;i++)
	{
		int cid = -1;
		if (town->builtBuildings.find(30+i)!=town->builtBuildings.end())
		{
			cid = (14*town->subID)+(i*2);
			if (town->builtBuildings.find(30+CREATURES_PER_TOWN+i)!=town->builtBuildings.end())
			{
				cid++;
			}
		}
		if (cid>=0)
		{
			int pomx, pomy;
			pomx = 22 + (55*((i>3)?(i-4):i));
			pomy = (i>3)?(507):(459);
			blitAt(CGI->creh->smallImgs[cid],pomx,pomy);
			std::ostringstream oss;
			oss << '+' << town->creatureIncome[i];
			CSDL_Ext::printAtMiddle(oss.str(),pomx+16,pomy+37,GEOR13,zwykly);
		}
	}

	//print name and income
	CSDL_Ext::printAt(town->name,85,389,GEOR13,zwykly);
	char temp[10];
	itoa(town->income,temp,10);
	CSDL_Ext::printAtMiddle(temp,195,442,GEOR13,zwykly);

	//blit town icon
	pom = town->subID*2;
	if (!town->hasFort())
		pom += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(bigTownPic->ourImages[pom].bitmap,15,387);

	//flag
	blitAt(flag->ourImages[town->getOwner()].bitmap,241,387);
	//print garrison
	for(std::map<int,std::pair<CCreature*,int> >::const_iterator i=town->garrison.slots.begin();i!=town->garrison.slots.end();i++)
	{
		blitAt(CGI->creh->bigImgs[i->second.first->idNumber],305+(62*(i->first)),387);
		itoa(i->second.second,temp,10);
		CSDL_Ext::printTo(temp,305+(62*(i->first))+57,387+61,GEOR13,zwykly);
	}
	
}
void CCastleInterface::activate()
{
}
void CCastleInterface::deactivate()
{
	exit->deactivate();
}