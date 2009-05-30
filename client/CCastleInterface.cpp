#include "../stdafx.h"
#include "CCastleInterface.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CTownHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <sstream>
using namespace boost::assign;
using namespace CSDL_Ext;

/*
 * CCastleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern TTF_Font * GEOR16;
CBuildingRect::CBuildingRect(Structure *Str)
	:moi(false), offset(0), str(Str)
{
	def = CDefHandler::giveDef(Str->defName);
	max = def->ourImages.size();

	if(str->ID == 33    &&    str->townID == 4) //little 'hack' for estate in necropolis - background color is not always the first color in the palette
	{
		for(std::vector<Cimage>::iterator i=def->ourImages.begin();i!=def->ourImages.end();i++)
		{
			SDL_SetColorKey(i->bitmap,SDL_SRCCOLORKEY,*((char*)i->bitmap->pixels));
		}
	}

	pos.x = str->pos.x + LOCPLINT->castleInt->pos.x;
	pos.y = str->pos.y + LOCPLINT->castleInt->pos.y;
	pos.w = def->ourImages[0].bitmap->w;
	pos.h = def->ourImages[0].bitmap->h;
	if(Str->ID<0  || (Str->ID>=27 && Str->ID<=29))
	{
		area = border = NULL;
		return;
	}
	if (border = BitmapHandler::loadBitmap(str->borderName)) //FIXME hmmm if '=' use () else '==' fatal mistake
	{
		SDL_SetColorKey(border,SDL_SRCCOLORKEY,SDL_MapRGB(border->format,0,255,255));
	}
	else
	{
		tlog2 << "Warning: no border for "<<Str->ID<<std::endl;
	}
	if (area = BitmapHandler::loadBitmap(str->areaName))//FIXME look up
	{ 
		;//SDL_SetColorKey(area,SDL_SRCCOLORKEY,SDL_MapRGB(area->format,0,255,255));
	}
	else
	{
		tlog2 << "Warning: no area for "<<Str->ID<<std::endl;
	}
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
	Hoverable::activate();
	ClickableL::activate();
	ClickableR::activate();

}
void CBuildingRect::deactivate()
{
	Hoverable::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
	if(moi)
		MotionInterested::deactivate();
	moi=false;
}
bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	if(str->pos.z != p2.str->pos.z)
		return (str->pos.z) < (p2.str->pos.z);
	else
		return (str->ID) < (p2.str->ID);
}
void CBuildingRect::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		if(!moi)
			MotionInterested::activate();
		moi = true;
	}
	else
	{
		if(moi)
			MotionInterested::deactivate();
		moi = false;
		if(LOCPLINT->castleInt->hBuild == this)
		{
			LOCPLINT->castleInt->hBuild = NULL;
			LOCPLINT->statusbar->clear();

			//call mouseMoved in other buildings, cursor might have been moved while they were inactive (eg. because of r-click popup)
			for(size_t i = 0; i < LOCPLINT->castleInt->buildings.size(); i++)
				LOCPLINT->castleInt->buildings[i]->mouseMoved(LOCPLINT->current->motion);
		}
	}
}
void CBuildingRect::clickLeft (tribool down)
{

	if(area && (LOCPLINT->castleInt->hBuild==this) && !(indeterminate(down)) && (CSDL_Ext::SDL_GetPixel(area,LOCPLINT->current->motion.x-pos.x,LOCPLINT->current->motion.y-pos.y) != 0)) //na polu
	{
		if(pressedL && !down)
			LOCPLINT->castleInt->buildingClicked(str->ID);
		ClickableL::clickLeft(down);
	}


	//todo - handle
}
void CBuildingRect::clickRight (tribool down)
{
	if((!area) || (!((bool)down)) || (this!=LOCPLINT->castleInt->hBuild))
		return;
	if((CSDL_Ext::SDL_GetPixel(area,LOCPLINT->current->motion.x-pos.x,LOCPLINT->current->motion.y-pos.y) != 0)) //na polu
	{
		CBuilding *bld = CGI->buildh->buildings[str->townID][str->ID];
		assert(bld);

		CInfoPopup *vinya = new CInfoPopup();
		vinya->free = true;
		vinya->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,
			bld->Description(),
			LOCPLINT->castleInt->bicons->ourImages[str->ID].bitmap,
			bld->Name());
		vinya->pos.x = screen->w/2 - vinya->bitmap->w/2;
		vinya->pos.y = screen->h/2 - vinya->bitmap->h/2;
		LOCPLINT->pushInt(vinya);
	}
}
void CBuildingRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if(area && isItIn(&pos,sEvent.x, sEvent.y))
	{
		if(CSDL_Ext::SDL_GetPixel(area,sEvent.x-pos.x,sEvent.y-pos.y) == 0) //hovered pixel is inside this building
		{
			if(LOCPLINT->castleInt->hBuild == this)
			{
				LOCPLINT->castleInt->hBuild = NULL;
				LOCPLINT->statusbar->clear();
			}
		}
		else //inside the area of this building
		{
			if(LOCPLINT->castleInt->hBuild) //a building is hovered
			{
				if((*LOCPLINT->castleInt->hBuild)<(*this)) //set if we are on top
				{
					LOCPLINT->castleInt->hBuild = this;
					if(CGI->buildh->buildings[str->townID][str->ID] && CGI->buildh->buildings[str->townID][str->ID]->Name().length())
						LOCPLINT->statusbar->print(CGI->buildh->buildings[str->townID][str->ID]->Name());
					else
						LOCPLINT->statusbar->print(str->name);
				}
			}
			else //no building hovered
			{
				LOCPLINT->castleInt->hBuild = this;
				if(CGI->buildh->buildings[str->townID][str->ID] && CGI->buildh->buildings[str->townID][str->ID]->Name().length())
					LOCPLINT->statusbar->print(CGI->buildh->buildings[str->townID][str->ID]->Name());
				else
					LOCPLINT->statusbar->print(str->name);
			}
		}
	}
	//if(border)
	//	blitAt(border,pos.x,pos.y);
}
void CHeroGSlot::hover (bool on)
{
	if(!on) return;
	CHeroGSlot *other = upg  ?  &owner->hslotup :  &owner->hslotdown;
	std::string temp;
	if(hero)
	{
		if(highlight)//view NNN
		{
			temp = CGI->generaltexth->tcommands[4];
			boost::algorithm::replace_first(temp,"%s",hero->name);
		}
		else if(other->hero && other->highlight)//exchange
		{
			temp = CGI->generaltexth->tcommands[7];
			boost::algorithm::replace_first(temp,"%s",hero->name);
			boost::algorithm::replace_first(temp,"%s",other->hero->name);
		}
		else// select NNN (in ZZZ)
		{
			if(upg)//down - visiting
			{
				temp = CGI->generaltexth->tcommands[32];
				boost::algorithm::replace_first(temp,"%s",hero->name);
			}
			else //up - garrison
			{
				temp = CGI->generaltexth->tcommands[12];
				boost::algorithm::replace_first(temp,"%s",hero->name);
			}
		}
	}
	else //we are empty slot
	{
		if(other->highlight && other->hero) //move NNNN
		{
			temp = CGI->generaltexth->tcommands[6];
			boost::algorithm::replace_first(temp,"%s",other->hero->name);
		}
		else //empty
		{
			temp = CGI->generaltexth->allTexts[507];
		}
	}
	if(temp.size())
		LOCPLINT->statusbar->print(temp);
}

void CHeroGSlot::clickRight (boost::logic::tribool down)
{

}

void CHeroGSlot::clickLeft(boost::logic::tribool down)
{
	CHeroGSlot *other = upg  ?  &owner->hslotup :  &owner->hslotdown;
	if(!down)
	{
		owner->garr->splitting = false;
		owner->garr->highlighted = NULL;
		if(hero && highlight)
		{
			highlight = false;
			LOCPLINT->openHeroWindow(hero);
		}
		else if(other->hero && other->highlight)
		{
			bool allow = true;
			if(upg) //moving hero out of town - check if it is allowed
			{
				if(!hero && LOCPLINT->cb->howManyHeroes(false) >= 8)
				{
					std::string tmp = CGI->generaltexth->allTexts[18]; //You already have %d adventuring heroes under your command.
					boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes(false)));
					LOCPLINT->showInfoDialog(tmp,std::vector<SComponent*>(), soundBase::sound_todo);
					allow = false;
				}
				else if(!other->hero->army.slots.size()) //hero has no creatures - strange, but if we have appropriate error message...
				{
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[19],std::vector<SComponent*>(), soundBase::sound_todo); //This hero has no creatures.  A hero must have creatures before he can brave the dangers of the countryside.
					allow = false;
				}
			}
			other->highlight = highlight = false;
			if(allow)
				LOCPLINT->cb->swapGarrisonHero(owner->town);
		}
		else if(hero)
		{
			highlight = true;
			owner->garr->highlighted = NULL;
			show(screen2);
		}
		hover(false);hover(true); //refresh statusbar
	}
	if(indeterminate(down) && !isItIn(&other->pos,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		other->highlight = highlight = false;
		show(screen2);
	}
}

void CHeroGSlot::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}

void CHeroGSlot::deactivate()
{
	highlight = false;
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}

void CHeroGSlot::show(SDL_Surface * to)
{
	if(hero) //there is hero
		blitAt(graphics->portraitLarge[hero->portrait],pos,to);
	else if(!upg) //up garrison
		blitAt(graphics->flags->ourImages[LOCPLINT->castleInt->town->getOwner()].bitmap,pos,to);
	if(highlight)
		blitAt(graphics->bigImgs[-1],pos,to);
}

CHeroGSlot::CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, CCastleInterface * Owner)
{
	owner = Owner;
	pos.x = x;
	pos.y = y;
	pos.w = 58;
	pos.h = 64;
	hero = h;
	upg = updown;
	highlight = false;
}

CHeroGSlot::~CHeroGSlot()
{
}

static std::string getBgName(int type) //TODO - co z tym zrobi�?
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
#ifndef __GNUC__
		throw new std::exception("std::string getBgName(int type): invalid type");
#else
		throw new std::exception();
#endif
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

CCastleInterface::CCastleInterface(const CGTownInstance * Town)
:hslotup(241,387,0,Town->garrisonHero,this),hslotdown(241,483,1,Town->visitingHero,this)
{
	bars = CDefHandler::giveDefEss("TPTHBAR.DEF");
	status = CDefHandler::giveDefEss("TPTHCHK.DEF");
	LOCPLINT->castleInt = this;
	hall = NULL;
	townInt = BitmapHandler::loadBitmap("TOWNSCRN.bmp");
	cityBg = BitmapHandler::loadBitmap(getBgName(Town->subID));
	pos.x = screen->w/2 - 400;
	pos.y = screen->h/2 - 300;
	hslotup.pos.x += pos.x;
	hslotup.pos.y += pos.y;
	hslotdown.pos.x += pos.x;
	hslotdown.pos.y += pos.y;
	hall = CDefHandler::giveDef("ITMTL.DEF");
	fort = CDefHandler::giveDef("ITMCL.DEF");
	hBuild = NULL;
	count=0;
	town = Town;
	animval = 0;

	//garrison
	garr = new CGarrisonInt(pos.x+305,pos.y+387,4,32,townInt,243,13,town,town->visitingHero);

	townlist = new CTownList(3,pos.x+744,pos.y+414,"IAM014.DEF","IAM015.DEF");//744,526);
	exit = new AdventureMapButton
		(CGI->generaltexth->tcommands[8],"",boost::bind(&CCastleInterface::close,this),pos.x+744,pos.y+544,"TSBTNS.DEF",SDLK_RETURN);
	split = new AdventureMapButton
		(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+744,pos.y+382,"TSBTNS.DEF");
	statusbar = new CStatusBar(pos.x+7,pos.y+555,"TSTATBAR.bmp",732);
	resdatabar = new CResDataBar("ZRESBAR.bmp",pos.x+3,pos.y+575,32,2,85,85);
	resdatabar->pos.x = pos.x+3; resdatabar->pos.y = pos.y+575;

	townlist->fun = boost::bind(&CCastleInterface::townChange,this);
	townlist->genList();
	townlist->selected = vstd::findPos(townlist->items,Town);
	if((townlist->selected+1) > townlist->SIZE)
		townlist->from = townlist->selected -  townlist->SIZE + 2;

	graphics->blueToPlayersAdv(townInt,LOCPLINT->playerID);
	exit->bitmapOffset = 4;


	//buildings
	recreateBuildings();


	std::string defname;
	switch (town->subID)
	{
	case 0:
		defname = "HALLCSTL.DEF";
		musicID = musicBase::castleTown;
		break;
	case 1:
		defname = "HALLRAMP.DEF";
		musicID = musicBase::rampartTown;
		break;
	case 2:
		defname = "HALLTOWR.DEF";
		musicID = musicBase::towerTown;
		break;
	case 3:
		defname = "HALLINFR.DEF";
		musicID = musicBase::infernoTown;
		break;
	case 4:
		defname = "HALLNECR.DEF";
		musicID = musicBase::necroTown;
		break;
	case 5:
		defname = "HALLDUNG.DEF";
		musicID = musicBase::dungeonTown;
		break;
	case 6:
		defname = "HALLSTRN.DEF";
		musicID = musicBase::strongHoldTown;
		break;
	case 7:
		defname = "HALLFORT.DEF";
		musicID = musicBase::fortressTown;
		break;
	case 8:
		defname = "HALLELEM.DEF";
		musicID = musicBase::elemTown;
		break;
	default:
		throw new std::string("Wrong town subID");
	}
	bicons = CDefHandler::giveDefEss(defname);
}

CCastleInterface::~CCastleInterface()
{
	delete bars;
	delete status;
	SDL_FreeSurface(townInt);
	SDL_FreeSurface(cityBg);
	delete exit;
	delete split;
	delete hall;
	delete fort;
	delete garr;
	delete townlist;
	delete statusbar;
	delete resdatabar;
	for(size_t i=0;i<buildings.size();i++)
	{
		delete buildings[i];
	}
	delete bicons;

}

void CCastleInterface::close()
{
	if(town->visitingHero)
		LOCPLINT->adventureInt->select(town->visitingHero);
	LOCPLINT->castleInt = NULL;
	LOCPLINT->popIntTotally(this);
	CGI->musich->stopMusic(5000);
}

void CCastleInterface::splitF()
{
}

void CCastleInterface::buildingClicked(int building)
{
	tlog5<<"You've clicked on "<<building<<std::endl;
	if(building==19 || building==18)
	{
		building = town->town->hordeLvl[0] + 30;
	}
	else if(building==24 || building==25)
	{
		building = town->town->hordeLvl[1] + 30;
	}
	if(building >= 30)
	{
		showRecruitmentWindow(building);
	}
	else
	{
		switch(building)
		{
		case 0: case 1: case 2: case 3: case 4: //mage guild
			{
				if(town->visitingHero && !vstd::contains(town->visitingHero->artifWorn,ui16(17))) //visiting hero doesn't have spellboks
				{
					if(LOCPLINT->cb->getResourceAmount(6) < 500) //not enough gold to buy spellbook
					{
						LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[213],std::vector<SComponent*>(), soundBase::sound_todo);
					}
					else
					{
						CFunctionList<void()> fl = boost::bind(&CCallback::buyArtifact,LOCPLINT->cb,town->visitingHero,0);
						fl += boost::bind(&CCastleInterface::enterMageGuild,this);
						std::vector<SComponent*> vvv(1,new SComponent(SComponent::artifact,0,0));
						LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[214],vvv,fl,0,true);
					}
				}
				else
				{ 
					enterMageGuild();
				}
				break;
			}
		case 5: //tavern
			{
				enterTavern();

				break;
			}
		//TODO: case 6: //shipyard
		case 7: case 8: case 9: //fort/citadel/castle
			{
				CFortScreen *fs = new CFortScreen(this);
				LOCPLINT->pushInt(fs);
				break;
			}
		case 10: case 11: case 12: case 13: //hall
			enterHall();
			break;
		case 14:  //marketplace
			{
				CMarketplaceWindow *cmw = new CMarketplaceWindow(0);
				LOCPLINT->pushInt(cmw);
				break;
			}
		case 15: //resource silo
			{
				LOCPLINT->showInfoDialog(CGI->buildh->buildings[town->subID][15]->Description(),std::vector<SComponent*>(), soundBase::sound_todo);
				break;
			}
		case 16: //blacksmith
			{
				const CGHeroInstance *hero = town->visitingHero;
				if(!hero)
				{
					std::string pom = CGI->generaltexth->allTexts[273];
					boost::algorithm::replace_first(pom,"%s",CGI->buildh->buildings[town->subID][16]->Name());
					LOCPLINT->showInfoDialog(pom,std::vector<SComponent*>(), soundBase::sound_todo);
					return;
				}
				int aid = town->town->warMachine;
				int price = CGI->arth->artifacts[aid].price;
				bool possible = (LOCPLINT->cb->getResourceAmount(6) >= price);
				if(vstd::contains(hero->artifWorn,ui16(aid+9))) //hero already has machine
					possible = false;

				LOCPLINT->pushInt(new CBlacksmithDialog(possible,CArtHandler::convertMachineID(aid,false),aid,hero->id));
				break;
			}
		//TODO: case 17: //special 1
		//TODO: case 18: //basic horde 1
		//TODO: case 19: //upg horde 1
		//TODO: case 20: //ship at shipyard
		//TODO: case 21: //special 2
		case 22: //special 3
			{
				switch(town->subID)
				{
				case 0: //brotherhood of sword
					enterTavern();
					break;

				default:
					tlog4<<"This building isn't handled...\n";
					break;
				}
				break;
			}
		//TODO: case 23: //special 4
		//TODO: case 24: //basic horde 2
		//TODO: case 25: //upg horde 2
		//TODO: case 26: //grail
		default:
			tlog4<<"This building isn't handled...\n";
		}
	}
}

void CCastleInterface::enterHall()
{
	CHallInterface *h = new CHallInterface(this);
	LOCPLINT->pushInt(h);
}

void CCastleInterface::showAll( SDL_Surface * to/*=NULL*/)
{
	blitAt(cityBg,pos,to);
	blitAt(townInt,pos.x,pos.y+374,to);
	LOCPLINT->adventureInt->resdatabar.draw(to);
	townlist->draw(to);
	statusbar->show(to);
	resdatabar->draw(to);

	garr->show(to);
	int pom;

	//draw fort icon
	if(town->builtBuildings.find(9)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(8)!=town->builtBuildings.end())
		pom = 1;
	else if(town->builtBuildings.find(7)!=town->builtBuildings.end())
		pom = 0;
	else pom = 3;
	blitAt(fort->ourImages[pom].bitmap,pos.x+122,pos.y+413,to);

	//draw ((village/town/city) hall)/capitol icon
	if(town->builtBuildings.find(13)!=town->builtBuildings.end())
		pom = 3;
	else if(town->builtBuildings.find(12)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(11)!=town->builtBuildings.end())
		pom = 1;
	else pom = 0;
	blitAt(hall->ourImages[pom].bitmap,pos.x+80,pos.y+413,to);

	//draw creatures icons and their growths
	for(int i=0;i<CREATURES_PER_TOWN;i++)
	{
		int cid = -1;
		if (town->builtBuildings.find(30+i)!=town->builtBuildings.end())
		{
			if (town->builtBuildings.find(30+CREATURES_PER_TOWN+i)!=town->builtBuildings.end())
				cid = town->town->upgradedCreatures[i];
			else
				cid = town->town->basicCreatures[i];
		}
		if (cid>=0)
		{
			int pomx, pomy;
			pomx = 22 + (55*((i>3)?(i-4):i));
			pomy = (i>3)?(507):(459);
			blitAt(graphics->smallImgs[cid],pos.x+pomx,pos.y+pomy,to);
			std::ostringstream oss;
			oss << '+' << town->creatureGrowth(i);
			CSDL_Ext::printAtMiddle(oss.str(),pos.x+pomx+16,pos.y+pomy+37,GEOR13,zwykly,to);
		}
	}

	//print name and income
	CSDL_Ext::printAt(town->name,pos.x+85,pos.y+389,GEOR13,zwykly,to);
	char temp[10];
	SDL_itoa(town->dailyIncome(),temp,10);
	CSDL_Ext::printAtMiddle(temp,pos.x+195,pos.y+442,GEOR13,zwykly,to);

	//blit town icon
	pom = town->subID*2;
	if (!town->hasFort())
		pom += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(graphics->bigTownPic->ourImages[pom].bitmap,pos.x+15,pos.y+387,to);

	hslotup.show(to);
	hslotdown.show(to);

	show(to);

	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
	exit->show(to);
	split->show(to);
}

void CCastleInterface::townChange()
{
	const CGTownInstance * nt = townlist->items[townlist->selected];
	LOCPLINT->popIntTotally(this);
	LOCPLINT->pushInt(new CCastleInterface(nt));
}

void CCastleInterface::show(SDL_Surface * to)
{
	count++;
	if(count==8)
	{
		count=0;
		animval++;
	}

	blitAt(cityBg,pos,to);


	//blit buildings
	for(size_t i=0;i<buildings.size();i++)
	{
		int frame = ((animval)%(buildings[i]->max - buildings[i]->offset)) + buildings[i]->offset;
		if(frame)
		{
			blitAt(buildings[i]->def->ourImages[0].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
			blitAt(buildings[i]->def->ourImages[frame].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
		}
		else
			blitAt(buildings[i]->def->ourImages[frame].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);

		if(hBuild==buildings[i] && hBuild->border) //if this this higlighted structure and has border we'll blit it
			blitAt(hBuild->border,hBuild->pos,to);
	}
}

void CCastleInterface::activate()
{
	showing = true;
	townlist->activate();
	garr->activate();
	LOCPLINT->statusbar = statusbar;
	exit->activate();
	split->activate();
	for(size_t i=0;i<buildings.size();i++) //XXX pls use iterators or at() but not []
	{
		buildings[i]->activate();
	}
	hslotdown.activate();
	hslotup.activate();
}

void CCastleInterface::deactivate()
{
	showing = false;
	townlist->deactivate();
	garr->deactivate();
	exit->deactivate();
	split->deactivate();
	for(size_t i=0;i<buildings.size();i++) //XXX iterators
	{
		buildings[i]->deactivate();
	}
	hslotdown.deactivate();
	hslotup.deactivate();
}

void CCastleInterface::addBuilding(int bid)
{
	//TODO: lepiej by bylo tylko dodawac co trzeba pamietajac o grupach
	deactivate();
	recreateBuildings();
	activate();
}

void CCastleInterface::removeBuilding(int bid)
{
	//TODO: lepiej by bylo tylko usuwac co trzeba pamietajac o grupach
	recreateBuildings();
}

void CCastleInterface::recreateBuildings()
{
	for(size_t i=0;i<buildings.size();i++)
	{
		if(showing)
			buildings[i]->deactivate();
		delete buildings[i];
	}
	buildings.clear();
	hBuild = NULL;

	std::set< std::pair<int,int> > s; //group - id


	for (std::set<si32>::const_iterator i=town->builtBuildings.begin();i!=town->builtBuildings.end();i++)
	{
		if(CGI->townh->structures.find(town->subID) != CGI->townh->structures.end()) //we have info about structures in this town
		{
			if(CGI->townh->structures[town->subID].find(*i)!=CGI->townh->structures[town->subID].end()) //we have info about that structure
			{
				Structure * st = CGI->townh->structures[town->subID][*i];
				if(st->group<0) //no group - just add it
				{
					buildings.push_back(new CBuildingRect(st));
				}
				else
				{
					std::set< std::pair<int,int> >::iterator obecny=s.end();
					for(std::set< std::pair<int,int> >::iterator seti = s.begin(); seti!=s.end(); seti++) //check if we have already building from same group
					{
						if(seti->first == st->group)
						{
							obecny = seti;
							break;
						}
					}
					if(obecny != s.end())
					{
						if((*(CGI->townh->structures[town->subID][obecny->second])) < (*(CGI->townh->structures[town->subID][st->ID]))) //we have to replace old building with current one
						{
							for(size_t itpb = 0; itpb<buildings.size(); itpb++)
							{
								if(buildings[itpb]->str->ID == obecny->second)
								{
									delete buildings[itpb];
									buildings.erase(buildings.begin() + itpb);
									#ifndef __GNUC__
									obecny->second = st->ID; //XXX look closer!
									#else
									*(const_cast<int*>(&(obecny->second))) = st->ID;
									#endif
									buildings.push_back(new CBuildingRect(st));
								}
							}
						}
					}
					else
					{
						buildings.push_back(new CBuildingRect(st));
						s.insert(std::pair<int,int>(st->group,st->ID));
					}
				}
			}
			else continue;
		}
		else
			break;
	}
	std::sort(buildings.begin(),buildings.end(),srthlp);

	//code for Mana Vortex (there are two sets of animation frames - one without mage guild and one with
	if((town->subID == 5) && (town->builtBuildings.find(21)!=town->builtBuildings.end()))
	{
		CBuildingRect *vortex = NULL;
		for(size_t i=0;i<buildings.size();i++)
		{
			if(buildings[i]->str->ID==21)
			{
				vortex=buildings[i];
				break;
			}
		}
		if(town->builtBuildings.find(4)!=town->builtBuildings.end()) //there is mage Guild level 5
		{
			vortex->offset = 10;
			vortex->max = vortex->def->ourImages.size();
		}
		else
		{
			vortex->offset = 0;
			vortex->max = 10;
		}
	}
	//code for the shipyard in the Castle
	else if((town->subID == 0) && (town->builtBuildings.find(6)!=town->builtBuildings.end()))
	{
		CBuildingRect *shipyard = NULL;
		for(size_t i=0;i<buildings.size();i++)
		{
			if(buildings[i]->str->ID==6)
			{
				shipyard=buildings[i];
				break;
			}
		}
		if(town->builtBuildings.find(8)!=town->builtBuildings.end()) //there is citadel
		{
			shipyard->offset = 1;
			shipyard->max = shipyard->def->ourImages.size();
		}
		else
		{
			shipyard->offset = 0;
			shipyard->max = 1;
		}
	}
}

CRecruitmentWindow * CCastleInterface::showRecruitmentWindow( int building )
{
	if(building>36)
		building-=7;
	std::vector<std::pair<int,int > > crs;
	int amount = (const_cast<CGTownInstance*>(town))->strInfo.creatures[building-30]; //trzeba odconstowac, bo inaczej operator [] by sypal :(

	if(town->builtBuildings.find(building+7) != town->builtBuildings.end()) //check if there is an upgraded building
		crs.push_back(std::make_pair(town->town->upgradedCreatures[building-30],amount));

	crs.push_back(std::make_pair(town->town->basicCreatures[building-30],amount));

	CRecruitmentWindow *rw = new CRecruitmentWindow(crs,boost::bind(&CCallback::recruitCreatures,LOCPLINT->cb,town,_1,_2));
	LOCPLINT->pushInt(rw);
	return rw;
}

void CCastleInterface::enterMageGuild()
{
	LOCPLINT->pushInt(new CMageGuildScreen(this));
}

void CCastleInterface::enterTavern()
{
	std::vector<const CGHeroInstance*> h = LOCPLINT->cb->getAvailableHeroes(town);
	CTavernWindow *tv = new CTavernWindow(h[0],h[1],"GOSSIP TEST");
	LOCPLINT->pushInt(tv);
}

void CHallInterface::CBuildingBox::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		std::string toPrint;
		if(state==8)
			toPrint = CGI->generaltexth->hcommands[5];
		else
			toPrint = CGI->generaltexth->hcommands[state];
		std::vector<std::string> name;
		name.push_back(CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][BID]->Name());
		LOCPLINT->statusbar->print(CSDL_Ext::processStr(toPrint,name));
	}
	else
		LOCPLINT->statusbar->clear();
}
void CHallInterface::CBuildingBox::clickLeft (tribool down)
{
	if(pressedL && (!down))
	{
		LOCPLINT->pushInt(new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,0));
	}
	ClickableL::clickLeft(down);
}
void CHallInterface::CBuildingBox::clickRight (tribool down)
{
	if(down)
	{
		LOCPLINT->pushInt(new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,1));
	}
	ClickableR::clickRight(down);
}
void CHallInterface::CBuildingBox::show(SDL_Surface * to)
{
	CCastleInterface *ci = LOCPLINT->castleInt;
	blitAt(ci->bicons->ourImages[BID].bitmap,pos.x,pos.y,to);
	int pom, pom2=-1;
	switch (state)
	{
	case 4:
		pom = 0;
		pom2 = 0;
		break;
	case 7:
		pom = 1;
		break;
	case 6:
		pom2 = 2;
		pom = 2;
		break;
	case 0: case 5: case 8:
		pom2 = 1;
		pom = 2;
		break;
	case 2: case 1: default:
		pom = 3;
		break;
	}
	blitAt(ci->bars->ourImages[pom].bitmap,pos.x-1,pos.y+71,to);
	if(pom2>=0)
		blitAt(ci->status->ourImages[pom2].bitmap,pos.x+135, pos.y+54,to);
	CSDL_Ext::printAtMiddle(CGI->buildh->buildings[ci->town->subID][BID]->Name(),pos.x-1+ci->bars->ourImages[0].bitmap->w/2,pos.y+71+ci->bars->ourImages[0].bitmap->h/2, GEOR13,zwykly,to);
}
void CHallInterface::CBuildingBox::activate()
{
	Hoverable::activate();
	ClickableL::activate();
	ClickableR::activate();
}
void CHallInterface::CBuildingBox::deactivate()
{
	Hoverable::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
}
CHallInterface::CBuildingBox::~CBuildingBox()
{
}
CHallInterface::CBuildingBox::CBuildingBox(int id)
	:BID(id)
{
	pos.w = 150;
	pos.h = 70;
}
CHallInterface::CBuildingBox::CBuildingBox(int id, int x, int y)
	:BID(id)
{
	pos.x = x;
	pos.y = y;
	pos.w = 150;
	pos.h = 70;
}


CHallInterface::CHallInterface(CCastleInterface * owner)
{
	resdatabar = new CMinorResDataBar;
	pos = owner->pos;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	bg = BitmapHandler::loadBitmap(CGI->buildh->hall[owner->town->subID].first);
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	exit = new AdventureMapButton
		(CGI->generaltexth->tcommands[8],"",boost::bind(&CHallInterface::close,this),pos.x+748,pos.y+556,"TPMAGE1.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	//preparing boxes with buildings//
	boxes.resize(5);
	for(size_t i=0;i<5;i++) //for each row
	{
		std::vector< std::vector< std::vector<int> > > &boxList = CGI->buildh->hall[owner->town->subID].second;

		for(size_t j=0; j<boxList[i].size();j++) //for each box
		{
			size_t k=0;
			for(;k<boxList[i][j].size();k++)//we are looking for the first not build structure
			{
				int bid = boxList[i][j][k];

				if(!vstd::contains(owner->town->builtBuildings,bid))
				{
					int x = 34 + 194*j,
						y = 37 + 104*i,
						ID = bid;
					if(boxList[i].size() == 2) //only two boxes in this row
						x+=194;
					else if(boxList[i].size() == 3) //only three boxes in this row
						x+=97;
					boxes[i].push_back(new CBuildingBox(bid,pos.x+x,pos.y+y));

					//if this is horde dwelling for upgraded creatures and we already have one for basic creatures
					if((bid == 25  &&  vstd::contains(owner->town->builtBuildings,24))
						|| (bid == 19  &&  vstd::contains(owner->town->builtBuildings,18))
					)
					{
						boxes[i].back()->state = 4; //already built
					}
					else
					{
						boxes[i].back()->state = LOCPLINT->cb->canBuildStructure(owner->town,ID);
					}

					break;
				}
			}
			if(k == boxList[i][j].size()) //all buildings built - let's take the last one
			{
				int x = 34 + 194*j,
					y = 37 + 104*i;
				if(boxList[i].size() == 2)
					x+=194;
				else if(boxList[i].size() == 3)
					x+=97;
				boxes[i].push_back(new CBuildingBox(boxList[i][j][k-1],pos.x+x,pos.y+y));
				boxes[i][boxes[i].size()-1]->state = 4; //already exists
			}
		}
	}
}
CHallInterface::~CHallInterface()
{
	SDL_FreeSurface(bg);
	for(size_t i=0;i<boxes.size();i++)
		for(size_t j=0;j<boxes[i].size();j++)
			delete boxes[i][j]; //TODO whats wrong with smartpointers?
	delete exit;
	delete resdatabar;
}
void CHallInterface::close()
{
	LOCPLINT->popIntTotally(this);
}
void CHallInterface::show(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	LOCPLINT->castleInt->statusbar->show(to);
	resdatabar->show(to);
	exit->show(to);
	for(int i=0; i<5; i++)
	{
		for(size_t j=0;j<boxes[i].size(); ++j)
			boxes[i][j]->show(to);
	}
}
void CHallInterface::activate()
{
	for(int i=0;i<5;i++)
	{
		for(size_t j=0; j < boxes[i].size(); ++j)
		{
			boxes[i][j]->activate();
		}
	}
	exit->activate();
}
void CHallInterface::deactivate()
{
	for(int i=0;i<5;i++)
	{
		for(size_t j=0;j<boxes[i].size();++j)
		{
			boxes[i][j]->deactivate();
		}
	}
	exit->deactivate();
}

void CHallInterface::CBuildWindow::activate()
{
	ClickableR::activate();
	if(mode)
		return;
	if(state==7)
		buy->activate();
	cancel->activate();
}

void CHallInterface::CBuildWindow::deactivate()
{
	ClickableR::deactivate();
	if(mode)
		return;
	if(state==7)
		buy->deactivate();
	cancel->deactivate();
}

void CHallInterface::CBuildWindow::Buy()
{
	int building = bid;
	LOCPLINT->popInts(2); //we - build window and hall screen
	LOCPLINT->cb->buildBuilding(LOCPLINT->castleInt->town,building);
}

void CHallInterface::CBuildWindow::close()
{
	LOCPLINT->popIntTotally(this);
}

void CHallInterface::CBuildWindow::clickRight (tribool down)
{
	if((!down || indeterminate(down)) && mode)
		close();
}

void CHallInterface::CBuildWindow::show(SDL_Surface * to)
{
	SDL_Rect pom = genRect(bitmap->h-1,bitmap->w-1,pos.x,pos.y);
	SDL_Rect poms = pom; poms.x=0;poms.y=0;
	SDL_BlitSurface(bitmap,&poms,to,&pom);
	if(!mode)
	{
		buy->show(to);
		cancel->show(to);
	}
}

std::string CHallInterface::CBuildWindow::getTextForState(int state)
{
	std::string ret;
	if(state<7)
		ret =  CGI->generaltexth->hcommands[state];
	switch (state)
	{
	case 4:	case 5: case 6:
		ret.replace(ret.find_first_of("%s"),2,CGI->buildh->buildings[tid][bid]->Name());
		break;
	case 7:
		return CGI->generaltexth->allTexts[219]; //all prereq. are met
	case 8:
		{
			ret = CGI->generaltexth->allTexts[52];
			std::set<int> used;
			used.insert(bid);
			std::set<int> reqs;

			for(std::set<int>::iterator i=CGI->townh->requirements[tid][bid].begin();
				i!=CGI->townh->requirements[tid][bid].end();
				i++
			  )
			{
				if (LOCPLINT->castleInt->town->builtBuildings.find(*i)   ==  LOCPLINT->castleInt->town->builtBuildings.end())
					reqs.insert(*i);
			}
			while(true)
			{
				size_t czystych=0;
				for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
				{
					if(used.find(*i)==used.end()) //we haven't added requirements for this building
					{
						used.insert(*i);
						for(
							std::set<int>::iterator j=CGI->townh->requirements[tid][*i].begin();
							j!=CGI->townh->requirements[tid][*i].end();
							j++
								)
						{
							if(LOCPLINT->castleInt->town->builtBuildings.find(*j)   ==   //this building is not built
													LOCPLINT->castleInt->town->builtBuildings.end())
							reqs.insert(*j);
						}
					}
					else
					{
						czystych++;
					}
				}
				if(czystych==reqs.size())
					break;
			}
			bool first=true;
			for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
			{
				ret+=(((first)?(" "):(", ")) + CGI->buildh->buildings[tid][*i]->Name());
				first = false;
			}
		}
	}
	return ret;
}

CHallInterface::CBuildWindow::CBuildWindow(int Tid, int Bid, int State, bool Mode)
:tid(Tid), bid(Bid), state(State), mode(Mode)
{
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("TPUBUILD.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0); //na 8bitowej mapie by sie psulo
	SDL_SetColorKey(hhlp,SDL_SRCCOLORKEY,SDL_MapRGB(hhlp->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	blitAt(LOCPLINT->castleInt->bicons->ourImages[bid].bitmap,125,50,bitmap);
	std::vector<std::string> pom; pom.push_back(CGI->buildh->buildings[tid][bid]->Name());
	CSDL_Ext::printAtMiddleWB(CGI->buildh->buildings[tid][bid]->Description(),197,168,GEOR16,40,zwykly,bitmap);
	CSDL_Ext::printAtMiddleWB(getTextForState(state),197,248,GEOR13,50,zwykly,bitmap);
	CSDL_Ext::printAtMiddle(CSDL_Ext::processStr(CGI->generaltexth->hcommands[7],pom),197,30,GEOR16,tytulowy,bitmap);

	int resamount=0; 
	for(int i=0;i<7;i++)
	{
		if(CGI->buildh->buildings[tid][bid]->resources[i])
		{
			resamount++;
		}
	}
	int ah = (resamount>4) ? 304 : 341;
	int cn=-1, it=0;
	int row1w = std::min(resamount,4) * 32 + (std::min(resamount,4)-1) * 45,
		row2w = (resamount-4) * 32 + (resamount-5) * 45;
	char buf[15];
	while(++cn<7)
	{
		if(!CGI->buildh->buildings[tid][bid]->resources[cn])
			continue;
		SDL_itoa(CGI->buildh->buildings[tid][bid]->resources[cn],buf,10);
		if(it<4)
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row1w/2)+77*it+16,ah+42,GEOR16,zwykly,bitmap);
			blitAt(graphics->resources32->ourImages[cn].bitmap,(bitmap->w/2-row1w/2)+77*it++,ah,bitmap);
		}
		else
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row2w/2)+77*it+16-308,ah+42,GEOR16,zwykly,bitmap);
			blitAt(graphics->resources32->ourImages[cn].bitmap,(bitmap->w/2-row2w/2)+77*it++ - 308,ah,bitmap);
		}
		if(it==4)
			ah+=75;
	}
	if(!mode)
	{
		buy = new AdventureMapButton
			("","",boost::bind(&CBuildWindow::Buy,this),pos.x+45,pos.y+446,"IBUY30.DEF",SDLK_RETURN);
		cancel = new AdventureMapButton
			("","",boost::bind(&CBuildWindow::close,this),pos.x+290,pos.y+445,"ICANCEL.DEF",SDLK_ESCAPE);
		if(state!=7)
			buy->state=2;
	}
}

CHallInterface::CBuildWindow::~CBuildWindow()
{
	SDL_FreeSurface(bitmap);
	if(!mode)
	{
		delete buy;
		delete cancel;
	}
}

CFortScreen::~CFortScreen()
{
	for(size_t i=0;i<crePics.size();i++)
		delete crePics[i];
	for (size_t i=0;i<recAreas.size();i++)
		delete recAreas[i];
	SDL_FreeSurface(bg);
	delete exit;
	delete resdatabar;
}

void CFortScreen::show( SDL_Surface * to)
{
	blitAt(bg,pos,to);
	static unsigned char anim = 1;
	for (int i=0; i<CREATURES_PER_TOWN; i++)
	{
		crePics[i]->blitPic(to,pos.x+positions[i].x+159,pos.y+positions[i].y+4,!(anim%4));
	}
	anim++;
	exit->show(to);
	resdatabar->show(to);
	LOCPLINT->statusbar->show(to);
}

void CFortScreen::activate()
{
	LOCPLINT->statusbar = LOCPLINT->castleInt->statusbar;
	exit->activate();
	for (size_t i=0;i<recAreas.size(); ++i)
	{
		recAreas[i]->activate();
	}
}

void CFortScreen::deactivate()
{
	exit->deactivate();
	for (size_t i=0;i<recAreas.size();i++)
	{
		recAreas[i]->deactivate();
	}
}

void CFortScreen::close()
{
	LOCPLINT->popIntTotally(this);
}

CFortScreen::CFortScreen( CCastleInterface * owner )
{
	resdatabar = new CMinorResDataBar;
	pos = owner->pos;
	bg = NULL;
	exit = new AdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CFortScreen::close,this),pos.x+748,pos.y+556,"TPMAGE1.DEF",SDLK_RETURN);
	positions += genRect(126,386,10,22),genRect(126,386,404,22),
		genRect(126,386,10,155),genRect(126,386,404,155),
		genRect(126,386,10,288),genRect(126,386,404,288),
		genRect(126,386,206,421);
	draw(owner,true);
	resdatabar->pos += pos;
}

void CFortScreen::draw( CCastleInterface * owner, bool first)
{
	if(bg)
		SDL_FreeSurface(bg);
	char buf[20];
	memset(buf,0,20);
	SDL_Surface *bg2 = BitmapHandler::loadBitmap("TPCASTL7.bmp"),
		*icons =  BitmapHandler::loadBitmap("ZPCAINFO.bmp");
	SDL_SetColorKey(icons,SDL_SRCCOLORKEY,SDL_MapRGB(icons->format,0,255,255));
	graphics->blueToPlayersAdv(bg2,LOCPLINT->playerID);
	bg = SDL_ConvertSurface(bg2,screen->format,0); 
	SDL_FreeSurface(bg2);
	printAtMiddle(CGI->buildh->buildings[owner->town->subID][owner->town->fortLevel()+6]->Name(),400,13,GEORXX,zwykly,bg);
	for(int i=0;i<CREATURES_PER_TOWN; i++)
	{
		bool upgraded = owner->town->creatureDwelling(i,true);
		bool present = owner->town->creatureDwelling(i,false);
		CCreature *c = &CGI->creh->creatures[upgraded ? owner->town->town->upgradedCreatures[i] : owner->town->town->basicCreatures[i]];
		printAtMiddle(c->namePl,positions[i].x+79,positions[i].y+10,GEOR13,zwykly,bg); //cr. name
		blitAt(owner->bicons->ourImages[30+i+upgraded*7].bitmap,positions[i].x+4,positions[i].y+21,bg); //dwelling pic
		printAtMiddle(CGI->buildh->buildings[owner->town->subID][30+i+upgraded*7]->Name(),positions[i].x+79,positions[i].y+100,GEOR13,zwykly,bg); //dwelling name
		if(present) //if creature is present print avail able quantity
		{
			SDL_itoa(owner->town->strInfo.creatures.find(i)->second,buf,10);
			printAtMiddle(CGI->generaltexth->allTexts[217] + buf,positions[i].x+79,positions[i].y+118,GEOR13,zwykly,bg);
		}
		blitAt(icons,positions[i].x+261,positions[i].y+3,bg);

		//atttack
		printAt(CGI->generaltexth->allTexts[190],positions[i].x+288,positions[i].y+5,GEOR13,zwykly,bg);
		SDL_itoa(c->attack,buf,10);
		printToWR(buf,positions[i].x+381,positions[i].y+18,GEOR13,zwykly,bg);

		//defense
		printAt(CGI->generaltexth->allTexts[191],positions[i].x+288,positions[i].y+25,GEOR13,zwykly,bg);
		SDL_itoa(c->defence,buf,10);
		printToWR(buf,positions[i].x+381,positions[i].y+38,GEOR13,zwykly,bg);

		//damage
		printAt(CGI->generaltexth->allTexts[199],positions[i].x+288,positions[i].y+46,GEOR13,zwykly,bg);
		SDL_itoa(c->damageMin,buf,10);
		int hlp;
		if(c->damageMin > 0)
			hlp = log10f(c->damageMin)+2;
		else
			hlp = 2;
		buf[hlp-1]=' '; buf[hlp]='-'; buf[hlp+1]=' ';
		SDL_itoa(c->damageMax,buf+hlp+2,10);
		printToWR(buf,positions[i].x+381,positions[i].y+59,GEOR13,zwykly,bg);

		//health
		printAt(CGI->generaltexth->zelp[439].first,positions[i].x+288,positions[i].y+66,GEOR13,zwykly,bg);
		SDL_itoa(c->hitPoints,buf,10);
		printToWR(buf,positions[i].x+381,positions[i].y+79,GEOR13,zwykly,bg);

		//speed
		printAt(CGI->generaltexth->zelp[441].first,positions[i].x+288,positions[i].y+87,GEOR13,zwykly,bg);
		SDL_itoa(c->speed,buf,10);
		printToWR(buf,positions[i].x+381,positions[i].y+100,GEOR13,zwykly,bg);

		if(present)//growth
		{
			printAt(CGI->generaltexth->allTexts[194],positions[i].x+288,positions[i].y+107,GEOR13,zwykly,bg);
			SDL_itoa(owner->town->creatureGrowth(i),buf,10);
			printToWR(buf,positions[i].x+381,positions[i].y+120,GEOR13,zwykly,bg);
		}
		if(first)
		{
			crePics.push_back(new CCreaturePic(c,false));
			if(present)
			{
				recAreas.push_back(new RecArea(30+i+upgraded*7));
				recAreas.back()->pos = positions[i] + pos;
			}
		}
	}
	SDL_FreeSurface(icons);
}
void CFortScreen::RecArea::clickLeft (tribool down)
{
	if(!down && pressedL)
	{
		LOCPLINT->castleInt->showRecruitmentWindow(bid);
	}
	ClickableL::clickLeft(down);
}
void CFortScreen::RecArea::activate()
{
	ClickableL::activate();
	ClickableR::activate();
}
void CFortScreen::RecArea::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
}

void CFortScreen::RecArea::clickRight( tribool down )
{
	clickLeft(down);; //r-click does same as l-click - opens recr. window
}
CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner)
{
	resdatabar = new CMinorResDataBar;
	pos = owner->pos;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	bg = BitmapHandler::loadBitmap("TPMAGE.bmp");
	exit = new AdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CMageGuildScreen::close,this),pos.x+748,pos.y+556,"TPMAGE1.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	scrolls2 = CDefHandler::giveDefEss("TPMAGES.DEF");
	SDL_Surface *view = BitmapHandler::loadBitmap(graphics->guildBgs[owner->town->subID]);
	SDL_SetColorKey(view,SDL_SRCCOLORKEY,SDL_MapRGB(view->format,0,255,255));
	positions.resize(5);
	positions[0] += genRect(61,83,222,445), genRect(61,83,312,445), genRect(61,83,402,445), genRect(61,83,520,445), genRect(61,83,610,445), genRect(61,83,700,445);
	positions[1] += genRect(61,83,48,53), genRect(61,83,48,147), genRect(61,83,48,241), genRect(61,83,48,335), genRect(61,83,48,429);
	positions[2] += genRect(61,83,570,82), genRect(61,83,672,82), genRect(61,83,570,157), genRect(61,83,672,157);
	positions[3] += genRect(61,83,183,42), genRect(61,83,183,148), genRect(61,83,183,253);
	positions[4] += genRect(61,83,491,325), genRect(61,83,591,325);
	blitAt(view,332,76,bg);
	for(size_t i=0; i<owner->town->town->mageLevel; i++)
	{
		size_t sp = owner->town->spellsAtLevel(i+1,false); //spell at level with -1 hmmm?
		for(size_t j=0; j<sp; j++)
		{
			if(i<owner->town->mageGuildLevel() && owner->town->spells[i].size()>j)
			{
				spells.push_back(Scroll(&CGI->spellh->spells[owner->town->spells[i][j]]));
				spells[spells.size()-1].pos = positions[i][j];
				blitAt(graphics->spellscr->ourImages[owner->town->spells[i][j]].bitmap,positions[i][j],bg);
			}
			else
			{
				blitAt(scrolls2->ourImages[1].bitmap,positions[i][j],bg);
			}
		}
	}
	SDL_FreeSurface(view);
	for(size_t i=0;i<spells.size();i++)
	{
		spells[i].pos.x += pos.x;
		spells[i].pos.y += pos.y;
	}
	delete scrolls2;
}

CMageGuildScreen::~CMageGuildScreen()
{
	delete exit;
	SDL_FreeSurface(bg);
	delete resdatabar;
}

void CMageGuildScreen::close()
{
	LOCPLINT->popIntTotally(this);
}

void CMageGuildScreen::show(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	resdatabar->show(to);
	LOCPLINT->statusbar->show(to);
	exit->show(to);
}

void CMageGuildScreen::activate()
{
	exit->activate();
	for(size_t i=0;i<spells.size();i++)
	{
		spells[i].activate();
	}
}

void CMageGuildScreen::deactivate()
{
	exit->deactivate();
	for(size_t i=0;i<spells.size();i++)
	{
		spells[i].deactivate();
	}
}

void CMageGuildScreen::Scroll::clickLeft (tribool down)
{
	if(down)
	{
		std::vector<SComponent*> comps(1,
			new CCustomImgComponent(SComponent::spell,spell->id,0,graphics->spellscr->ourImages[spell->id].bitmap,false)
		);
		LOCPLINT->showInfoDialog(spell->descriptions[0],comps, soundBase::sound_todo);
	}
}

void CMageGuildScreen::Scroll::clickRight (tribool down)
{
	if(down)
	{
		CInfoPopup *vinya = new CInfoPopup();
		vinya->free = true;
		vinya->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,
			spell->descriptions[0],graphics->spellscr->ourImages[spell->id].bitmap,
			spell->name,30,30);
		vinya->pos.x = screen->w/2 - vinya->bitmap->w/2;
		vinya->pos.y = screen->h/2 - vinya->bitmap->h/2;
		LOCPLINT->pushInt(vinya);
	}
}

void CMageGuildScreen::Scroll::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
		LOCPLINT->statusbar->print(spell->name);
	else
		LOCPLINT->statusbar->clear();

}

void CMageGuildScreen::Scroll::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}

void CMageGuildScreen::Scroll::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}

CBlacksmithDialog::CBlacksmithDialog(bool possible, int creMachineID, int aid, int hid)
{
	SDL_Surface *bg2 = BitmapHandler::loadBitmap("TPSMITH.bmp");
	SDL_SetColorKey(bg2,SDL_SRCCOLORKEY,SDL_MapRGB(bg2->format,0,255,255));
	graphics->blueToPlayersAdv(bg2,LOCPLINT->playerID);
	bmp = SDL_ConvertSurface(bg2,screen->format,0); 
	SDL_FreeSurface(bg2);
	bg2 = BitmapHandler::loadBitmap("TPSMITBK.bmp");
	blitAt(bg2,64,50,bmp);
	SDL_FreeSurface(bg2);

	CCreatureAnimation cra(CGI->creh->creatures[creMachineID].animDefName);
	cra.nextFrameMiddle(bmp,170,120,true,0,false);
	char pom[75];
	sprintf(pom,CGI->generaltexth->allTexts[274].c_str(),CGI->creh->creatures[creMachineID].nameSing.c_str()); //build a new ...
	printAtMiddle(pom,165,28,GEORXX,tytulowy,bmp);
	printAtMiddle(CGI->generaltexth->jktexts[43],165,218,GEOR16,zwykly,bmp); //resource cost
	SDL_itoa(CGI->arth->artifacts[aid].price,pom,10);
	printAtMiddle(pom,165,290,GEOR13,zwykly,bmp);

	pos.w = bmp->w;
	pos.h = bmp->h;
	pos.x = screen->w/2 - pos.w/2;
	pos.y = screen->h/2 - pos.h/2;

	buy = new AdventureMapButton("","",boost::bind(&CBlacksmithDialog::close,this),pos.x + 42,pos.y + 312,"IBUY30.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton("","",boost::bind(&CBlacksmithDialog::close,this),pos.x + 224,pos.y + 312,"ICANCEL.DEF",SDLK_ESCAPE);

	if(possible)
		buy->callback += boost::bind(&CCallback::buyArtifact,LOCPLINT->cb,LOCPLINT->cb->getHeroInfo(hid,2),aid);
	else
		buy->bitmapOffset = 2;

	blitAt(graphics->resources32->ourImages[6].bitmap,148,244,bmp);
}

void CBlacksmithDialog::show( SDL_Surface * to )
{
	blitAt(bmp,pos,to);
	buy->show(to);
	cancel->show(to);
}

void CBlacksmithDialog::activate()
{
	if(!buy->bitmapOffset)
		buy->activate();
	cancel->activate();
}

void CBlacksmithDialog::deactivate()
{
	if(!buy->bitmapOffset)
		buy->deactivate();
	cancel->deactivate();
}

CBlacksmithDialog::~CBlacksmithDialog()
{
	SDL_FreeSurface(bmp);
	delete cancel;
	delete buy;
}

void CBlacksmithDialog::close()
{
	LOCPLINT->popIntTotally(this);
}
