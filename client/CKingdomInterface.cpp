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

int PicCount = 4;

CDefEssential* CKingdomInterface::slots;
CDefEssential* CKingdomInterface::fort;
CDefEssential* CKingdomInterface::hall;

CKingdomInterface::CKingdomInterface()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	defActions = SHARE_POS;
	pos.x = screen->w/2 - 400;
	pos.y = screen->h/2 - 300;
	size = 4;//we have 4 visible items in the list, would be nice to move this value to configs later
	heroPos = townPos = 0;
	state = 2;
	showHarrisoned = false;

	bg = BitmapHandler::loadBitmap("OVCAST.bmp");
	graphics->blueToPlayersAdv(bg, LOCPLINT->playerID);
	mines = CDefHandler::giveDefEss("OVMINES.DEF");
	slots = CDefHandler::giveDefEss("OVSLOT.DEF");
	title = CDefHandler::giveDefEss("OVTITLE.DEF");
	hall = CDefHandler::giveDefEss("ITMTL.DEF");
	fort = CDefHandler::giveDefEss("ITMCL.DEF");

	toHeroes = new AdventureMapButton (CGI->generaltexth->overview[11],"",
		boost::bind(&CKingdomInterface::listToHeroes,this),748,492,"OVBUTN1.DEF");
	toHeroes->block(2);

	toTowns = new AdventureMapButton (CGI->generaltexth->overview[12],"",
		boost::bind(&CKingdomInterface::listToTowns,this),748,528,"OVBUTN6.DEF");
	toTowns->block(0);

	exit = new AdventureMapButton (CGI->generaltexth->allTexts[600],"",
		boost::bind(&CKingdomInterface::close,this),748,563,"OVBUTN1.DEF");
	exit->bitmapOffset = 3;

	statusbar = new CStatusBar(pos.x+7,pos.y+555,"TSTATBAR.bmp",732);
	resdatabar = new CResDataBar("KRESBAR.bmp",pos.x+3,pos.y+575,32,2,76,76);

	for (int i=0; i<RESOURCE_QUANTITY; i++)
		incomes.push_back(new CResIncomePic(i,mines));

	heroes.resize(size);
	for(size_t i=0;i<size;i++)//preparing lists for input
		heroes[i] = NULL;
	towns.resize(size);
	for(size_t i=0;i<size;i++)
		towns[i] = NULL;

	slider = new CSlider(4, 4, 483, boost::bind (&CKingdomInterface::sliderMoved, this, _1),
		size, LOCPLINT->cb->howManyHeroes(showHarrisoned), 0, false, 0);
}

CKingdomInterface::~CKingdomInterface()
{
	SDL_FreeSurface(bg);

	delete statusbar;
	delete resdatabar;

	delete exit;
	delete toTowns;
	delete toHeroes;

	delete slider;
	delete title;
	delete slots;
	delete fort;
	delete hall;
	delete mines;

/*	for(size_t i=0;i<size;i++)
		delete heroes[i];
	heroes.clear();
	for(size_t i=0;i<size;i++)
		delete towns[i];
	towns.clear();*/
	for(size_t i=0;i<incomes.size();i++)
		delete incomes[i];
	incomes.clear();
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

	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
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
	for (int j=0; j<size; j++)
		delete heroes[j];//removing old list
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
		heroes[i] = new CHeroItem(i, Heroes[j]);
		i++;
	}
	for (i;i<size;i++)//if we still have empty pieces
		heroes[i] = new CHeroItem(i, NULL);//empty pic
	GH.totalRedraw();
}

void CKingdomInterface::recreateTownList(int pos)
{
	std::vector<const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(true);
	for(int i=0;i<size;i++)
	{
		delete towns[i];//remove old
		if (i+pos<Towns.size())
			towns[i] = new CTownItem(i, Towns[i+pos]);//and add new
		else
			towns[i] = new CTownItem(i, NULL);//empty pic
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
	resID = RID;
	pos.x += 20 + RID*80;
	pos.y += 495;
	pos.h = 54;
	pos.w = (resID!=7)?68:136;//gold pile is bigger
	mines = Mines;

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
}

void CKingdomInterface::CResIncomePic::show(SDL_Surface * to)
{
	if (resID < 7)//this is not income
		blitAt(mines->ourImages[resID].bitmap,pos.x,pos.y,to);

	std::ostringstream oss;
	oss << value;
	CSDL_Ext::printAtMiddle(oss.str(),pos.x+pos.w/2,pos.y+50,GEOR13,zwykly,to);
}


CKingdomInterface::CTownItem::CTownItem(int num, const CGTownInstance * Town)
{
//	defActions = ACTIVATE | DEACTIVATE | SHOWALL | DISPOSE;
	numb = num;
	pos.x = screen->w/2 - 400 + 23;
	pos.y = screen->h/2 - 300 + 26+num*116;
	pos.w = 702;
	pos.h = 114;
	town = Town;
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
	blitAt(slots->ourImages[6].bitmap,pos.x,pos.y,to);
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

CKingdomInterface::CHeroItem::CHeroItem(int num, const CGHeroInstance * Hero)
{
	numb = num;
	pos.x = screen->w/2 - 400 + 23;
	pos.y = screen->h/2 - 300 + 26+num*116;
	pos.w = 702;
	pos.h = 114;
	hero = Hero;
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
	blitAt(slots->ourImages[(artGroup=2)?4:5].bitmap,pos.x,pos.y,to);
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
