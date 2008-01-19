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

CBuildingRect::CBuildingRect(Structure *Str)
:str(Str)
{	
	def = CGI->spriteh->giveDef(Str->defName);
	border = area = NULL;
	pos.x = str->pos.x;
	pos.y = str->pos.y;
	pos.w = def->ourImages[0].bitmap->w;
	pos.h = def->ourImages[0].bitmap->h;
}

CBuildingRect::~CBuildingRect()
{
	delete def;
	if(border)
		SDL_FreeSurface(border);
	if(area)
		SDL_FreeSurface(area);
}
void CBuildingRect::activate()
{
	MotionInterested::activate();
	ClickableL::activate();
	ClickableR::activate();
}
void CBuildingRect::deactivate()
{
	MotionInterested::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
}
bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	if(str->pos.z != p2.str->pos.z)
		return (str->pos.z) < (p2.str->pos.z);
	else
		return (str->ID) < (p2.str->ID);
}
void CBuildingRect::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	//todo - handle
}
void CBuildingRect::clickLeft (tribool down)
{
	//todo - handle
}
void CBuildingRect::clickRight (tribool down)
{
	//todo - handle
}
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
class SORTHELP
{
public:
	bool operator ()
		(const CBuildingRect *a ,
		 const CBuildingRect *b)
	{
		return (*a)<(*b);
	}
} srthlp ;

CCastleInterface::CCastleInterface(const CGTownInstance * Town, bool Activate)
{
	count=0;
	town = Town;
	townInt = CGI->bitmaph->loadBitmap("TOWNSCRN.bmp");
	cityBg = CGI->bitmaph->loadBitmap(getBgName(town->subID));
	hall = CGI->spriteh->giveDef("ITMTL.DEF");
	fort = CGI->spriteh->giveDef("ITMCL.DEF");
	bigTownPic =  CGI->spriteh->giveDef("ITPT.DEF");
	flag =  CGI->spriteh->giveDef("CREST58.DEF");
	CSDL_Ext::blueToPlayersAdv(townInt,LOCPLINT->playerID);
	exit = new AdventureMapButton<CCastleInterface>
		(CGI->townh->tcommands[8],"",&CCastleInterface::close,744,544,"TSBTNS.DEF",this,Activate);
	exit->bitmapOffset = 4;

	for (std::set<int>::const_iterator i=town->builtBuildings.begin();i!=town->builtBuildings.end();i++)
	{
		if(CGI->townh->structures.find(town->subID) != CGI->townh->structures.end())
		{
			if(CGI->townh->structures[town->subID].find(*i)!=CGI->townh->structures[town->subID].end())
			{
				//CDefHandler *b = CGI->spriteh->giveDef(CGI->townh->structures[town->subID][*i]->defName);
				buildings.push_back(new CBuildingRect(CGI->townh->structures[town->subID][*i]));
				//boost::tuples::tuple<int,CDefHandler*,Structure*,SDL_Surface*,SDL_Surface*> *t 
				//	= new boost::tuples::tuple<int,CDefHandler*,Structure*,SDL_Surface*,SDL_Surface*>
				//	(*i,b,CGI->townh->structures[town->subID][*i],NULL,NULL);
				////TODO: obwódki i pola
				//buildings.push_back(t);
			}
			else continue;
		}
		else
			break;
	}

	std::sort(buildings.begin(),buildings.end(),srthlp);

	if(Activate)
	{
		LOCPLINT->objsToBlit.push_back(this);
		activate();
		showAll();
	}

	//blit buildings on bg
	//for(int i=0;i<buildings.size();i++)
	//{
	//	blitAt(buildings[i]->def->ourImages[0].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,cityBg);
	//}
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

	for(int i=0;i<buildings.size();i++)
	{
		delete buildings[i];
	}

}
void CCastleInterface::close()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	deactivate();
	LOCPLINT->castleInt = NULL;
	LOCPLINT->adventureInt->show();
	delete this;
}
void CCastleInterface::showAll(SDL_Surface * to)
{	
	if (!to)
		to=ekran;
	blitAt(cityBg,0,0,to);
	blitAt(townInt,0,374,to);
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
	blitAt(fort->ourImages[pom].bitmap,122,413,to);

	//draw ((village/town/city) hall)/capitol icon
	if(town->builtBuildings.find(13)!=town->builtBuildings.end())
		pom = 3;
	else if(town->builtBuildings.find(12)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(11)!=town->builtBuildings.end())
		pom = 1;
	else pom = 0;
	blitAt(hall->ourImages[pom].bitmap,80,413,to);

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
			blitAt(CGI->creh->smallImgs[cid],pomx,pomy,to);
			std::ostringstream oss;
			oss << '+' << town->creatureIncome[i];
			CSDL_Ext::printAtMiddle(oss.str(),pomx+16,pomy+37,GEOR13,zwykly,to);
		}
	}

	//print name and income
	CSDL_Ext::printAt(town->name,85,389,GEOR13,zwykly,to);
	char temp[10];
	itoa(town->income,temp,10);
	CSDL_Ext::printAtMiddle(temp,195,442,GEOR13,zwykly,to);

	//blit town icon
	pom = town->subID*2;
	if (!town->hasFort())
		pom += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(bigTownPic->ourImages[pom].bitmap,15,387,to);

	//flag
	blitAt(flag->ourImages[town->getOwner()].bitmap,241,387,to);
	//print garrison
	for(
		std::map<int,std::pair<CCreature*,int> >::const_iterator i=town->garrison.slots.begin();
		i!=town->garrison.slots.end();
		i++
			)
	{
		blitAt(CGI->creh->bigImgs[i->second.first->idNumber],305+(62*(i->first)),387,to);
		itoa(i->second.second,temp,10);
		CSDL_Ext::printTo(temp,305+(62*(i->first))+57,387+61,GEOR13,zwykly,to);
	}
	show();
}
void CCastleInterface::show(SDL_Surface * to)
{
	if (!to)
		to=ekran;
	count++;
	if(count==5)
	{
		count=0;
		animval++;
	}


	//blit buildings
	for(int i=0;i<buildings.size();i++)
	{
		if((animval)%(buildings[i]->def->ourImages.size()))
		{
			blitAt(buildings[i]->def->ourImages[0].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
			blitAt(buildings[i]->def->ourImages[(animval)%(buildings[i]->def->ourImages.size())].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
		}
		else 
			blitAt(buildings[i]->def->ourImages[(animval)%(buildings[i]->def->ourImages.size())].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
	}
	//for(int i=0;i<buildings.size();i++)
	//{
	//	if((animval)%(buildings[i]->def->ourImages.size())==0)
	//		blitAt(buildings[i]->def->ourImages[(animval)%(buildings[i]->def->ourImages.size())].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
	//	else continue;
	//}
	
}
void CCastleInterface::activate()
{
	//for(int i=0;i<buildings.size();i++)
	//	buildings[i]->activate();
}
void CCastleInterface::deactivate()
{
	exit->deactivate();
}