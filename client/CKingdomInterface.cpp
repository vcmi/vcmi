#include "CKingdomInterface.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/map.h"
#include "../lib/NetPacks.h"
#include <boost/assign/std/vector.hpp> 
#include <sstream>
using namespace boost::assign;
using namespace CSDL_Ext;

/*
 * CKingdomInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

int PicCount = 4;//how many pictures for background present in .def TODO - move to config
int size = 4;//we have 4 visible items in the list, would be nice to move this value to configs later

CDefEssential* CKingdomInterface::slots;
CDefEssential* CKingdomInterface::fort;
CDefEssential* CKingdomInterface::hall;

CKingdomInterface::CKingdomInterface()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	defActions = /*ACTIVATE | DEACTIVATE |*/ SHARE_POS | DISPOSE;//??? why buttons dont receive activate/deactivate???
	pos.x = screen->w/2 - 400;
	pos.y = screen->h/2 - (68+58*size);
	heroPos = townPos = objPos = 0;
	state = 2;
	showHarrisoned = false;

	bg = BitmapHandler::loadBitmap("OVCAST.bmp");
	graphics->blueToPlayersAdv(bg, LOCPLINT->playerID);
	mines = CDefHandler::giveDefEss("OVMINES.DEF");
	slots = CDefHandler::giveDefEss("OVSLOT.DEF");
	title = CDefHandler::giveDefEss("OVTITLE.DEF");
	hall = CDefHandler::giveDefEss("ITMTL.DEF");
	fort = CDefHandler::giveDefEss("ITMCL.DEF");
	objPics = CDefHandler::giveDefEss("FLAGPORT.DEF");

	toHeroes = new AdventureMapButton (CGI->generaltexth->overview[11],"",
		boost::bind(&CKingdomInterface::listToHeroes,this),748,28+size*116,"OVBUTN1.DEF");
	toHeroes->block(2);

	toTowns = new AdventureMapButton (CGI->generaltexth->overview[12],"",
		boost::bind(&CKingdomInterface::listToTowns,this),748,64+size*116,"OVBUTN6.DEF");
	toTowns->block(0);

	exit = new AdventureMapButton (CGI->generaltexth->allTexts[600],"",
		boost::bind(&CKingdomInterface::close,this),748,99+size*116,"OVBUTN1.DEF");
	exit->bitmapOffset = 3;

	statusbar = new CStatusBar(pos.x+7,pos.y+91+size*116,"TSTATBAR.bmp",732);
	resdatabar = new CResDataBar("KRESBAR.bmp",pos.x+3,pos.y+111+size*116,32,2,76,76);

	for (int i=0; i<RESOURCE_QUANTITY; i++)
		incomes.push_back(new CResIncomePic(i,mines));

	heroes.resize(size);
	for(size_t i=0;i<size;i++)//preparing lists for input
		heroes[i] = new  CHeroItem(i);
	towns.resize(size);
	for(size_t i=0;i<size;i++)
		towns[i] = new  CTownItem(i);

	slider = new CSlider(4, 4, size*116+19, boost::bind (&CKingdomInterface::sliderMoved, this, _1),
		size, LOCPLINT->cb->howManyHeroes(showHarrisoned), 0, false, 0);

	//creating objects list

	ObjTop = new AdventureMapButton ("","", boost::bind(&CKingdomInterface::moveObjectList,this,0),
		733,4,"OVBUTN4.DEF");

	ObjUp = new AdventureMapButton ("","", boost::bind(&CKingdomInterface::moveObjectList,this,1),
		733,24,"OVBUTN4.DEF");
	ObjUp->bitmapOffset = 4;

	ObjDown = new AdventureMapButton ("","", boost::bind(&CKingdomInterface::moveObjectList,this,2),
		733,size*116-18,"OVBUTN4.DEF");
	ObjDown->bitmapOffset = 6;

	ObjBottom = new AdventureMapButton ("","", boost::bind(&CKingdomInterface::moveObjectList,this,3),
		733,size*116+2,"OVBUTN4.DEF");
	ObjBottom->bitmapOffset = 2;

	std::map<std::pair<int,int>,int> addObjects;//objects to print, except 17th dwelling
	//format: (id,subID),image index
	#define INSERT_MAP addObjects.insert(std::pair<std::pair<int,int>,int>(std::pair<int,int>
	INSERT_MAP (20,1) ,81));//Golem factory
	INSERT_MAP (42,0) ,82));//Lighthouse
	INSERT_MAP (33,0) ,83));//Garrison
	INSERT_MAP (219,0),83));//Garrison
	INSERT_MAP (33,1) ,84));//Anti-magic Garrison
	INSERT_MAP (219,1),84));//Anti-magic Garrison
	INSERT_MAP (53,7) ,85));//Abandoned mine
	INSERT_MAP (20,0) ,86));//Conflux
	INSERT_MAP (87,0) ,87));//Harbor
	#undef INSERT_MAP
	for(size_t i = 0; i<CGI->state->map->objects.size(); i++)
	{
		CGObjectInstance* obj = CGI->state->map->objects[i];
		if (obj)
		{
			std::pair<int,int > curElm = std::pair<int,int >(obj->ID, obj->subID);

			if (obj->tempOwner == CGI->state->currentPlayer)
			{
				if ( obj->ID == 17 )
				{
					objList[obj->subID].first += 1;
					objList[obj->subID].second = & CGI->creh->creatures[CGI->objh->cregens[obj->subID]].namePl;
				}
				else if (addObjects.find(curElm) != addObjects.end())
				{
					objList[addObjects[curElm]].first += 1;
					objList[addObjects[curElm]].second = & obj->hoverName;
				}
			}
		}
	}
	addObjects.clear();
	objSize = (size*116-64)/57; //in object list will fit (height of panel)/(height of one element) items
	ObjList.resize(objSize);
	for(size_t i=0;i<objSize;i++)
	{
		ObjList[i] = new HoverableArea();
		ObjList[i]->pos = genRect(50,50,pos.x+740,pos.y+44+i*57);
	}
}

void CKingdomInterface::moveObjectList(int newPos)
{
	int top =    objList.size() > objSize ? 0 : objList.size() - objSize ;
	int bottom = objList.size() > objSize ? objList.size() - objSize : 0 ;
	switch (newPos)//checking what button was pressed
	{
		case 0: objPos = top;
			break;
		case 1: objPos = objPos==top?top:(objPos-1);
			break;
		case 2: objPos = objPos==bottom?bottom:(objPos+1);
			break;
		case 3: objPos = bottom;
			break;
	}
	GH.totalRedraw();
}

CKingdomInterface::~CKingdomInterface()
{
	SDL_FreeSurface(bg);

	delete title;
	delete slots;
	delete fort;
	delete hall;
	delete mines;

	towns.clear();
	heroes.clear();
	incomes.clear();
	ObjList.clear();
	objList.clear();
}

void CKingdomInterface::close()
{
	GH.popIntTotally(this);
}

void CKingdomInterface::showAll( SDL_Surface * to/*=NULL*/)
{
	LOCPLINT->adventureInt->resdatabar.draw(to);
	blitAt(bg,pos,to);
	resdatabar->draw(to);
	toTowns->show(to);
	toHeroes->show(to);
	exit->show(to);

	ObjTop->show(to);
	ObjUp->show(to);
	ObjDown->show(to);
	ObjBottom->show(to);

	for (size_t i=0; i<ObjList.size(); i++)//list may be moved, recreate hover text
		ObjList[i]->hoverText = "";

	int skipCount=0, curPos=objPos<0?(-objPos):0;
	for (std::map<int,std::pair<int, std::string*> >::iterator it=objList.begin(); it!= objList.end(); it++)
	{
		if (skipCount<objPos)
		{
			skipCount++;
			continue;
		}
		blitAt(objPics->ourImages[(*it).first].bitmap,pos.x+740,pos.y+44+curPos*57,to);

		std::ostringstream ostrs;
		ostrs << (*it).second.first;
		CSDL_Ext::printTo(ostrs.str(),pos.x+790,pos.y+94+curPos*57,GEOR13,zwykly,to);

		ObjList[curPos]->hoverText = * (*it).second.second;
		curPos++;
		if (curPos == objSize)
			break;
	}

	if (state == 1)
	{//printing text "Town", "Harrisoned hero", "Visiting hero"
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[3],pos.x+145,pos.y+12,TNRB16,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[4],pos.x+370,pos.y+12,TNRB16,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[5],pos.x+600,pos.y+12,TNRB16,zwykly,to);
		for (size_t i=0; i<size; i++)
			towns[i]->show(to);//show town list
	}
	else
	{//text "Hero/stats" and "Skills"
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[0],pos.x+150,pos.y+12,TNRB16,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[1],pos.x+500,pos.y+12,TNRB16,zwykly,to);
		for (size_t i=0; i<size; i++)
			heroes[i]->show(to);//show hero list
	}

	for(size_t i=0;i<incomes.size();i++)
		incomes[i]->show(to);//printing resource incomes

	slider->showAll(to);
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,136+116*size+29,pos.x-14,pos.y-15);
	show(to);
}

void CKingdomInterface::show(SDL_Surface * to)
{
	statusbar->show(to);
}

void CKingdomInterface::activate()
{
	LOCPLINT->statusbar = statusbar;
	exit->activate();
	toTowns->activate();
	toHeroes->activate();

	ObjTop->activate();
	ObjUp->activate();
	ObjDown->activate();
	ObjBottom->activate();

	for (size_t i=0; i<ObjList.size(); i++)
		ObjList[i]->activate();

	for (size_t i=0; i<incomes.size(); i++)
		incomes[i]->activate();

	if (state == 1)
		for (int i=0; i<size; i++)
			towns[i]->activate();
	else
		for (int i=0; i<size; i++)
			heroes[i]->activate();

	slider->activate();
}

void CKingdomInterface::deactivate()
{
	exit->deactivate();
	toTowns->deactivate();
	toHeroes->deactivate();

	ObjTop->deactivate();
	ObjUp->deactivate();
	ObjDown->deactivate();
	ObjBottom->deactivate();

	for (size_t i=0; i<ObjList.size(); i++)
		ObjList[i]->deactivate();

	for (size_t i=0; i<incomes.size(); i++)
		incomes[i]->deactivate();

	if (state == 1)
		for (int i=0; i<size; i++)
			towns[i]->deactivate();
	else
		for (int i=0; i<size; i++)
			heroes[i]->deactivate();
	slider->deactivate();
}

void CKingdomInterface::keyPressed(const SDL_KeyboardEvent & key)
{
}

void CKingdomInterface::recreateHeroList(int pos)
{
	std::vector<const CGHeroInstance*> Heroes = LOCPLINT->cb->getHeroesInfo(true);
	int i=0, cnt=0;
	for (int j = 0; ((j<Heroes.size()) && (i<size));j++)
	{
		if (Heroes[j]->inTownGarrison && (!showHarrisoned))//if hero in garrison and we don't show them
		{
			continue;
		}
		if (cnt<pos)//skipping heroes
		{
			cnt++;
			continue;
		}//this hero will be added
		heroes[i]->hero = Heroes[j];
		i++;
	}
	for (i;i<size;i++)//if we still have empty pieces
		heroes[i]->hero = NULL;//empty pic
	GH.totalRedraw();
}

void CKingdomInterface::recreateTownList(int pos)
{
	std::vector<const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(true);
	for(int i=0;i<size;i++)
	{
		if (i+pos<Towns.size())
			towns[i]->town = Towns[i+pos];//replace town
		else
			towns[i]->town = NULL;//only empty pic
	}
	GH.totalRedraw();
}

void CKingdomInterface::listToTowns()
{
	state = 1;
	toHeroes->block(0);
	toTowns->block(2);
	heroPos = slider->value;
	slider->setAmount(LOCPLINT->cb->howManyTowns());
	slider->value=townPos;//moving slider
	recreateTownList(townPos);
	for (size_t i=0;i<size;i++)//TODO:is this loop needed?
	{
		towns[i]->deactivate();
		heroes[i]->activate();
	}
}

void CKingdomInterface::listToHeroes()
{
	state = 2;
	toHeroes->block(2);
	toTowns->block(0);
	townPos = slider->value;
	slider->setAmount(LOCPLINT->cb->howManyHeroes(showHarrisoned));
	slider->value=heroPos;//moving slider
	recreateHeroList(heroPos);
	for (size_t i=0;i<size;i++)//TODO:is this loop needed?
	{
		towns[i]->deactivate();
		heroes[i]->activate();
	}
}

void CKingdomInterface::sliderMoved(int newpos)
{
	if ( state == 1 )//towns
	{
		townPos = newpos;
		recreateTownList(newpos);
	}
	else//heroes
	{
		heroPos = newpos;
		recreateHeroList(newpos);
	}
}

CKingdomInterface::CResIncomePic::CResIncomePic(int RID, CDefEssential * Mines)
{
	used = HOVER;
	recActions = DISPOSE | SHARE_POS;
	resID = RID;
	pos.x += 20 + RID*80;
	pos.y += 31+size*116;
	pos.h = 54;
	pos.w = (resID!=7)?68:136;//gold pile is bigger
	mines = Mines;

	if ( resID != 7)
	{
		MetaString ms;
		ms << std::pair<ui8,ui32>(9,resID);
		ms.toString(hoverText);
	}
	else
		hoverText = CGI->generaltexth->allTexts[255];
	tlog1<<hoverText<<"\n";

	value = 0;
	int resource = resID==7?6:resID;

	for(size_t i = 0; i<CGI->state->map->objects.size(); i++)
	{
		CGObjectInstance* obj = CGI->state->map->objects[i];
		if (obj)
			if (obj->ID == 53 && obj->subID == resource && //this is mine, produce required resource
				CGI->state->currentPlayer == obj->tempOwner )//mine is ours
					value++;
	}
	if (resID == 7)//we need to calculate income of whole kingdom
	{
		value *=1000;// mines = 1000 gold
		std::vector<const CGHeroInstance*> heroes = LOCPLINT->cb->getHeroesInfo(true);
		for(size_t i=0; i<heroes.size();i++)
			switch(heroes[i]->getSecSkillLevel(13))//some heroes may have estates
			{
			case 1: //basic
				value += 125;
				break;
			case 2: //advanced
				value += 250;
				break;
			case 3: //expert
				value += 500;
				break;
			}
		std::vector<const CGTownInstance*> towns = LOCPLINT->cb->getTownsInfo(true);
		for(size_t i=0; i<towns.size();i++)
			value += towns[i]->dailyIncome();
	}
}

CKingdomInterface::CResIncomePic::~CResIncomePic()
{
}

void CKingdomInterface::CResIncomePic::hover(bool on)
{
	if (on)
	{
		LOCPLINT->statusbar->print(hoverText);
	}
	else 
		LOCPLINT->statusbar->clear();
}

void CKingdomInterface::CResIncomePic::show(SDL_Surface * to)
{
	if (resID < 7)//this is not income
		blitAt(mines->ourImages[resID].bitmap,pos.x,pos.y,to);

	std::ostringstream oss;
	oss << value;
	CSDL_Ext::printAtMiddle(oss.str(),pos.x+pos.w/2,pos.y+50,GEOR13,zwykly,to);
}


CKingdomInterface::CTownItem::CTownItem(int num)
{
	recActions = DISPOSE | SHARE_POS;
	numb = num;
	pos.x += 23;
	pos.y += 26+num*116;
	pos.w = 702;
	pos.h = 114;
	town = NULL;
}

CKingdomInterface::CTownItem::~CTownItem()
{
}

void CKingdomInterface::CTownItem::activate()
{
}

void CKingdomInterface::CTownItem::deactivate()
{
}

void CKingdomInterface::CTownItem::show(SDL_Surface * to)
{
	if (!town)
	{//if NULL - print background & exit
		blitAt(slots->ourImages[numb % PicCount].bitmap,pos.x,pos.y,to);
		return;
	}//background
	blitAt(slots->ourImages[PicCount+2].bitmap,pos.x,pos.y,to);
	//town pic/name
	int townPic = town->subID*2;
	if (!town->hasFort())
		townPic += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		townPic++;
	blitAt(graphics->bigTownPic->ourImages[townPic].bitmap,pos.x+5,pos.y+6,to);
	CSDL_Ext::printAt(town->name,pos.x+73,pos.y+7,GEOR13,zwykly,to);
	//fort pic
	townPic = town->fortLevel()-1;
	if (townPic==-1) townPic = 3;
	blitAt(fort->ourImages[townPic].bitmap,pos.x+111,pos.y+31,to);
	//hall pic
	townPic = town->hallLevel();
	blitAt(hall->ourImages[townPic].bitmap,pos.x+69,pos.y+31,to);
	//income pic
	std::ostringstream oss;
	oss << town->dailyIncome();
	CSDL_Ext::printAtMiddle(oss.str(),pos.x+188,pos.y+60,GEOR13,zwykly,to);
//	Creature bonuses/ Creature available texts - need to find text wrapper thingy
//	CSDL_Ext::printAtWR(CGI->generaltexth->allTexts[265],pos.x,pos.y+80,GEOR13,zwykly,to);
//	CSDL_Ext::printTo(CGI->generaltexth->allTexts[266],pos.x+350,pos.y+80,GEOR13,zwykly,to);
	for (int i=0; i<CREATURES_PER_TOWN;i++)
	{//creatures info
		int crid = -1;
		int bid = 30+i;
		if (!vstd::contains(town->builtBuildings,bid))
			continue;

		if (vstd::contains(town->builtBuildings,bid+CREATURES_PER_TOWN))
		{
			crid = town->town->upgradedCreatures[i];
			bid += CREATURES_PER_TOWN;
		}
		else
			crid = town->town->basicCreatures[i];
		//creature growth
		blitAt(graphics->smallImgs[crid],pos.x+56+i*37,pos.y+78,to);
		std::ostringstream oss;
		oss << '+' << town->creatureGrowth(i);
		CSDL_Ext::printTo(oss.str(),pos.x+87+i*37,pos.y+110,GEORM,zwykly,to);
		//creature available
		blitAt(graphics->smallImgs[crid],pos.x+409+i*37,pos.y+78,to);
		std::ostringstream ostrs;
		ostrs << town->creatures[i].first;
		CSDL_Ext::printTo(ostrs.str(),pos.x+440+i*37,pos.y+110,GEORM,zwykly,to);
	}
	const CGHeroInstance * hero = town->garrisonHero;
	int posX = 244;
	for (int i=0;i<2;i++)
	{//heroes info
		if (hero)
		{
			int iter = 0;//portrait
			blitAt(graphics->portraitLarge[hero->portrait],pos.x+posX,pos.y+6,to);
			for(std::map<si32,std::pair<ui32,si32> >::const_iterator
				j=hero->army.slots.begin(); j!=hero->army.slots.end(); j++)
			{//army
				int X = (iter<4)?(pos.x+posX+70+36*iter):(pos.x+posX+88+36*(iter-4));
				int Y = (iter<4)?(pos.y+3):(pos.y+40);
				iter++;
				blitAt(graphics->smallImgs[j->second.first],X,Y,to);
				std::ostringstream creanum;
				creanum << (j->second.second);
				CSDL_Ext::printTo(creanum.str(),X+30,Y+32,GEORM,zwykly,to);
			}
		}
		hero = town->visitingHero;
		posX = 476;
		}
}

CKingdomInterface::CHeroItem::CHeroItem(int num)
{
	recActions = DISPOSE | SHARE_POS;
	numb = num;
	pos.x += 23;
	pos.y += 26+num*116;
	pos.w = 702;
	pos.h = 114;
	hero = NULL;
	artGroup = 0;
}

CKingdomInterface::CHeroItem::~CHeroItem()
{
}

void CKingdomInterface::CHeroItem::show(SDL_Surface * to)
{
	if (!hero)
	{//if we have no hero for this slot - print background & exit
		blitAt(slots->ourImages[numb % PicCount].bitmap,pos.x,pos.y,to);
		return;
	}//print background, different for arts view/backpack mode
	blitAt(slots->ourImages[(artGroup=2)?PicCount:(PicCount+1)].bitmap,pos.x,pos.y,to);
	//text "Artifacts"
	CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[2],pos.x+320,pos.y+55,GEOR13,zwykly,to);
	int X = pos.x+6;//portrait
	blitAt(graphics->portraitLarge[hero->portrait],pos.x+5,pos.y+6,to);
	for(std::map<si32,std::pair<ui32,si32> >::const_iterator
		j=hero->army.slots.begin(); j!=hero->army.slots.end(); j++)
	{//army
		blitAt(graphics->smallImgs[j->second.first],X,pos.y+78,to);
		std::ostringstream creanum;
		creanum << (j->second.second);
		CSDL_Ext::printTo(creanum.str(),X+30,pos.y+110,GEOR13,zwykly,to);
		X+=36;
	}//hero name
	CSDL_Ext::printAt(hero->name,pos.x+73,pos.y+7,GEOR13,zwykly,to);
	for (int i = 0; i<6; i++)
	{//primary skills, mana and exp. pics
		blitAt(graphics->pskillst->ourImages[i].bitmap,(i<4)?(pos.x+78+36*i):(pos.x+539-52*i),
								(i<4)?(pos.y+26):(pos.y+6),to);
		if (i>3) continue;//primary skills text
		std::ostringstream str;
		str << (hero->primSkills[i]);
		CSDL_Ext::printAtMiddle(str.str(),pos.x+95+36*i,pos.y+65,GEOR13,zwykly,to);
	}
	{//luck and morale pics, experience and mana text
		blitAt(graphics->luck30->ourImages[hero->getCurrentLuck()+3].bitmap,pos.x+222,pos.y+30,to);
		blitAt(graphics->morale30->ourImages[hero->getCurrentMorale()+3].bitmap,pos.x+222,pos.y+54,to);
		std::ostringstream str;
		str << (hero->exp);
		CSDL_Ext::printAtMiddle(str.str(),(pos.x+348),(pos.y+31),GEORM,zwykly,to);
		std::ostringstream strnew;
		strnew << (hero->mana)<<"/"<<(hero->manaLimit());
		CSDL_Ext::printAtMiddle(strnew.str(),(pos.x+298),(pos.y+31),GEORM,zwykly,to);
	}
	//hero speciality
	blitAt(graphics->un32->ourImages[hero->subID].bitmap, pos.x+375, pos.y+6, to);

	for(int i=0; i<hero->secSkills.size(); i++)
	{//secondary skills
		int skill = hero->secSkills[i].first,
		    level = hero->secSkills[i].second;
		blitAt(graphics->abils32->ourImages[skill*3+level+2].bitmap,pos.x+411+i*36,pos.y+6,to);
	}

	int iter=0;
	switch (artGroup)
	{//arts
		case 1:iter = 9;//misc. arts, spellbook, war machines
		case 0://equipped arts
			for (int i = iter ; i<iter+9;i++) 
			{
				int artID = hero->getArtAtPos(i);
				if (artID>=0)
					blitAt(graphics->artDefs->ourImages[artID].bitmap,pos.x+268+48*(i%9),pos.y+66,to);
			}
			break;
		case 2://TODO:backpack
			break;
		default: tlog1<<"Unknown artifact group: "<<artGroup<<"\n";
	}
}

void CKingdomInterface::CHeroItem::onArtChange(int newstate)
{
	artGroup = newstate;
}

void CKingdomInterface::CHeroItem::activate()
{
}

void CKingdomInterface::CHeroItem::deactivate()
{
}
