#include "CKingdomInterface.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "../global.h"
#include "CConfigHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CSpellWindow.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"
#include "../lib/map.h"
#include "../lib/NetPacks.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <SDL.h>

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

#define ADVOPT (conf.go()->ac)
CDefEssential* CKingdomInterface::slots;
CDefEssential* CKingdomInterface::fort;
CDefEssential* CKingdomInterface::hall;

CKingdomInterface::CKingdomInterface()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	defActions = SHARE_POS | DISPOSE;
	PicCount = ADVOPT.overviewPics;
	size =     ADVOPT.overviewSize;
	pos.x = screen->w/2 - 400;
	pos.y = screen->h/2 - (68+58*size);
	showHarrisoned = false;//set to true if you want to see garrisoned heroes
	heroPos = townPos = objPos = state = 0;

	bg = BitmapHandler::loadBitmap(ADVOPT.overviewBg);
	graphics->blueToPlayersAdv(bg, LOCPLINT->playerID);

	mines   = CDefHandler::giveDefEss("OVMINES.DEF");
	title   = CDefHandler::giveDefEss("OVTITLE.DEF");
	hall    = CDefHandler::giveDefEss("ITMTL.DEF");
	fort    = CDefHandler::giveDefEss("ITMCL.DEF");
	objPics = CDefHandler::giveDefEss("FLAGPORT.DEF");
	slots   = CDefHandler::giveDefEss("OVSLOT.DEF");

	toHeroes = new AdventureMapButton (CGI->generaltexth->overview[11],"",
		boost::bind(&CKingdomInterface::listToHeroes,this),748,28+size*116,"OVBUTN1.DEF", SDLK_h);
	toHeroes->block(2);

	toTowns = new AdventureMapButton (CGI->generaltexth->overview[12],"",
		boost::bind(&CKingdomInterface::listToTowns,this),748,64+size*116,"OVBUTN6.DEF", SDLK_t);
	toTowns->block(0);

	exit = new AdventureMapButton (CGI->generaltexth->allTexts[600],"",
		boost::bind(&CKingdomInterface::close,this),748,99+size*116,"OVBUTN1.DEF", SDLK_RETURN);
	exit->bitmapOffset = 3;

	statusbar = new CStatusBar(7, 91+size*116,"TSTATBAR.bmp",732);
	resdatabar = new CResDataBar("KRESBAR.bmp",pos.x+3,pos.y+111+size*116,32,2,76,76);

	for(size_t i=0;i<size;i++)//creating empty hero/town lists for input
	{
		heroes.push_back( new CHeroItem(i,this));
		 towns.push_back( new CTownItem(i,this));
	}

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

	for (size_t i=0; i<8; i++)
	{
		incomes.push_back(new HoverableArea());//bottom panel with mines
		incomes[i]->pos = genRect(57,68,pos.x+20+i*80,pos.y+31+size*116);
		incomes[i]->hoverText = CGI->generaltexth->mines[i].first;
	}
	incomes[7]->pos.w = 136;
	incomes[7]->hoverText = CGI->generaltexth->allTexts[255];
	incomesVal+=0,0,0,0,0,0,0,0;//for mines images

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

	for(size_t i = 0; i<CGI->state->map->objects.size(); i++)//getting mines and dwelling (in one pass to make it faster)
	{
		CGObjectInstance* obj = CGI->state->map->objects[i];//current object
		if (obj)
		{
			if (obj->tempOwner == CGI->state->currentPlayer)//if object is our
			{
				std::pair<int,int > curElm = std::pair<int,int >(obj->ID, obj->subID);
				if ( obj->ID == 17 )//dwelling, text is a plural name of a creature
				{
					objList[obj->subID].first += 1;
					objList[obj->subID].second = & CGI->creh->creatures[CGI->objh->cregens[obj->subID]].namePl;
				}
				else if (addObjects.find(curElm) != addObjects.end())
				{//object from addObjects map, text is name of the object
					objList[addObjects[curElm]].first += 1;
					objList[addObjects[curElm]].second = & obj->hoverName;
				}
				else if ( obj->ID == 53 )//TODO: abandoned mines
					incomesVal[obj->subID]+=1;
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

	incomesVal[7] = incomesVal[6]*1000;//gold mines -> total income
	std::vector<const CGHeroInstance*> heroes = LOCPLINT->cb->getHeroesInfo(true);
	for(size_t i=0; i<heroes.size();i++)
		switch(heroes[i]->getSecSkillLevel(13))//some heroes may have estates
		{
		case 1: //basic
			incomesVal[7] += 125;
			break;
		case 2: //advanced
			incomesVal[7] += 250;
			break;
		case 3: //expert
			incomesVal[7] += 500;
			break;
		}
	std::vector<const CGTownInstance*> towns = LOCPLINT->cb->getTownsInfo(true);
	for(size_t i=0; i<towns.size();i++)
		incomesVal[7] += towns[i]->dailyIncome();
}

void CKingdomInterface::moveObjectList(int newPos)
{
	int top =    objList.size() > objSize ? 0 : objList.size() - objSize ;
	int bottom = objList.size() > objSize ? objList.size() - objSize : 0 ;
	switch (newPos)//checking what button was pressed
	{
/*  Top */	case 0: objPos = top;
			break;
/*  Up  */	case 1: objPos = objPos==top?top:(objPos-1);
			break;
/* Down */	case 2: objPos = objPos==bottom?bottom:(objPos+1);
			break;
/*Bottom*/	case 3: objPos = bottom;
			break;
	}
	showAll(screen2);
}

CKingdomInterface::~CKingdomInterface()
{
	SDL_FreeSurface(bg);

	delete title;//deleting .def
	delete slots;
	delete fort;
	delete hall;
	delete objPics;
	delete mines;

	towns.clear();//deleting lists
	heroes.clear();
	incomes.clear();
	ObjList.clear();
	objList.clear();
}

void CKingdomInterface::close()
{
	GH.popIntTotally(this);
}

void CKingdomInterface::updateAllGarrisons()
{
	for (int i = 0; i<towns.size(); i++)
	{
		if (towns[i] && towns[i]->garr)
			towns[i]->garr->recreateSlots();
	}
	for (int i = 0; i<heroes.size(); i++)
	{
		if (heroes[i] && heroes[i]->garr)
			heroes[i]->garr->recreateSlots();
	}
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
		if (skipCount<objPos)//we will show only objects from objPos
		{
			skipCount++;
			continue;
		}
		blitAt(objPics->ourImages[(*it).first].bitmap,pos.x+740,pos.y+44+curPos*57,to);

		std::ostringstream ostrs;//objects count
		ostrs << (*it).second.first;
		CSDL_Ext::printTo(ostrs.str(),pos.x+790,pos.y+94+curPos*57,FONT_SMALL,zwykly,to);

		ObjList[curPos]->hoverText = * (*it).second.second;
		curPos++;
		if (curPos == objSize)
			break;
	}

	if (state == 1)
	{//printing text "Town", "Harrisoned hero", "Visiting hero"
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[3],pos.x+144,pos.y+14,FONT_MEDIUM,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[4],pos.x+373,pos.y+14,FONT_MEDIUM,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[5],pos.x+606,pos.y+14,FONT_MEDIUM,zwykly,to);
		for (size_t i=0; i<size; i++)
			towns[i]->showAll(to);//show town list
	}
	else
	{//text "Hero/stats" and "Skills"
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[0],pos.x+150,pos.y+14,FONT_MEDIUM,zwykly,to);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[1],pos.x+500,pos.y+14,FONT_MEDIUM,zwykly,to);
		for (size_t i=0; i<size; i++)
			heroes[i]->showAll(to);//show hero list
	}
	for (int i = 0; i<7; i++)
		blitAt(mines->ourImages[i].bitmap,pos.x + 20 + i*80,pos.y + 31+size*116,to);

	for(size_t i=0;i<incomes.size();i++)
	{
		std::ostringstream oss;
		oss << incomesVal[i];
		CSDL_Ext::printAtMiddle(oss.str(),incomes[i]->pos.x+incomes[i]->pos.w/2,incomes[i]->pos.y+50,FONT_SMALL,zwykly,to);
	}

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
	GH.statusbar = statusbar;
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
		for (size_t i=0; i<size; i++)
			towns[i]->activate();
	else
		for (size_t i=0; i<size; i++)
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
		for (size_t i=0; i<size; i++)
			towns[i]->deactivate();
	else
		for (size_t i=0; i<size; i++)
			heroes[i]->deactivate();
	slider->deactivate();
}

void CKingdomInterface::recreateHeroList(int pos)
{
	std::vector<const CGHeroInstance*> Heroes = LOCPLINT->cb->getHeroesInfo(true);
	int i=0, cnt=0;
	for (size_t j = 0; ((j<Heroes.size()) && (i<size));j++)
	{
		if (Heroes[j]->inTownGarrison && (!showHarrisoned))//if hero in garrison and we don't show them
			continue;
		if (cnt<pos)//skipping heroes
		{
			cnt++;
			continue;
		}//this hero will be added
		heroes[i]->setHero(Heroes[j]);
		i++;
	}
	for (i;i<size;i++)//if we still have empty pieces
		heroes[i]->setHero(NULL);//empty pic
}

void CKingdomInterface::recreateTownList(int pos)
{
	std::vector<const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(false);
	for(size_t i=0;i<size;i++)
	{
		if (i+pos<LOCPLINT->cb->howManyTowns())
			towns[i]->setTown(Towns[i+pos]);//replace town
		else
			towns[i]->setTown(NULL);//only empty pic
	}
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
	for (size_t i=0;i<size;i++)
	{
		heroes[i]->deactivate();
		towns[i]->activate();
	}
	showAll(screen2);
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
	for (size_t i=0;i<size;i++)
	{
		towns[i]->deactivate();
		heroes[i]->activate();
	}
	showAll(screen2);
}

void CKingdomInterface::sliderMoved(int newpos)
{
	if (state == 0)
	{
		townPos = newpos;
		recreateHeroList(newpos);
		state = 2;
	}
	else if ( state == 1 )//towns
	{
		for (size_t i=0; i<size; i++)
			towns[i]->deactivate();
		townPos = newpos;
		recreateTownList(newpos);
		for (size_t i=0; i<size; i++)
			towns[i]->activate();
		showAll(screen2);
	}
	else//heroes
	{
		for (size_t i=0; i<size; i++)
			heroes[i]->deactivate();
		heroPos = newpos;
		recreateHeroList(newpos);
		for (size_t i=0; i<size; i++)
			heroes[i]->activate();
		showAll(screen2);
	}
}

CKingdomInterface::CTownItem::CTownItem(int num, CKingdomInterface * Owner)
{
	recActions = DISPOSE | SHARE_POS;
	owner = Owner;
	numb = num;
	pos.x += 23;
	pos.y += 25+num*116;
	pos.w = 702;
	pos.h = 114;
	town = NULL;
	garr = NULL;

	garrHero = new LRClickableAreaOpenHero();
	garrHero->pos = genRect(64, 58, pos.x+244, pos.y + 6);

	visitHero = new LRClickableAreaOpenHero();
	visitHero->pos = genRect(64, 58, pos.x+476, pos.y + 6);

	for (int i=0; i<CREATURES_PER_TOWN;i++)
	{//creatures info
		creaGrowth.push_back(new HoverableArea());
		creaGrowth[i]->pos = genRect(32, 32, pos.x+56+i*37, pos.y + 78);

		creaCount.push_back(new CCreaPlace());
		creaCount[i]->pos = genRect(32, 32, pos.x+409+i*37, pos.y + 78);
		creaCount[i]->type = i;
	}
	hallArea = new HoverableArea();
	hallArea->pos = genRect(38, 38, pos.x+69, pos.y + 31);

	fortArea = new HoverableArea();
	fortArea->pos = genRect(38, 38, pos.x+111, pos.y + 31);

	townImage = new LRClickableAreaOpenTown();
	townImage->pos = genRect(64, 58, pos.x+5, pos.y + 6);

	incomeArea = new HoverableArea();
	incomeArea->pos = genRect(42, 64, pos.x+154, pos.y + 31);
	incomeArea->hoverText = CGI->generaltexth->allTexts[255];

}

CKingdomInterface::CTownItem::~CTownItem()
{
	creaGrowth.clear();
	creaCount.clear();
	delete garr;
}

void CKingdomInterface::CTownItem::setTown(const CGTownInstance * newTown)
{
	BLOCK_CAPTURING;
	delete garr;
	town = newTown;
	if (!town)
	{
		return;
		garr = NULL;
	}
	garr = new CGarrisonInt(pos.x+313,pos.y+3,4,Point(232,0),slots->ourImages[owner->PicCount+2].bitmap,Point(313,2),town,town->visitingHero,true,true, 4,Point(-126,37));
	garr->update = true;

	char buf[400];
	if (town->garrisonHero)
		{
		garrHero->hero = town->garrisonHero;
		sprintf(buf, CGI->generaltexth->allTexts[15].c_str(), town->garrisonHero->name.c_str(), town->garrisonHero->type->heroClass->name.c_str());
		garrHero->hoverText = buf;
		}
	if (town->visitingHero)
		{
		visitHero->hero = town->visitingHero;
		sprintf(buf, CGI->generaltexth->allTexts[15].c_str(), town->visitingHero->name.c_str(), town->visitingHero->type->heroClass->name.c_str());
		visitHero->hoverText = buf;
		}

	for (int i=0; i<CREATURES_PER_TOWN;i++)
	{
		creaCount[i]->town = NULL;

		int crid = -1;

		if (vstd::contains(town->builtBuildings,30+i+CREATURES_PER_TOWN))
			crid = town->town->upgradedCreatures[i];
		else
			crid = town->town->basicCreatures[i];

		std::string descr=CGI->generaltexth->allTexts[588];
		boost::algorithm::replace_first(descr,"%s",CGI->creh->creatures[crid].namePl);
		creaGrowth[i]->hoverText = descr;

		descr=CGI->generaltexth->heroscrn[1];
		boost::algorithm::replace_first(descr,"%s",CGI->creh->creatures[crid].namePl);
		creaCount[i]->hoverText = descr;
		creaCount[i]->town = town;
	}

	townImage->hoverText = town->name;
	townImage->town = town;

	hallArea->hoverText = CGI->buildh->buildings[town->subID][10+town->hallLevel()]->Name();
	if (town->hasFort())
		fortArea->hoverText = CGI->buildh->buildings[town->subID][6+town->fortLevel()]->Name();
	else
		fortArea->hoverText = "";
}

void CKingdomInterface::CTownItem::activate()
{
	if (!town)
		return;

	setTown(town);
	hallArea->activate();
	fortArea->activate();
	incomeArea->activate();
	townImage->activate();

	if (town->garrisonHero)
		garrHero->activate();
	if (town->visitingHero)
		visitHero->activate();

	for (int i=0; i<CREATURES_PER_TOWN;i++)
		if (vstd::contains(town->builtBuildings,30+i))
		{
			creaGrowth[i]->activate();
			creaCount [i]->activate();
		}
	garr->activate();
}

void CKingdomInterface::CTownItem::deactivate()
{
	if (!town)
		return;

	hallArea->deactivate();
	fortArea->deactivate();
	incomeArea->deactivate();
	townImage->deactivate();

	if (town->garrisonHero)
		garrHero->deactivate();
	if (town->visitingHero)
		visitHero->deactivate();

	for (int i=0; i<CREATURES_PER_TOWN;i++)
		if (vstd::contains(town->builtBuildings,30+i))
		{
			creaGrowth[i]->deactivate();
			creaCount [i]->deactivate();
		}	garr->deactivate();
}

void CKingdomInterface::CTownItem::showAll(SDL_Surface * to)
{
	if (!town)
	{//if NULL - print background & exit
		blitAt(slots->ourImages[numb % owner->PicCount].bitmap,pos.x,pos.y,to);
		return;
	}//background
	blitAt(slots->ourImages[owner->PicCount+2].bitmap,pos.x,pos.y,to);	garr->show(to);
	//town pic/name
	int townPic = town->subID*2;
	if (!town->hasFort())
		townPic += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		townPic++;
	blitAt(graphics->bigTownPic->ourImages[townPic].bitmap,pos.x+5,pos.y+6,to);
	CSDL_Ext::printAt(town->name,pos.x+73,pos.y+8,FONT_SMALL,zwykly,to);
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
	CSDL_Ext::printAtMiddle(oss.str(),pos.x+189,pos.y+61,FONT_SMALL,zwykly,to);

	std::vector<std::string> * toPrin = CMessage::breakText(CGI->generaltexth->allTexts[265]);

	CSDL_Ext::printAt((*toPrin)[0], pos.x+4, pos.y+76, FONT_SMALL, tytulowy, to);
	if(toPrin->size()!=1)
		CSDL_Ext::printAt((*toPrin)[1], pos.x+4, pos.y+92, FONT_SMALL, tytulowy, to);

	delete toPrin;
	toPrin = CMessage::breakText(CGI->generaltexth->allTexts[266]);

	CSDL_Ext::printAt((*toPrin)[0], pos.x+351, pos.y+76, FONT_SMALL, tytulowy, to);
	if(toPrin->size()!=1)
		CSDL_Ext::printAt((*toPrin)[1], pos.x+351, pos.y+92, FONT_SMALL, tytulowy, to);
	delete toPrin;

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
		CSDL_Ext::printTo(oss.str(),pos.x+87+i*37,pos.y+110,FONT_TINY,zwykly,to);
		//creature available
		blitAt(graphics->smallImgs[crid],pos.x+409+i*37,pos.y+78,to);
		std::ostringstream ostrs;
		ostrs << town->creatures[i].first;
		CSDL_Ext::printTo(ostrs.str(),pos.x+440+i*37,pos.y+110,FONT_TINY,zwykly,to);
	}


	if (town->garrisonHero)
		blitAt(graphics->portraitLarge[town->garrisonHero->portrait],pos.x+244,pos.y+6,to);

	if (town->visitingHero)
		blitAt(graphics->portraitLarge[town->visitingHero->portrait],pos.x+476,pos.y+6,to);
}

CKingdomInterface::CHeroItem::CHeroItem(int num, CKingdomInterface * Owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	recActions = DISPOSE | SHARE_POS;
	defActions = SHARE_POS;
	owner = Owner;
	numb = num;
	pos.x += 23;
	pos.y += 25+num*116;
	pos.w = 702;
	pos.h = 114;
	hero = NULL;
	garr = NULL;
	artGroup = 0;
	backpackPos = 0;
	artButtons = new CHighlightableButtonsGroup(0);
	for (size_t it = 0; it<3; it++)
	{
		artButtons->addButton(boost::assign::map_list_of(0,CGI->generaltexth->overview[13+it]),
			CGI->generaltexth->overview[8+it], "OVBUTN3.DEF",364+it*112, 46, it);
		std::string str = CGI->generaltexth->overview[8+it];
		str = str.substr(str.find_first_of("{")+1, str.find_first_of("}")-str.find_first_of("{"));
		artButtons->buttons[it]->addTextOverlay(str, FONT_SMALL, tytulowy);
	}
	artButtons->onChange = boost::bind(&CKingdomInterface::CHeroItem::onArtChange, this, _1);
	artButtons->select(0,0);

	artLeft = new AdventureMapButton("", "", boost::bind
		(&CKingdomInterface::CHeroItem::scrollArts,this,-1), 269, 66, "hsbtns3.def", SDLK_LEFT);
	artRight = new AdventureMapButton("", "", boost::bind
		(&CKingdomInterface::CHeroItem::scrollArts,this,+1), 675, 66, "hsbtns5.def", SDLK_RIGHT);

	portrait = new LRClickableAreaOpenHero();
	portrait->pos = genRect(64, 58, pos.x+5, pos.y + 5);
	char bufor[400];
	for(int i=0; i<PRIMARY_SKILLS; i++)
	{
		primarySkills.push_back(new LRClickableAreaWTextComp());
		primarySkills[i]->pos = genRect(45, 32, pos.x+77 + 36*i, pos.y+26);
		primarySkills[i]->text = CGI->generaltexth->arraytxt[2+i];
		primarySkills[i]->type = i;
		primarySkills[i]->baseType = 0;
		sprintf(bufor, CGI->generaltexth->heroscrn[1].c_str(), CGI->generaltexth->primarySkillNames[i].c_str());
		primarySkills[i]->hoverText = std::string(bufor);
	};
	experience = new LRClickableAreaWText();
	experience->pos = genRect(33, 49, pos.x+322, pos.y+5);
	experience->hoverText = CGI->generaltexth->heroscrn[9];

	morale = new LRClickableAreaWTextComp();
	morale->pos = genRect(20,32,pos.x+221,pos.y+52);

	luck = new LRClickableAreaWTextComp();
	luck->pos = genRect(20,32,pos.x+221,pos.y+28);

	spellPoints = new LRClickableAreaWText();
	spellPoints->pos = genRect(33, 49, pos.x+270, pos.y+5);
	spellPoints->hoverText = CGI->generaltexth->heroscrn[22];

	speciality = new LRClickableAreaWText();
	speciality->pos = genRect(32, 32, pos.x+374, pos.y+5);
	speciality->hoverText = CGI->generaltexth->heroscrn[27];

	for(int i=0; i<SKILL_PER_HERO; ++i)
	{
		secondarySkills.push_back(new LRClickableAreaWTextComp());
		secondarySkills[i]->pos = genRect(32, 32, pos.x+410+i*37, pos.y+5);
		secondarySkills[i]->baseType = 1;
	};

	for (int i=0; i<18;i++)
	{
		artifacts.push_back(new CArtPlace(this));
		artifacts[i]->pos = genRect(44, 44, pos.x+268+(i%9)*48, pos.y+66);
		artifacts[i]->baseType = SComponent::artifact;
	};

	for (int i=0; i<8;i++)
	{
		backpack.push_back(new CArtPlace(this));
		backpack[i]->pos = genRect(44, 44, pos.x+293+(i%9)*48, pos.y+66);
		backpack[i]->baseType = SComponent::artifact;
	};

}

CKingdomInterface::CHeroItem::~CHeroItem()
{
	delete garr;
	delete artButtons;
	primarySkills.clear();
	secondarySkills.clear();
	artifacts.clear();
	backpack.clear();
}

void CKingdomInterface::CHeroItem::setHero(const CGHeroInstance * newHero)
{
	BLOCK_CAPTURING;
	delete garr;
	hero = newHero;
	if (!hero)
	{
		return;
		garr = NULL;
	}
	char bufor[4000];
	artLeft->block(hero->artifacts.size() <= 8);
	artRight->block(hero->artifacts.size() <= 8);
	garr = new CGarrisonInt(pos.x+6, pos.y+78, 4, Point(), slots->ourImages[owner->PicCount].bitmap,
		Point(6,78), hero, NULL, true, true);

	for (int i=0; i<artifacts.size(); i++)
	{
		artifacts[i]->type = hero->getArtAtPos(i);
		if (artifacts[i]->type<0)
			artifacts[i]->hoverText = CGI->generaltexth->heroscrn[11];
		else
		{
			artifacts[i]->text = CGI->generaltexth->artifDescriptions[artifacts[i]->type];
			artifacts[i]->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1].c_str()) % CGI->arth->artifacts[artifacts[i]->type].Name());
		}
	}

	for (int i=0; i<backpack.size(); i++)
	{
		backpack[i]->type = hero->getArtAtPos(19+i);
		if (backpack[i]->type<0)
			backpack[i]->hoverText ="";
		else
		{
			backpack[i]->text = CGI->generaltexth->artifDescriptions[backpack[i]->type];
			backpack[i]->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1].c_str()) % CGI->arth->artifacts[backpack[i]->type].Name());
		}
	}

	sprintf(bufor, CGI->generaltexth->allTexts[15].c_str(), hero->name.c_str(), hero->type->heroClass->name.c_str());
	portrait->hoverText = std::string(bufor);
	portrait->hero = hero;

	speciality->text = CGI->generaltexth->hTxts[hero->subID].longBonus;

	//primary skills
	for(size_t g=0; g<primarySkills.size(); ++g)
		primarySkills[g]->bonus = hero->getPrimSkillLevel(g);

	//secondary skills
	for(size_t g=0; g<std::min(secondarySkills.size(),hero->secSkills.size()); ++g)
	{
		int skill = hero->secSkills[g].first, 
		    level = hero->secSkills[g].second;
		secondarySkills[g]->type = skill;
		secondarySkills[g]->bonus = level;
		secondarySkills[g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];
		sprintf(bufor, CGI->generaltexth->heroscrn[21].c_str(), CGI->generaltexth->levels[level-1].c_str(), CGI->generaltexth->skillName[skill].c_str());
		secondarySkills[g]->hoverText = std::string(bufor);
	}
	//experience
	experience->text = CGI->generaltexth->allTexts[2].c_str();
	boost::replace_first(experience->text, "%d", boost::lexical_cast<std::string>(hero->level));
	boost::replace_first(experience->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(hero->level+1)));
	boost::replace_first(experience->text, "%d", boost::lexical_cast<std::string>(hero->exp));

	//spell points
	sprintf(bufor, CGI->generaltexth->allTexts[205].c_str(), hero->name.c_str(), hero->mana, hero->manaLimit());
	spellPoints->text = std::string(bufor);

	//setting morale
	std::vector<std::pair<int,std::string> > mrl = hero->getCurrentMoraleModifiers();
	int mrlv = hero->getCurrentMorale();
	int mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad morale, 0 - neutral, 1 - good
	morale->hoverText = CGI->generaltexth->heroscrn[4 - mrlt];
	morale->baseType = SComponent::morale;
	morale->bonus = mrlv;
	morale->text = CGI->generaltexth->arraytxt[88];
	boost::algorithm::replace_first(morale->text,"%s",CGI->generaltexth->arraytxt[86-mrlt]);
	if (!mrl.size())
		morale->text += CGI->generaltexth->arraytxt[108];
	else
		for(int it=0; it < mrl.size(); it++)
			morale->text += "\n" + mrl[it].second;

	//setting luck
	mrl = hero->getCurrentLuckModifiers();
	mrlv = hero->getCurrentLuck();
	mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad luck, 0 - neutral, 1 - good
	luck->hoverText = CGI->generaltexth->heroscrn[7 - mrlt];
	luck->baseType = SComponent::luck;
	luck->bonus = mrlv;
	luck->text = CGI->generaltexth->arraytxt[62];
	boost::algorithm::replace_first(luck->text,"%s",CGI->generaltexth->arraytxt[60-mrlt]);
	if (!mrl.size())
		luck->text += CGI->generaltexth->arraytxt[77];
	else
		for(int it=0; it < mrl.size(); it++)
			luck->text += "\n" + mrl[it].second;
}

void CKingdomInterface::CHeroItem::scrollArts(int move)
{
	backpackPos = ( backpackPos + move + hero->artifacts.size()) % hero->artifacts.size();
	for (int i=0; i<backpack.size(); i++)
	{
		backpack[i]->type = hero->getArtAtPos(19+(backpackPos + i)%hero->artifacts.size());
		if (backpack[i]->type<0)
			backpack[i]->hoverText ="";
		else
		{
			backpack[i]->text = CGI->generaltexth->artifDescriptions[backpack[i]->type];
			backpack[i]->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1].c_str()) % CGI->arth->artifacts[backpack[i]->type].Name());
		}
	}
	showAll(screen2);
}

void CKingdomInterface::CHeroItem::showAll(SDL_Surface * to)
{
	if (!hero)
	{//if we have no hero for this slot - print background & exit
		blitAt(slots->ourImages[numb % owner->PicCount].bitmap,pos.x,pos.y,to);
		return;
	}//print background, different for arts view/backpack mode
	blitAt(slots->ourImages[(artGroup!=2)?owner->PicCount:(owner->PicCount+1)].bitmap,pos.x,pos.y,to);
	//text "Artifacts"
	CSDL_Ext::printAtMiddle(CGI->generaltexth->overview[2],pos.x+320,pos.y+55,FONT_SMALL,zwykly,to);
	blitAt(graphics->portraitLarge[hero->portrait],pos.x+5,pos.y+6,to);

	garr->show(to);
	//hero name
	CSDL_Ext::printAt(hero->name,pos.x+73,pos.y+7,FONT_SMALL,zwykly,to);
	for (int i = 0; i<6; i++)
	{//primary skills, mana and exp. pics
		blitAt(graphics->pskillst->ourImages[i].bitmap,(i<4)?(pos.x+78+36*i):(pos.x+539-52*i),
								(i<4)?(pos.y+26):(pos.y+6),to);
		if (i>3) continue;//primary skills text
		std::ostringstream str;
		str << (hero->primSkills[i]);
		CSDL_Ext::printAtMiddle(str.str(),pos.x+94+36*i,pos.y+66,FONT_SMALL,zwykly,to);
	}
	{//luck and morale pics, experience and mana text
		blitAt(graphics->luck30->ourImages[hero->getCurrentLuck()+3].bitmap,pos.x+222,pos.y+30,to);
		blitAt(graphics->morale30->ourImages[hero->getCurrentMorale()+3].bitmap,pos.x+222,pos.y+54,to);
		std::ostringstream str;
		str << (hero->exp);
		CSDL_Ext::printAtMiddle(str.str(),(pos.x+348),(pos.y+31),FONT_TINY,zwykly,to);
		std::ostringstream strnew;
		strnew << (hero->mana)<<"/"<<(hero->manaLimit());
		CSDL_Ext::printAtMiddle(strnew.str(),(pos.x+295),(pos.y+30),FONT_TINY,zwykly,to);
	}
	//hero speciality
	blitAt(graphics->un32->ourImages[hero->subID].bitmap, pos.x+375, pos.y+6, to);

	for(int i=0; i<hero->secSkills.size(); i++)
	{//secondary skills
		int skill = hero->secSkills[i].first,
		    level = hero->secSkills[i].second;
		blitAt(graphics->abils32->ourImages[skill*3+level+2].bitmap,pos.x+411+i*36,pos.y+6,to);
	}

	artButtons->show(to);

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
		case 2:
			artLeft->show(to);
			artRight->show(to);
			int max = hero->artifacts.size();
			iter = std::min(8, max);
			for (size_t it = 0 ; it<iter;it++)
				blitAt(graphics->artDefs->ourImages[hero->artifacts[(it+backpackPos)%max]].bitmap,pos.x+293+48*it,pos.y+66,to);
			break;
	}
	show(to);
}

void CKingdomInterface::CHeroItem::onArtChange(int newstate)
{
	if (!hero)
		return;
	deactivate();
	artGroup = newstate;
	activate();
	showAll(screen2);
}

void CKingdomInterface::CHeroItem::activate()
{
	setHero(hero);
	if (!hero)
		return;
	artButtons->activate();
	garr->activate();
	if ( artGroup == 2 )
	{
		artLeft->activate();
		artRight->activate();
		for (size_t i=0; i<8;i++)
			backpack[i]->activate();
	}
	else
	{
		for (size_t i=artGroup*9; i<9+artGroup*9;i++)
			artifacts[i]->activate();
	}
	portrait->activate();
	experience->activate();
	morale->activate();
	luck->activate();
	spellPoints->activate();
	speciality->activate();

	for (size_t i=0; i<primarySkills.size();i++)
		primarySkills[i]->activate();

	for (size_t i=0; i<std::min(secondarySkills.size(),hero->secSkills.size());i++)
		secondarySkills[i]->activate();
}

void CKingdomInterface::CHeroItem::deactivate()
{
	if (!hero)
		return;
	artButtons->deactivate();
	garr->deactivate();
	if ( artGroup == 2 )
	{
		artLeft->deactivate();
		artRight->deactivate();
		for (size_t i=0; i<8;i++)
			backpack[i]->deactivate();
	}
	else
	{
		for (size_t i=artGroup*9; i<9+artGroup*9;i++)
			artifacts[i]->deactivate();
	}

	portrait->deactivate();
	experience->deactivate();
	morale->deactivate();
	luck->deactivate();
	spellPoints->deactivate();
	speciality->deactivate();
	for (size_t i=0; i<primarySkills.size();i++)
		primarySkills[i]->deactivate();

	for (size_t i=0; i<std::min(secondarySkills.size(),hero->secSkills.size());i++)
		secondarySkills[i]->deactivate();
}

CKingdomInterface::CHeroItem::CArtPlace::CArtPlace(CHeroItem * owner)
{
	hero = owner;
	used = LCLICK | RCLICK | HOVER;
}

void CKingdomInterface::CHeroItem::CArtPlace::activate()
{
	LRClickableAreaWTextComp::activate();
}

void CKingdomInterface::CHeroItem::CArtPlace::clickLeft(tribool down, bool previousState)
{
	if (!down && previousState && type>=0)
	{
		if(type == 0)
		{
			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), hero->hero);
			GH.pushInt(spellWindow);
		}
		else
			LRClickableAreaWTextComp::clickLeft(down,previousState);
	}
}

void CKingdomInterface::CHeroItem::CArtPlace::clickRight(tribool down, bool previousState)
{
	if (type>=0)
		LRClickableAreaWTextComp::clickRight(down, previousState);
}

void CKingdomInterface::CHeroItem::CArtPlace::deactivate()
{
		LRClickableAreaWTextComp::deactivate();
}


CKingdomInterface::CTownItem::CCreaPlace::CCreaPlace()
{
	town = NULL;
	used = LCLICK | RCLICK | HOVER;
}

void CKingdomInterface::CTownItem::CCreaPlace::activate()
{
	LRClickableAreaWTextComp::activate();
}

void CKingdomInterface::CTownItem::CCreaPlace::clickLeft(tribool down, bool previousState)
{
	if (!down && previousState && town)
	{
		GH.pushInt (new CRecruitmentWindow(town, type, town, boost::bind
			(&CCallback::recruitCreatures,LOCPLINT->cb,town,_1,_2)));
	}
}

void CKingdomInterface::CTownItem::CCreaPlace::clickRight(tribool down, bool previousState)
{
	if (down && town)
	{
		int crid;
		if (town->builtBuildings.find(30+type+CREATURES_PER_TOWN)!=town->builtBuildings.end())
			crid = town->town->upgradedCreatures[type];
		else
			crid = town->town->basicCreatures[type];
		GH.pushInt(new CCreInfoWindow(crid, 0, town->creatures[type].first, NULL, 0, 0, NULL));
	}
}

void CKingdomInterface::CTownItem::CCreaPlace::deactivate()
{
		LRClickableAreaWTextComp::deactivate();
}
