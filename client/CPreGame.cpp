#include "../stdafx.h"
#include "CPreGame.h"
#include "../hch/CDefHandler.h"
#include <ctime>
#include <SDL.h>
#include <boost/filesystem.hpp>   // includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>
//#include "boost/foreach.hpp"
#include <zlib.h>
#include "../timeHandler.h"
#include <sstream>
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "../hch/CGeneralTextHandler.h"
#include "CCursorHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"
#include <cmath>
#include "Graphics.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include "../lib/Connection.h"
#include "../hch/CMusicHandler.h"
#include "../hch/CVideoHandler.h"
/*
 * CPreGame.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
extern SDL_Color tytulowy, tlo, zwykly ;
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
CPreGame * CPG;

namespace fs = boost::filesystem;
namespace s = CSDL_Ext;

static int getNextCastle(int current, PlayerInfo * ourInf, bool next=true) //next=flase => previous castle
{
	int dir = next ? 1 : -1;
	if (current==-2) //no castle - no change
		return current;
	else if (current==-1) //random => first/last available
	{
		int pom = (next) ? (0) : (F_NUMBER-1); // last or first
		for (;pom>=0 && pom<F_NUMBER;pom+=dir)
		{
			if (((int)pow((double)2,pom))&ourInf->allowedFactions)
			{
				current=pom;
				break;
			}
			else continue;
		}
	}
	else // next/previous available
	{
		for (;;)
		{
			current+=dir;
			if (((int)pow((double)2,(int)current))&ourInf->allowedFactions)
			{
				break;
			}
			if (current>=F_NUMBER || current<0)
			{
				double p1 = log((double)ourInf->allowedFactions)/log(2.0f)+0.000001f;
				double check = p1-((int)p1);
				if (check < 0.001)
					current=(int)p1;
				else
					current=-1;
				break;
			}
		}
	}
	return current;
}

HighButton::HighButton( SDL_Rect Pos, CDefHandler* Imgs, bool Sel, int id)
{
	type=0;
	imgs=Imgs;
	selectable=Sel;
	selected=false;
	state=0;
	pos=Pos;
	ID=id;
	highlightable=false;
	freeimgs = false;
};
HighButton::HighButton()
{
	freeimgs = true;
	state=0;
}
HighButton::~HighButton()
{
	if(freeimgs)
		delete imgs;
}
void HighButton::show()
{
	blitAt(imgs->ourImages[state].bitmap,pos.x,pos.y);
	updateRect(&pos);
}

void HighButton::hover(bool on)
{
	if (!highlightable) return;
	int i;
	if (on)
	{
		state=i=3;
		highlighted=true;
	}
	else
	{
		state=i=0;
		highlighted=false;
	}
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,screen,&pos);
	updateRect(&pos);
}

void HighButton::press(bool down)
{
	int i;
	if (down) state=i=1;
	else state=i=0;
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,screen,&pos);
	updateRect(&pos);
}
Button::Button( SDL_Rect Pos, boost::function<void()> Fun,CDefHandler* Imgs, bool Sel, CGroup* gr, int id)
	:HighButton(Pos,Imgs,Sel,id),ourGroup(gr),fun(Fun)
{
	type=1;
};	
Button::Button()
{
	ourGroup=NULL;type=1;
};

SetrButton::SetrButton()
{
	type=1;
	selectable=false;
	selected=false;
	state=0;
	highlightable=false;
}
void SetrButton::press(bool down)
{
	if (!down && state==1)
		*poin=key;
	HighButton::press(down);
}
void Button::hover(bool on)
{
	HighButton::hover(on);
}
void Button::select(bool on)
{
	int i;
	if (on) state=i=3;
	else state=i=0;
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,screen,&pos);
	updateRect(&pos);
	if (ourGroup && on && ourGroup->type==1)
	{
		if (ourGroup->selected && ourGroup->selected!=this)
			ourGroup->selected->select(false);
		ourGroup->selected =this;
	}
}

void Slider::updateSlid()
{
	float perc = -1.0f;
	if(positionsAmnt > capacity)
		perc = ((float)whereAreWe)/((float)positionsAmnt-capacity);
	else
		perc = 0;
	float myh;
	if (vertical)
	{
		myh=perc*((float)pos.h-48)+pos.y+16;
		SDL_FillRect(screen,&genRect(pos.h-32,pos.w,pos.x,pos.y+16),0);
		blitAt(slider->imgs->ourImages[0].bitmap,pos.x,(int)myh);
		slider->pos.y=(int)myh;
	}
	else
	{
		myh=perc*((float)pos.w-48)+pos.x+16;
		SDL_FillRect(screen,&genRect(pos.h,pos.w-32,pos.x+16,pos.y),0);
		blitAt(slider->imgs->ourImages[0].bitmap,(int)myh,pos.y);
		slider->pos.x=(int)myh;
	}
	updateRect(&pos);
}

void Slider::moveDown()
{
	if (whereAreWe<positionsAmnt-capacity)
		fun(++whereAreWe);
	updateSlid();
}
void Slider::moveUp()
{
	if (whereAreWe>0)
		fun(--whereAreWe);
	updateSlid();
}
Slider::Slider(int x, int y, int h, int amnt, int cap, bool ver)
{
	vertical=ver;
	positionsAmnt = amnt;
	capacity = cap;
	if (ver)
	{
		pos = genRect(h,16,x,y);
		down = new Button(genRect(16,16,x,y+h-16),boost::bind(&Slider::moveDown,this),CDefHandler::giveDef("SCNRBDN.DEF"),false);
		up = new Button(genRect(16,16,x,y),boost::bind(&Slider::moveUp,this),CDefHandler::giveDef("SCNRBUP.DEF"),false);
		slider = new Button(genRect(16,16,x,y+16),boost::function<void()>(),CDefHandler::giveDef("SCNRBSL.DEF"),false);
	}
	else
	{
		pos = genRect(16,h,x,y);
		down = new Button(genRect(16,16,x+h-16,y),boost::bind(&Slider::moveDown,this),CDefHandler::giveDef("SCNRBRT.DEF"),false);
		up = new Button(genRect(16,16,x,y),boost::bind(&Slider::moveUp,this),CDefHandler::giveDef("SCNRBLF.DEF"),false);
		slider = new Button(genRect(16,16,x+16,y),boost::function<void()>(),CDefHandler::giveDef("SCNRBSL.DEF"),false);
	}
	moving = false;
	whereAreWe=0;
}
void Slider::deactivate()
{
	CPG->interested.erase(std::find(CPG->interested.begin(),CPG->interested.end(),this));
}

void Slider::activate()
{
	SDL_FillRect(screen,&pos,0);
	up->show();
	down->show();
	slider->show();
	//SDL_Flip(screen);
	CSDL_Ext::update(screen);
	CPG->interested.push_back(this);
}

void Slider::handleIt(SDL_Event sEvent)
{
	if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		if (isItIn(&down->pos,sEvent.motion.x,sEvent.motion.y))
		{
			down->press();
		}
		else if (isItIn(&up->pos,sEvent.motion.x,sEvent.motion.y))
		{
			up->press();
		}
		else if (isItIn(&slider->pos,sEvent.motion.x,sEvent.motion.y))
		{
			//slider->press();
			moving=true;
		}
		else if (isItIn(&pos,sEvent.motion.x,sEvent.motion.y))
		{
			float dy;
			float pe;
			if (vertical)
			{
				dy = sEvent.motion.y-pos.y-16;
				pe = dy/((float)(pos.h-32));
				if (pe>1) pe=1;
				if (pe<0) pe=0;
			}
			else
			{
				dy = sEvent.motion.x-pos.x-16;
				pe = dy/((float)(pos.w-32));
				if (pe>1) pe=1;
				if (pe<0) pe=0;
			}
			whereAreWe = pe*(positionsAmnt-capacity);
			if (whereAreWe<0)whereAreWe=0;
			updateSlid();
			fun(whereAreWe);
		}
	}
	else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{

		if ((down->state==1) && isItIn(&down->pos,sEvent.motion.x,sEvent.motion.y))
		{
			this->down->fun();
		}
		if ((up->state==1) && isItIn(&up->pos,sEvent.motion.x,sEvent.motion.y))
		{
			this->up->fun();
		}
		if (down->state==1) down->press(false);
		if (up->state==1) up->press(false);
		if (moving)
		{
			//slider->press();
			moving=false;
		}
	}
	else if (sEvent.type==SDL_KEYDOWN)
	{
		switch (sEvent.key.keysym.sym)
		{
		case (SDLK_UP):
			CPG->ourScenSel->mapsel.moveByX(-1);
			break;
		case (SDLK_DOWN):
			CPG->ourScenSel->mapsel.moveByX(+1);
			break;
		case SDLK_PAGEUP:
			CPG->ourScenSel->mapsel.moveByX(-CPG->ourScenSel->mapsel.slid->capacity);
			break;
		case SDLK_PAGEDOWN:
			CPG->ourScenSel->mapsel.moveByX(+CPG->ourScenSel->mapsel.slid->capacity);
			break;
		case SDLK_HOME:
			CPG->ourScenSel->mapsel.moveByX(-CPG->ourScenSel->mapsel.slid->positionsAmnt);
			break;
		case SDLK_END:
			CPG->ourScenSel->mapsel.moveByX(+CPG->ourScenSel->mapsel.slid->positionsAmnt);
			break;
		default:
			//TODO do something nasty here like logs entry..
			break;
		}
	}
	else if (moving && sEvent.type==SDL_MOUSEMOTION)
	{
		if (isItIn(&genRect(pos.h,pos.w+64,pos.x-32,pos.y),sEvent.motion.x,sEvent.motion.y))
		{
			int my;
			int all;
			float ile;
			if (vertical)
			{
				my = sEvent.motion.y-(pos.y+16);
				all =pos.h-48;
				ile = (float)my / (float)all;
				if (ile>1) ile=1;
				if (ile<0) ile=0;
			}
			else
			{
				my = sEvent.motion.x-(pos.x+16);
				all =pos.w-48;
				ile = (float)my / (float)all;
				if (ile>1) ile=1;
				if (ile<0) ile=0;
			}
			int ktory = ile*(positionsAmnt-capacity);
			if (ktory!=whereAreWe)
			{
				whereAreWe=ktory;
				updateSlid();
				fun(whereAreWe);
			}
		}
	}

	/*else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		if (ourScenSel->pressed)
		{
			ourScenSel->pressed->press(false);
			ourScenSel->pressed=NULL;
		}
		for (int i=0;i<btns.size(); i++)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				if (btns[i]->selectable)
					btns[i]->select(true);
				if (btns[i]->fun)
					(this->*(btns[i]->fun))();
				return;
			}
		}


		if (isItIn(&down->pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*down->fun)();
		}
		if (isItIn(&up->pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*up->fun)();
		}
		if (isItIn(&slider->pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*slider->fun)();
		}

	}*/
}

Slider::~Slider()
{
	delete up;
	delete down;
	delete slider;
}
IntBut::IntBut()
{
	type=2;
	fun=NULL;
	highlightable=false;
}
void IntBut::set()
{
	*what=key;
}
void CPoinGroup::setYour(IntSelBut * your)
{
	*gdzie=your->key;
};
IntSelBut::IntSelBut( SDL_Rect Pos, boost::function<void()> Fun,CDefHandler* Imgs, bool Sel, CPoinGroup* gr, int My)
	: Button(Pos,Fun,Imgs,Sel,gr),key(My)
{
	ourPoinGroup=gr;
};
void IntSelBut::select(bool on) 
{
	Button::select(on);
	ourPoinGroup->setYour(this);
	CPG->printRating();
}
/********************************************************************************************/
void PreGameTab::show()
{
	if (CPG->currentTab)
		CPG->currentTab->hide();
	showed=true;
	CPG->currentTab=this;
}
void PreGameTab::hide()
{
	showed=false;
	CPG->currentTab=NULL;
}
PreGameTab::PreGameTab()
{
	showed=false;
}
/********************************************************************************************/
Options::PlayerOptions::PlayerOptions(int serial, int player)
	:flag(genRect(50,42,14,130+serial*50),CPG->ourOptions->flags[player],player),
	Cleft(genRect(24,11,164,133+serial*50),CPG->ourOptions->left,true,-1), //left castle arrow
	Cright(genRect(24,11,225,133+serial*50),CPG->ourOptions->right,false,-1), //right castle arrow
	Hleft(genRect(24,11,240,133+serial*50),CPG->ourOptions->left,true,0), //left hero arrow
	Hright(genRect(24,11,301,133+serial*50),CPG->ourOptions->right,false,0), //right hero arrow
	Bleft(genRect(24,11,316,133+serial*50),CPG->ourOptions->left,true,1), //left bonus arrow
	Bright(genRect(24,11,377,133+serial*50),CPG->ourOptions->right,false,1) //right bonus arrow
{
	Bleft.playerID=Bright.playerID=Hleft.playerID=Hright.playerID=Cleft.playerID=Cright.playerID=player;
	Bleft.serialID=Bright.serialID=Hleft.serialID=Hright.serialID=Cleft.serialID=Cright.serialID=serial;
}
bool Options::canUseThisHero(int ID)
{
	//TODO: check if hero is allowed on selected map
	for(int i=0;i<CPG->ret.playerInfos.size();i++)
		if(CPG->ret.playerInfos[i].hero==ID)
			return false;
	return (usedHeroes.find(ID) == usedHeroes.end());
}
int Options::nextAllowedHero(int min, int max, int incl, int dir) //incl 0 - wlacznie; incl 1 - wylacznie; min-max - zakres szukania
{
	if(dir>0)
	{
		for(int i=min+incl; i<=max-incl; i++)
		{
			if(canUseThisHero(i))
				return i;
		}
	}
	else
	{
		for(int i=max-incl; i>=min+incl; i--)
		{
			if(canUseThisHero(i))
				return i;
		}
	}
	return -1;
}

void Options::OptionSwitch::press(bool down)
{
	HighButton::press(down);
	PlayerSettings * ourOpt = &CPG->ret.playerInfos[serialID];
	PlayerInfo * ourInf = &CPG->ourScenSel->mapsel.selMaps[CPG->ourScenSel->mapsel.selected]->players[playerID];
	int dir = (left) ? (-1) : (1);
	if (down) return;
	switch (which) //which button is this?
	{
	case -1: //castle change
		{
			int nCas = getNextCastle(ourOpt->castle,ourInf,!left);

			if (nCas!=ourOpt->castle) //changed castle
			{
				ourOpt->castle = nCas;
				if(ourOpt->hero != -2)
				{
					ourOpt->hero=-1;
					CPG->ourOptions->showIcon(0,serialID,false);
				}
				if(nCas < 0 && ourOpt->bonus==bresource)
				{
					ourOpt->bonus = brandom;
				}
				CPG->ourOptions->showIcon(1,serialID,false);
			}
			break;
		}
	case 0: //hero change
		{
			if (ourOpt->castle<0
				|| !ourOpt->human
				)
			{
				break;
			}
			if (ourOpt->hero==-2) //no hero - no change
				return;
			else if (ourOpt->hero==-1) //random => first/last available
			{
				int max = (ourOpt->castle*HEROES_PER_TYPE*2+15),
					min = (ourOpt->castle*HEROES_PER_TYPE*2);
				ourOpt->hero = CPG->ourOptions->nextAllowedHero(min,max,0,dir);
			}
			else
			{
				if(dir>0)
					ourOpt->hero = CPG->ourOptions->nextAllowedHero(ourOpt->hero,(ourOpt->castle*HEROES_PER_TYPE*2+16),1,dir);
				else
					ourOpt->hero = CPG->ourOptions->nextAllowedHero(ourOpt->castle*HEROES_PER_TYPE*2-1,ourOpt->hero,1,dir);
			}
			break;
		}
	case 1: //bonus change
		{
			ourOpt->bonus += dir;

			if (ourOpt->hero==-2 && !ourInf->heroesNames.size() && ourOpt->bonus==bartifact) //no hero - can't be artifact
			{
				if (dir<0)
					ourOpt->bonus=brandom;
				else ourOpt->bonus=bgold;
			}

			if(ourOpt->bonus > bresource)
				ourOpt->bonus = brandom;
			if(ourOpt->bonus < brandom)
				ourOpt->bonus = bresource;

			if (ourOpt->castle==-1 && ourOpt->bonus==bresource) //random castle - can't be resource
			{
				if (dir<0)
					ourOpt->bonus=bgold;
				else ourOpt->bonus=brandom;
			}

			break;
		}
	}
	CPG->ourOptions->showIcon(which,serialID,false);
}
void Options::PlayerFlag::press(bool down)
{
	HighButton::press(down);
	size_t i=0;
	for(;i<CPG->ret.playerInfos.size();i++)
		if(CPG->ret.playerInfos[i].color==color)
			break;
	if (CPG->ret.playerInfos[i].human || (!CPG->ourScenSel->mapsel.selMaps[CPG->ourScenSel->mapsel.selected]->players[CPG->ret.playerInfos[i].color].canHumanPlay))
		return; //if this is already human player, or if human is forbidden
	size_t j=0;
	for(;j<CPG->ret.playerInfos.size();j++)
		if(CPG->ret.playerInfos[j].human)
			break;
	CPG->ret.playerInfos[i].human = true;
	CPG->ret.playerInfos[j].human = false;
	if(CPG->ret.playerInfos[j].hero >= 0)
	{
		CPG->ret.playerInfos[j].hero = -1;
		CPG->ourOptions->showIcon(0,j,false);
	}
	std::string pom = CPG->ret.playerInfos[i].name;
	CPG->ret.playerInfos[i].name = CPG->ret.playerInfos[j].name;
	CPG->ret.playerInfos[j].name = pom;
	SDL_BlitSurface(CPG->ourOptions->bgs[CPG->ret.playerInfos[i].color],&genRect(19,99,5,1),screen,&genRect(19,99,62,129+i*50));
	SDL_UpdateRect(screen,62,129+50*i,99,19);
	SDL_BlitSurface(CPG->ourOptions->bgs[CPG->ret.playerInfos[j].color],&genRect(19,99,5,1),screen,&genRect(19,99,62,129+j*50));
	SDL_UpdateRect(screen,62,129+50*j,99,19);
	CSDL_Ext::printAtMiddle(CPG->ret.playerInfos[i].name,111,137+i*50,GEOR13,zwykly);
	CSDL_Ext::printAtMiddle(CPG->ret.playerInfos[j].name,111,137+j*50,GEOR13,zwykly);
	CPG->playerColor = CPG->ret.playerInfos[i].color;
	CPG->ourScenSel->mapsel.printFlags();
};
void Options::PlayerFlag::hover(bool on)
{
	HighButton::hover(on);
}
void Options::showIcon (int what, int nr, bool abs) //what: -1=castle, 0=hero, 1=bonus, 2=all; abs=true -> nr is absolute
{
	if (what==-2)
	{
		showIcon(-1,nr,abs);
		showIcon(0,nr,abs);
		showIcon(1,nr,abs);
	}
	int ab, se;
	if (!abs)
	{
		ab = CPG->ret.playerInfos[nr].color;
		se = nr;
	}
	else
	{
		ab = nr;
		for (size_t i=0; i<CPG->ret.playerInfos.size();++i)
		{
			if (CPG->ret.playerInfos[i].color==nr)
			{
				se=i;
				break;
			}
		}
	}
	PlayerSettings * ourOpt = &CPG->ret.playerInfos[se];
	switch (what)
	{
	case -1:
		{
			int pom=ourOpt->castle;
			if (ourOpt->castle<F_NUMBER && ourOpt->castle>=0)
			{
				blitAtWR(graphics->getPic(ourOpt->castle,true,false),176,130+50*se);
			}
			else if (ourOpt->castle==-1)
			{
				blitAtWR(CPG->ourOptions->rCastle,176,130+50*se);
			}
			else if (ourOpt->castle==-2)
			{
				blitAtWR(CPG->ourOptions->nCastle,176,130+50*se);
			}
			break;
		}
	case 0:
		{
			int pom=ourOpt->hero;
			if (ourOpt->hero==-1)
			{
				blitAtWR(CPG->ourOptions->rHero,252,130+50*se);
			}
			else if (ourOpt->hero==-2)
			{
				if(ourOpt->heroPortrait>=0)
				{
					blitAtWR(graphics->portraitSmall[ourOpt->heroPortrait],252,130+50*se);
				}
				else
				{
					blitAtWR(CPG->ourOptions->nHero,252,130+50*se);
				}
			}
			else
			{
				blitAtWR(graphics->portraitSmall[pom],252,130+50*se);
			}
			break;
		}
	case 1:
		{
			int pom;
			switch (ourOpt->bonus)
			{
			case -1:
				pom=10;
				break;
			case 0:
				pom=9;
				break;
			case 1:
				pom=8;
				break;
			case 2:
				pom=CGI->townh->towns[ourOpt->castle].bonus;
				break;
			}
			blitAtWR(bonuses->ourImages[pom].bitmap,328,130+50*se);
			break;
		}
	}
}
Options::~Options()
{
	if (!inited) {
            return;
        }
	for (size_t i=0; i<bgs.size();i++) {
		SDL_FreeSurface(bgs[i]);
        }
	for (size_t i=0; i<flags.size();i++) {
		delete flags[i];
        }
	SDL_FreeSurface(bg);
	SDL_FreeSurface(rHero);
	SDL_FreeSurface(rCastle);
	SDL_FreeSurface(nHero);
	SDL_FreeSurface(nCastle);
	delete turnLength;
	delete left;
	delete right;
	delete bonuses;
}
void Options::init()
{
	inited=true;
	bg = BitmapHandler::loadBitmap("ADVOPTBK.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	left = CDefHandler::giveDef("ADOPLFA.DEF");
	right = CDefHandler::giveDef("ADOPRTA.DEF");
	bonuses = CDefHandler::giveDef("SCNRSTAR.DEF");
	rHero = BitmapHandler::loadBitmap("HPSRAND1.bmp");
	rCastle = BitmapHandler::loadBitmap("HPSRAND0.bmp");
	nHero = BitmapHandler::loadBitmap("HPSRAND6.bmp");
	nCastle = BitmapHandler::loadBitmap("HPSRAND5.bmp");
	turnLength = new Slider(57,557,195,11,1,false);
	turnLength->fun=boost::bind(&CPreGame::setTurnLength,CPG,_1);

	flags.push_back(CDefHandler::giveDef("AOFLGBR.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBB.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBY.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBG.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBO.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBP.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBT.DEF"));
	flags.push_back(CDefHandler::giveDef("AOFLGBS.DEF"));

	bgs.push_back(BitmapHandler::loadBitmap("ADOPRPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPBPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPYPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPGPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPOPNL.bmp")); 
	bgs.push_back(BitmapHandler::loadBitmap("ADOPPPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPTPNL.bmp"));
	bgs.push_back(BitmapHandler::loadBitmap("ADOPSPNL.bmp"));
}
void Options::show()
{
	if (showed)
		return;

	PreGameTab::show();
	MapSel & ms = CPG->ourScenSel->mapsel;
	blitAt(bg,3,6);
	CPG->ourScenSel->listShowed=false;
	for (size_t i=0; i < CPG->btns.size(); ++i)
	{
		if (CPG->btns[i]->ID!=10) //leave only right panel buttons
		{
			CPG->btns.erase(CPG->btns.begin()+i);
			i--;
		}
	}
	CPG->interested.clear();
	CSDL_Ext::printAtMiddle("Advanced Options",225,35,GEORXX);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[521],224,544,GEOR13); // Player Turn Duration

	int playersSoFar = 0;

	for (size_t i=0; i < PLAYER_LIMIT; ++i)
	{
		PlayerInfo &curInfo = ms.selMaps[ms.selected]->players[i]; //map information for current player

		if(!(curInfo.canComputerPlay || curInfo.canComputerPlay)) //no one can play - player not present on the map (empty entry)
			continue;

		for(size_t hi=0; hi<curInfo.heroesNames.size(); hi++)
			usedHeroes.insert(curInfo.heroesNames[hi].heroID);

		blitAt(bgs[i],57,128+playersSoFar*50);

		PlayerOptions *options = new PlayerOptions(playersSoFar,i); //new options entry for current player
		options->nr = playersSoFar;
		options->color = (Ecolor)i;
		poptions.push_back(options);

		assert(CPG->ret.playerInfos.size() > playersSoFar);
		PlayerSettings &curSettings = CPG->ret.playerInfos[playersSoFar]; //settings entry for current players

		if(curSettings.hero == -1)
		{
			options->Hleft.show();
			options->Hright.show();
			CPG->btns.push_back(&options->Hleft);
			CPG->btns.push_back(&options->Hright);
		}

		if(getNextCastle(curSettings.castle,&curInfo) != curSettings.castle)
		{
			options->Cleft.show();
			options->Cright.show();
			CPG->btns.push_back(&options->Cleft);
			CPG->btns.push_back(&options->Cright);
		}

		options->Bleft.show();
		options->Bright.show();
		CPG->btns.push_back(&options->Bleft);
		CPG->btns.push_back(&options->Bright);

		CSDL_Ext::printAtMiddle(curSettings.name,111,137+playersSoFar*50,GEOR13,zwykly);
		if (curInfo.canHumanPlay)
		{
			options->flag.show();
			CPG->btns.push_back(&options->flag);
			if(curInfo.canComputerPlay) 
				CSDL_Ext::printAtMiddleWB("Human or CPU",86,163+playersSoFar*50,GEORM,7,zwykly);
			else 
				CSDL_Ext::printAtMiddleWB("Human",86,163+playersSoFar*50,GEORM,6,zwykly);
		}
		else 
		{
			CSDL_Ext::printAtMiddleWB("CPU",86,163+playersSoFar*50,GEORM,6,zwykly);
		}
		playersSoFar++;
	}

	CSDL_Ext::printAtMiddleWB(CGI->generaltexth->allTexts[516],221,63,GEOR13,55,zwykly);
	CSDL_Ext::printAtMiddleWB(CGI->generaltexth->getTitle(CGI->generaltexth->zelp[256].second),109,109,GEOR13,14);
	CSDL_Ext::printAtMiddleWB(CGI->generaltexth->getTitle(CGI->generaltexth->zelp[259].second),201,109,GEOR13,10);
	CSDL_Ext::printAtMiddleWB(CGI->generaltexth->getTitle(CGI->generaltexth->zelp[260].second),275,109,GEOR13,10);
	CSDL_Ext::printAtMiddleWB(CGI->generaltexth->getTitle(CGI->generaltexth->zelp[261].second),354,109,GEOR13,10);
	turnLength->activate();

	for (size_t i=0; i < poptions.size(); ++i) 
		showIcon(-2,i,false);

	for(int i=0;i<12;i++)
		turnLength->moveDown();

	CSDL_Ext::update(screen);
}
void Options::hide()
{
	if (!showed) {
            return;
        }
	PreGameTab::hide();
	for (size_t i=0; i < CPG->btns.size(); ++i)
		if (CPG->btns[i]->ID==7)
			CPG->btns.erase(CPG->btns.begin()+i--);
	for (size_t i=0;i<poptions.size();i++) {
		delete poptions[i];
        }
	poptions.clear();
	turnLength->deactivate();
}
MapSel::~MapSel()
{
	SDL_FreeSurface(bg);
	for (size_t i=0; i < scenImgs.size(); ++i) 
	{
		SDL_FreeSurface(scenImgs[i]);
	}
	delete sFlags;
}

// Display the tab with the scenario names
//
// elemIdx is the index of the maps or saved game to display on line 0
// slid->capacity contains the number of available screen lines
// slid->positionsAmnt is the number of elements after filtering
void MapSel::printMaps(int elemIdx)
{
	// Display all elements if there's enough space
	if(slid->positionsAmnt < slid->capacity)
		elemIdx = 0;

	SDL_Surface * scenin = CSDL_Ext::newSurface(351,25);
	SDL_Color nasz;

	for (int line=0; line<slid->capacity; elemIdx++)
	{
		if (elemIdx >= selMaps.size()) {
			// No elements left to display, so it's an empty line.
			SDL_BlitSurface(bg, &genRect(25, 351, 22, 115+line*25), scenin, NULL);
			blitAt(scenin, 25, 121+line*25);
			line ++;
			continue;
		}

		CMapInfo* curMap = selMaps[elemIdx];

		// Build the line to display.

		if (elemIdx == selected)
			nasz=tytulowy;
		else 
			nasz=zwykly;

		SDL_BlitSurface(bg,&genRect(25, 351, 22, line*25+115), scenin, NULL);

		std::ostringstream ostr(std::ostringstream::out);
		ostr << curMap->playerAmnt << "/" << curMap->humenPlayers;
		CSDL_Ext::printAt(ostr.str(),6,4,GEOR13,nasz,scenin, 2);

		std::string temp2;
		switch (curMap->width)
		{
		case 36:
			temp2="S";
			break;
		case 72:
			temp2="M";
			break;
		case 108:
			temp2="L";
			break;
		case 144:
			temp2="XL";
			break;
		default:
			temp2="C";
			break;
		}
		CSDL_Ext::printAtMiddle(temp2,50,13,GEOR13,nasz,scenin, 2);

		int temp=-1;
		switch (curMap->version)
		{
		case CMapHeader::RoE:
			temp=0;
			break;
		case CMapHeader::AB:
			temp=1;
			break;
		case CMapHeader::SoD:
			temp=2;
			break;
		case CMapHeader::WoG:
			temp=3;
			break;
		default:
			// Unknown version. Be safe and ignore that map
			tlog2 << "Warning: " << curMap->filename << " has wrong version!\n";
			continue;
		}
		blitAt(Dtypes->ourImages[temp].bitmap,67,2,scenin);

		if (CPG->fromMenu == CPG->newGame)
		{
			if (!(curMap->name.length()))
				curMap->name = "Unnamed";
			CSDL_Ext::printAtMiddle(curMap->name,192,13,GEOR13,nasz,scenin, 2);
		}
		else
		{
			std::string &name = curMap->filename;
			CSDL_Ext::printAtMiddle(name.substr(6,name.size()-12),192,13,GEOR13,nasz,scenin, 2);
		}

		if (curMap->victoryCondition.condition == winStandard)
			temp=11;
		else 
			temp=curMap->victoryCondition.condition;
		blitAt(Dvic->ourImages[temp].bitmap,285,2,scenin);

		if (curMap->lossCondition.typeOfLossCon == lossStandard)
			temp=3;
		else 
			temp=curMap->lossCondition.typeOfLossCon;
		blitAt(Dloss->ourImages[temp].bitmap,318,2,scenin);

		blitAt(scenin,25,121+line*25);
		line ++;
	}
	SDL_FreeSurface(scenin);
	SDL_UpdateRect(screen, 25, 121, 351, 19*25);
}

void MapSel::hide()
{
	if (!showed)return;
	PreGameTab::hide();
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&small));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&medium));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&large));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&xlarge));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&all));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&nrplayer));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&mapsize));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&type));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&name));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&viccon));
	CPG->btns.erase(std::find(CPG->btns.begin(),CPG->btns.end(),&loscon));
	slid->deactivate();
	CPG->currentTab = NULL;
};
void MapSel::show()
{
	if (showed)return;
	PreGameTab::show();
	//blit bg
	blitAt(bg,3,6);
	CSDL_Ext::printAt("Map Sizes",55,60,GEOR13);
	CSDL_Ext::printAt(CGI->generaltexth->arraytxt[CPG->fromMenu==CPG->newGame ? 229 : 230],110,25,TNRB16); //Select a Scenario to Play : Load a Saved Game
	//size buttons
	small.show();
	medium.show();
	large.show();
	xlarge.show();
	all.show();
	CPG->btns.push_back(&small);
	CPG->btns.push_back(&medium);
	CPG->btns.push_back(&large);
	CPG->btns.push_back(&xlarge);
	CPG->btns.push_back(&all);
	//sort by buttons
	nrplayer.show();
	mapsize.show();
	type.show();
	name.show();
	viccon.show();
	loscon.show();
	CPG->btns.push_back(&nrplayer);
	CPG->btns.push_back(&mapsize);
	CPG->btns.push_back(&type);
	CPG->btns.push_back(&name);
	CPG->btns.push_back(&viccon);
	CPG->btns.push_back(&loscon);

	//print scenario list
	printMaps();

	slid->whereAreWe = 0;
	slid->activate();

	//SDL_Flip(screen);
	CSDL_Ext::update(screen);
}
void MapSel::processMaps(const std::vector<std::string> &pliczkiTemp, int start, int threads)
{
	int read=0;
	unsigned char sss[1000];
	while(true)
	{
		if(start >= ourMaps.size())
			break;
		ourMaps[start] = NULL;
		gzFile tempf = gzopen(pliczkiTemp[start].c_str(),"rb");
		read = gzread(tempf, sss, 1000);
		gzclose(tempf);
		if(read < 50)
		{
			tlog3<<"\t\tWarning: corrupted map file: "<<pliczkiTemp[start]<<std::endl; 
		}
		else if (sss[4]) //valid map
		{
			CMapInfo *mi = new CMapInfo(pliczkiTemp[start],sss);
			ourMaps[start] = mi;
		}
		start += threads;
	}
}

void MapSel::processGames(const std::vector<std::string> &pliczkiTemp)
{
	ourGames.resize(pliczkiTemp.size());
	ui32 hlp;

	for(int i=0; i<pliczkiTemp.size(); i++)
	{
		CLoadFile lf(pliczkiTemp[i]);
		if(!lf.sfile)
			continue;

		ui8 sign[8]; 
		lf >> sign;
		if(std::memcmp(sign,"VCMISVG",7))
		{
			tlog1 << pliczkiTemp[i] << " is not a correct savefile!" << std::endl;
			continue;
		}
		ourGames[i] = new CMapInfo();
		lf >> static_cast<CMapHeader&>(*ourGames[i]) >> ourGames[i]->seldiff;
		ourGames[i]->filename = pliczkiTemp[i];
		ourGames[i]->countPlayers();
	}
}

static bool isNull(CMapInfo*mi)
{
	return mi==NULL;
}

void MapSel::init()
{
	//get map files names
	std::vector<std::string> pliczkiTemp;
	if(!boost::filesystem::exists("Maps"))
	{
		tlog1 << "Cannot find /Maps directory!\n";
	}
	fs::path tie( (fs::initial_path<fs::path>())/"Maps" );
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator dir (tie); dir!=end_iter; ++dir )
	{
		if (fs::is_regular_file(dir->status()));
		{
			if (boost::ends_with(dir->path().filename(),".h3m"))
				pliczkiTemp.push_back("Maps/"+(dir->path().leaf()));
		}
	}
	ourMaps.resize(pliczkiTemp.size());
	boost::thread_group group;
	if(pliczkiTemp.size())
	{
		int threads = std::max((unsigned int)1,boost::thread::hardware_concurrency());
		for(int ti=0;ti<threads;ti++)
			group.create_thread(boost::bind(&MapSel::processMaps,this,boost::ref(pliczkiTemp),ti,threads));
	}
	else
	{
		tlog1 << "No maps in the /Maps directory!\n";
	}

	bg = BitmapHandler::loadBitmap("SCSELBCK.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	small.imgs = CDefHandler::giveDef("SCSMBUT.DEF");
	small.fun = NULL;
	small.pos = genRect(small.imgs->ourImages[0].bitmap->h,small.imgs->ourImages[0].bitmap->w,161,52);
	small.ourGroup=NULL;
	medium.imgs = CDefHandler::giveDef("SCMDBUT.DEF");
	medium.fun = NULL;
	medium.pos = genRect(medium.imgs->ourImages[0].bitmap->h,medium.imgs->ourImages[0].bitmap->w,208,52);
	medium.ourGroup=NULL;
	large.imgs = CDefHandler::giveDef("SCLGBUT.DEF");
	large.fun = NULL;
	large.pos = genRect(large.imgs->ourImages[0].bitmap->h,large.imgs->ourImages[0].bitmap->w,255,52);
	large.ourGroup=NULL;
	xlarge.imgs = CDefHandler::giveDef("SCXLBUT.DEF");
	xlarge.fun = NULL;
	xlarge.pos = genRect(xlarge.imgs->ourImages[0].bitmap->h,xlarge.imgs->ourImages[0].bitmap->w,302,52);
	xlarge.ourGroup=NULL;
	all.imgs = CDefHandler::giveDef("SCALBUT.DEF");
	all.fun = NULL;
	all.pos = genRect(all.imgs->ourImages[0].bitmap->h,all.imgs->ourImages[0].bitmap->w,349,52);
	all.ourGroup=NULL;
	all.selectable=xlarge.selectable=large.selectable=medium.selectable=small.selectable=false;
	small.what=medium.what=large.what=xlarge.what=all.what=&sizeFilter;
	small.key=36;medium.key=72;large.key=108;xlarge.key=144;all.key=0;
//Button<> nrplayer, mapsize, type, name, viccon, loscon;
	nrplayer.imgs = CDefHandler::giveDef("SCBUTT1.DEF");
	nrplayer.fun = NULL;
	nrplayer.pos = genRect(nrplayer.imgs->ourImages[0].bitmap->h,nrplayer.imgs->ourImages[0].bitmap->w,26,92);
	nrplayer.key=_playerAm;

	mapsize.imgs = CDefHandler::giveDef("SCBUTT2.DEF");
	mapsize.fun = NULL;
	mapsize.pos = genRect(mapsize.imgs->ourImages[0].bitmap->h,mapsize.imgs->ourImages[0].bitmap->w,58,92);
	mapsize.key=_size;

	type.imgs = CDefHandler::giveDef("SCBUTCP.DEF");
	type.fun = NULL;
	type.pos = genRect(type.imgs->ourImages[0].bitmap->h,type.imgs->ourImages[0].bitmap->w,91,92);
	type.key=_format;

	name.imgs = CDefHandler::giveDef("SCBUTT3.DEF");
	name.fun = NULL;
	name.pos = genRect(name.imgs->ourImages[0].bitmap->h,name.imgs->ourImages[0].bitmap->w,124,92);
	name.key=_name;

	viccon.imgs = CDefHandler::giveDef("SCBUTT4.DEF");
	viccon.fun = NULL;
	viccon.pos = genRect(viccon.imgs->ourImages[0].bitmap->h,viccon.imgs->ourImages[0].bitmap->w,309,92);
	viccon.key=_viccon;

	loscon.imgs = CDefHandler::giveDef("SCBUTT5.DEF");
	loscon.fun = NULL;
	loscon.pos = genRect(loscon.imgs->ourImages[0].bitmap->h,loscon.imgs->ourImages[0].bitmap->w,342,92);
	loscon.key=_loscon;

	nrplayer.poin=mapsize.poin=type.poin=name.poin=viccon.poin=loscon.poin=(int*)(&sortBy);
	nrplayer.fun=mapsize.fun=type.fun=name.fun=viccon.fun=loscon.fun=boost::bind(&CPreGame::sortMaps,CPG);

	Dtypes = CDefHandler::giveDef("SCSELC.DEF");
	Dvic = CDefHandler::giveDef("SCNRVICT.DEF");
	Dloss = CDefHandler::giveDef("SCNRLOSS.DEF");
	//Dsizes = CPG->slh->giveDef("SCNRMPSZ.DEF");
	Dsizes = CDefHandler::giveDef("SCNRMPSZ.DEF");
	sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");

	slid = new Slider(375,92,480,ourMaps.size(),-1,true);
	slid->fun = boost::bind(&CPreGame::printMapsFrom,CPG,_1);

	group.join_all();
	std::vector<CMapInfo*>::iterator maps = std::remove_if(ourMaps.begin(),ourMaps.end(),isNull);
	ourMaps.erase(maps,ourMaps.end());
	std::sort(ourMaps.begin(),ourMaps.end(),mapSorter(_name));

	pliczkiTemp.clear();
	std::vector<std::string> datestemp;
	if(!boost::filesystem::exists("Games"))
	{
		tlog1 << "Cannot find /Games directory!\n";
	}
	tie = fs::path( (fs::initial_path<fs::path>())/"/Games" );
	for ( fs::directory_iterator dir (tie); dir!=end_iter; ++dir )
	{
		if (fs::is_regular_file(dir->status()));
		{
			if (boost::ends_with(dir->path().filename(),".vlgm1"))
			{
				if( fs::file_size(dir->path()) < 16000 )
				{
					tlog3 << "\t\tWarning: savegame " << dir->path().filename() << " seems to be corrupted and will be ommited.\n";
					continue;
				}
				pliczkiTemp.push_back("Games/"+(dir->path().leaf()));
				std::time_t time = fs::last_write_time(dir->path());
				datestemp.push_back(std::asctime(std::gmtime(&time)));
			}
		}
	}
	processGames(pliczkiTemp);
	for (int i = 0; i < ourGames.size(); i++)
	{
		if(ourGames[i])
			ourGames[i]->date = datestemp[i];
	}
	maps = std::remove_if(ourGames.begin(),ourGames.end(),isNull);
	ourGames.erase(maps,ourGames.end());
	std::sort(ourGames.begin(),ourGames.end(),mapSorter(_name));
}

// Move the list up or down by a specified amount (positive or negative).
void MapSel::moveByX(int nlines)
{
	selected += nlines;

	// Adjust selected
	if (selected < 0)
		selected = 0;
	else if (selected >= selMaps.size())
		selected = selMaps.size() - 1;

	// Since we want the selection to appear, we may have to change which
	// maps are to be displayed
	if (selected < slid->whereAreWe)
		slid->whereAreWe = selected;
	else if (selected >= slid->whereAreWe + slid->capacity) {
		slid->whereAreWe = selected - slid->capacity + 1;
		if (slid->whereAreWe >= slid->positionsAmnt)
			slid->whereAreWe = slid->positionsAmnt - 1;
	}

	select(selected);
	slid->updateSlid();
}

void MapSel::select(int which, bool updateMapsList, bool forceSettingsUpdate)
{
	if(which < 0  ||  which >= selMaps.size())
		// Empty list
		return;

	// If there's currently no default selection, make one
	if (selected == -1 && selMaps.size())
		selected = 0;

	bool dontSaveSettings = ((selected!=which) || (CPG->ret.playerInfos.size()==0) || forceSettingsUpdate);
	if (selected >= 0) 
	{
		selected = which;
		CPG->ret.mapname = selMaps[selected]->filename;
	}
	if(updateMapsList)
		printMaps(slid->whereAreWe);
	int serialC=0;
	if(selected >=0 && dontSaveSettings)
	{
		CPG->ret.playerInfos.clear();
		bool wasntpl = true;
		for (int i=0;i<PLAYER_LIMIT;i++)
		{
			if (!(selMaps[selected]->players[i].canComputerPlay 
				|| selMaps[selected]->players[i].canComputerPlay)	
			  )
				continue; // this caused some serious problems becouse of lack of simple bijection between two sets of player's numbers (one is returned by CPreGame, second is used in h3m)
			PlayerSettings pset;
			pset.color=(Ecolor)i;
			pset.serial = serialC;
			serialC++;
			pset.bonus=brandom;
			pset.castle=-2;

			if (selMaps[which]->players[i].canHumanPlay && wasntpl)
			{
				pset.name=CGI->generaltexth->allTexts[434]; //Player
				pset.human = true;
				CPG->playerColor = i;
				wasntpl = false;
			}
			else
			{
				pset.name=CGI->generaltexth->allTexts[468];//Computer
				pset.human = false;
			}

			for (int j=0;j<F_NUMBER;j++)
			{
				if (((int)pow((double)2,j))&selMaps[selected]->players[i].allowedFactions)
				{
					if (pset.castle>=0)
						pset.castle=-1;
					if (pset.castle==-2)
						pset.castle=j;
				}
			}
			pset.heroPortrait=-1;
			if (!
				(((selMaps[which]->players[i].generateHeroAtMainTown || selMaps[which]->version==CMapHeader::RoE)
					&& selMaps[which]->players[i].hasMainTown) 
				|| selMaps[which]->players[i].p8)
			  )
				pset.hero=-2;
			else
				pset.hero=-1;

			if(selMaps[which]->players[i].mainHeroName.length())
			{
				pset.heroName = selMaps[which]->players[i].mainHeroName;
				if((pset.heroPortrait = selMaps[which]->players[i].mainHeroPortrait)==255)
					pset.heroPortrait = selMaps[which]->players[i].p9;
			}
			pset.handicap=0;
			CPG->ret.playerInfos.push_back(pset);
		}
	}
	printSelectedInfo();

}
MapSel::MapSel():selected(-1),sizeFilter(0)
{
}
void MapSel::printSelectedInfo()
{
	SDL_BlitSurface(CPG->ourScenSel->scenInf,&genRect(399,337,17,23),screen,&genRect(399,337,413,29));
	SDL_BlitSurface(CPG->ourScenSel->scenInf,&genRect(50,91,18,447),screen,&genRect(50,91,414,453));

	//blit texts
	CSDL_Ext::printAt(CGI->generaltexth->zelp[21].second,420,25,GEOR13);
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[496],420,135,GEOR13);
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[497],420,285,GEOR13);
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[498],420,340,GEOR13);
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[390],420,406,GEOR13,zwykly);
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[391],585,406,GEOR13,zwykly);

	if(CPG->fromMenu==CPG->newGame)
	{
		SDL_BlitSurface(CPG->ourScenSel->bScens.imgs->ourImages[0].bitmap,NULL,screen,&CPG->ourScenSel->bScens.pos);
		SDL_BlitSurface(CPG->ourScenSel->bOptions.imgs->ourImages[0].bitmap,NULL,screen,&CPG->ourScenSel->bOptions.pos);
		SDL_BlitSurface(CPG->ourScenSel->bRandom.imgs->ourImages[0].bitmap,NULL,screen,&CPG->ourScenSel->bRandom.pos);
	}

	if (selected >= 0)
	{
		CMapInfo &selMap = 	*selMaps[selected];
		if(CPG->fromMenu != CPG->newGame)
		{
			CPG->ourScenSel->bEasy.state = 2 + (selMap.seldiff==0);
			CPG->ourScenSel->bNormal.state = 2 + (selMap.seldiff==1);
			CPG->ourScenSel->bHard.state = 2 + (selMap.seldiff==2);
			CPG->ourScenSel->bExpert.state = 2 + (selMap.seldiff==3);
			CPG->ourScenSel->bImpossible.state = 2 + (selMap.seldiff==4);
			CPG->ourScenSel->bEasy.show();
			CPG->ourScenSel->bNormal.show();
			CPG->ourScenSel->bHard.show();
			CPG->ourScenSel->bExpert.show();
			CPG->ourScenSel->bImpossible.show();
		}

		int temp = selMap.victoryCondition.condition+1;
		if (temp>20) temp=0;
		std::string sss = CGI->generaltexth->victoryConditions[temp];
		if (temp && selMap.victoryCondition.allowNormalVictory) sss+= "/" + CGI->generaltexth->victoryConditions[0];
		CSDL_Ext::printAt(sss,452,310,GEOR13,zwykly);


		temp = selMap.lossCondition.typeOfLossCon+1;
		if (temp>20) temp=0;
		sss = CGI->generaltexth->lossCondtions[temp];
		CSDL_Ext::printAt(sss,452,370,GEOR13,zwykly);

		//blit description
		std::vector<std::string> desc = *CMessage::breakText(selMap.description,50);
		for (int i=0;i<desc.size();i++)
			CSDL_Ext::printAt(desc[i],417,152+i*13,GEOR13,zwykly);

		if ((selected < 0) || (selected >= selMaps.size()))
			return;
		if (selMap.name.length())
			CSDL_Ext::printAt(selMap.name,420,41,GEORXX);
		else CSDL_Ext::printAt("Unnamed",420,41,GEORXX);

		assert(selMap.difficulty <= 4);
		std::string &diff = CGI->generaltexth->arraytxt[142 + selMap.difficulty];

		temp=-1;
		switch (selMap.width)
		{
		case 36:
			temp=0;
			break;
		case 72:
			temp=1;
			break;
		case 108:
			temp=2;
			break;
		case 144:
			temp=3;
			break;
		default:
			temp=4;
			break;
		}
		blitAt(Dsizes->ourImages[temp].bitmap,714,28);
		temp = selMap.victoryCondition.condition;
		if (temp>12) temp=11;
		blitAt(Dvic->ourImages[temp].bitmap,420,308); //v
		temp=selMap.lossCondition.typeOfLossCon;
		if (temp>12) temp=3;
		blitAt(Dloss->ourImages[temp].bitmap,420,366); //l

		CSDL_Ext::printAtMiddle(diff,458,477,GEOR13,zwykly);

		CSDL_Ext::printTo(selMap.date,704,40,GEOR13,zwykly);

		printFlags();
	}
	//SDL_Flip(screen);
	CSDL_Ext::update(screen);
}
void MapSel::printFlags()
{
	CMapInfo &selMap = *selMaps[selected];
	int hy=405, fx=460, ex=640, myT;
	if (selMap.howManyTeams)
		myT = selMap.players[CPG->playerColor].team;
	else myT = -1;
	for (int i=0;i<CPG->ret.playerInfos.size();i++)
	{
		if (myT>=0)
		{
			if(selMap.players[CPG->ret.playerInfos[i].color].team==myT)
			{
				blitAtWR(sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap,fx,hy);
				fx+=sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap->w;
			}
			else
			{
				blitAtWR(sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap,ex,hy);
				ex+=sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap->w;
			}
		}
		else
		{
			if(CPG->ret.playerInfos[i].color==CPG->playerColor)
			{
				blitAtWR(sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap,fx,hy);
				fx+=sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap->w;
			}
			else
			{
				blitAtWR(sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap,ex,hy);
				ex+=sFlags->ourImages[CPG->ret.playerInfos[i].color].bitmap->w;
			}
		}
	}
}
// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void MapSel::updateSelection()
{
	std::vector<CMapInfo*> &maps = CPG->fromMenu==CPG->newGame ? ourMaps : ourGames;

	selMaps.clear();

	if (sizeFilter) {
		// Filter in effect. Add only maps that matches.
		for (int i=0; i<maps.size(); i++) {
			if (maps[i]->width == sizeFilter) {
				selMaps.push_back(maps[i]);
			}
		}
	} else {
		// No size filter. Copy all maps pointers.
		selMaps = maps;
	}
}

void CPreGame::printRating()
{
	SDL_BlitSurface(CPG->ourScenSel->scenInf,&genRect(47,83,271,449),screen,&genRect(47,83,666,455));
	updateRect(&genRect(47,83,666,455));
	std::string tob;
	switch (ourScenSel->selectedDiff)
	{
	case 0:
		tob="80%";
		break;
	case 1:
		tob="100%";
		break;
	case 2:
		tob="130%";
		break;
	case 3:
		tob="160%";
		break;
	case 4:
		tob="200%";
		break;
	}
	CSDL_Ext::printAtMiddle(tob,703,477,GEOR13,zwykly);
}
void CPreGame::printMapsFrom(int from)
{
	ourScenSel->mapsel.printMaps(from);
}
void CPreGame::showScenList()
{
	if (currentTab != &ourScenSel->mapsel)
	{
		ourScenSel->listShowed=true;
		ourScenSel->mapsel.show();
	}
	else
	{
		currentTab->hide();
		showScenSel();
	}
}
CPreGame::CPreGame()
{
	CPG=this;
	highlighted=NULL;
	currentTab=NULL;
	run=true;
	timeHandler tmh;tmh.getDif();
	tytulowy.r=229;tytulowy.g=215;tytulowy.b=123;tytulowy.unused=0;
	zwykly.r=255;zwykly.g=255;zwykly.b=255;zwykly.unused=0; //gbr
	tlo.r=66;tlo.g=44;tlo.b=24;tlo.unused=0;
	currentMessage=NULL;
	behindCurMes=NULL;
	initMainMenu();
	tlog0<<"\tCPreGame: main menu initialization: "<<tmh.getDif()<<std::endl;
	initNewMenu();
	tlog0<<"\tCPreGame: newgame menu initialization: "<<tmh.getDif()<<std::endl;
	initLoadMenu();
	tlog0<<"\tCPreGame: loadgame menu initialization: "<<tmh.getDif()<<std::endl;
	initScenSel();
	tlog0<<"\tCPreGame: scenario choice initialization: "<<tmh.getDif()<<std::endl;
	initOptions();
	tlog0<<"\tCPreGame: scenario options initialization: "<<tmh.getDif()<<std::endl;
	playerName="Player";
}
void CPreGame::initOptions()
{
	ourOptions = new Options();
	ourOptions->init();
}
void CPreGame::initScenSel()
{
	ourScenSel = new ScenSel();
	tlog5 << "\t\tLoaded graphics\n";
	ourScenSel->mapsel.init();
	tlog5 << "\t\tLoaded maps\n";
}

// New Game or Load Game seletion..
void CPreGame::showScenSel()
{
	state=ScenarioList;

	ourScenSel->mapsel.updateSelection();
	ourScenSel->mapsel.slid->positionsAmnt = ourScenSel->mapsel.selMaps.size();
	SDL_BlitSurface(ourScenSel->background,NULL,screen,NULL);
	SDL_BlitSurface(ourScenSel->scenInf,NULL,screen,&genRect(ourScenSel->scenInf->h,ourScenSel->scenInf->w,396,6));
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[494],427,438,GEOR13);//"Map Diff:"
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[492],527,438,GEOR13); //player difficulty
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[218],685,438,GEOR13);//"Rating:"
	//blit buttons
	SDL_BlitSurface(ourScenSel->bEasy.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bEasy.pos);
	SDL_BlitSurface(ourScenSel->bNormal.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bNormal.pos);
	SDL_BlitSurface(ourScenSel->bHard.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bHard.pos);
	SDL_BlitSurface(ourScenSel->bExpert.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bExpert.pos);
	SDL_BlitSurface(ourScenSel->bImpossible.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bImpossible.pos);
	SDL_BlitSurface((fromMenu==newGame ? ourScenSel->bBegin : ourScenSel->bLoad).imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bBegin.pos);
	SDL_BlitSurface(ourScenSel->bBack.imgs->ourImages[0].bitmap,NULL,screen,&ourScenSel->bBack.pos);
	//blitAt(ourScenSel->bScens.imgs->ourImages[0].bitmap,ourScenSel->bScens.pos.x,ourScenSel->bScens.pos.y);
	//blitAt(ourScenSel->bRandom.imgs->ourImages[0].bitmap,414,105);
	//blitAt(ourScenSel->bOptions.imgs->ourImages[0].bitmap,414,509);
	//blitAt(ourScenSel->bBegin.imgs->ourImages[0].bitmap,414,535);
	//blitAt(ourScenSel->bBack.imgs->ourImages[0].bitmap,584,535);

	//add buttons info
	if(first)
	{
		if(fromMenu==newGame)
		{
			btns.push_back(&ourScenSel->bEasy);
			btns.push_back(&ourScenSel->bNormal);
			btns.push_back(&ourScenSel->bHard);
			btns.push_back(&ourScenSel->bExpert);
			btns.push_back(&ourScenSel->bImpossible);
			btns.push_back(&ourScenSel->bScens);
			btns.push_back(&ourScenSel->bRandom);
			btns.push_back(&ourScenSel->bOptions);
		}
		else
			ourScenSel->mapsel.show();
		btns.push_back(&(fromMenu==newGame ? ourScenSel->bBegin : ourScenSel->bLoad));
		btns.push_back(&ourScenSel->bBack);

		ourScenSel->selectedDiff=1;
		ourScenSel->bNormal.select(true);
		handleOther = &CPreGame::scenHandleEv;
		ourScenSel->mapsel.select(0,false);


		for (size_t i=0; i < btns.size(); ++i)
		{
			btns[i]->pos.w=btns[i]->imgs->ourImages[0].bitmap->w;
			btns[i]->pos.h=btns[i]->imgs->ourImages[0].bitmap->h;
			btns[i]->ID=10;
		}
	}
	else
	{
		ourScenSel->mapsel.select(ourScenSel->mapsel.selected,false);

		switch (ourScenSel->selectedDiff)
		{
		case 0:
			ourScenSel->bEasy.select(true);
			break;
		case 1:
			ourScenSel->bNormal.select(true);
			break;
		case 2:
			ourScenSel->bHard.select(true);
			break;
		case 3:
			ourScenSel->bExpert.select(true);
			break;
		case 4:
			ourScenSel->bImpossible.select(true);
			break;
		}

	}
	//ourScenSel->mapsel.printSelectedInfo(); // this is already called in select()
	//SDL_Flip(screen);
	CSDL_Ext::update(screen);
	first = false;
}
void CPreGame::showOptions()
{
	if (ourScenSel->mapsel.selected == -1)
		return;

	if (currentTab != ourOptions)
		ourOptions->show();
	else
	{
		currentTab->hide();
		showScenSel();
	}
}
void CPreGame::initNewMenu()
{
	ourNewMenu = new menuItems();
	ourNewMenu->bgAd = BitmapHandler::loadBitmap("ZNEWGAM.bmp");
	ourNewMenu->background = BitmapHandler::loadBitmap("ZPIC1005.bmp");
	blitAt(ourNewMenu->bgAd,114,312,ourNewMenu->background);
	 //loading menu buttons
	ourNewMenu->newGame = CDefHandler::giveDef("ZTSINGL.DEF");
	ourNewMenu->loadGame = CDefHandler::giveDef("ZTMULTI.DEF");
	ourNewMenu->highScores = CDefHandler::giveDef("ZTCAMPN.DEF");
	ourNewMenu->credits = CDefHandler::giveDef("ZTTUTOR.DEF");
	ourNewMenu->quit = CDefHandler::giveDef("ZTBACK.DEF");
	// single scenario
	ourNewMenu->lNewGame.h=ourNewMenu->newGame->ourImages[0].bitmap->h;
	ourNewMenu->lNewGame.w=ourNewMenu->newGame->ourImages[0].bitmap->w;
	ourNewMenu->lNewGame.x=545;
	ourNewMenu->lNewGame.y=4;
	ourNewMenu->fNewGame=&CPreGame::showScenSel;
	//multiplayer
	ourNewMenu->lLoadGame.h=ourNewMenu->loadGame->ourImages[0].bitmap->h;
	ourNewMenu->lLoadGame.w=ourNewMenu->loadGame->ourImages[0].bitmap->w;
	ourNewMenu->lLoadGame.x=568;
	ourNewMenu->lLoadGame.y=120;
	ourNewMenu->fLoadGame = NULL;
	//campaign
	ourNewMenu->lHighScores.h=ourNewMenu->highScores->ourImages[0].bitmap->h;
	ourNewMenu->lHighScores.w=ourNewMenu->highScores->ourImages[0].bitmap->w;
	ourNewMenu->lHighScores.x=541;
	ourNewMenu->lHighScores.y=233;
	ourNewMenu->fHighScores = NULL;
	//tutorial
	ourNewMenu->lCredits.h=ourNewMenu->credits->ourImages[0].bitmap->h;
	ourNewMenu->lCredits.w=ourNewMenu->credits->ourImages[0].bitmap->w;
	ourNewMenu->lCredits.x=545;
	ourNewMenu->lCredits.y=358;
	ourNewMenu->fCredits = NULL;
	//back
	ourNewMenu->lQuit.h=ourNewMenu->quit->ourImages[0].bitmap->h;
	ourNewMenu->lQuit.w=ourNewMenu->quit->ourImages[0].bitmap->w;
	ourNewMenu->lQuit.x=582;
	ourNewMenu->lQuit.y=464;
	ourNewMenu->fQuit=&CPreGame::showMainMenu;

	ourNewMenu->highlighted=0;
}
void CPreGame::showNewMenu()
{
	if(state == ScenarioList && fromMenu==loadGame)
	{
		showLoadMenu();
		return;
	}
	if (currentTab/*==&ourScenSel->mapsel*/)
		currentTab->hide();
	btns.clear();
	interested.clear();
	handleOther=NULL;
	state = newGame;
	fromMenu = newGame;
	ourScenSel->mapsel.slid->capacity = 18;
	SDL_BlitSurface(ourNewMenu->background,NULL,screen,NULL);
	SDL_BlitSurface(ourNewMenu->newGame->ourImages[0].bitmap,NULL,screen,&ourNewMenu->lNewGame);
	SDL_BlitSurface(ourNewMenu->loadGame->ourImages[0].bitmap,NULL,screen,&ourNewMenu->lLoadGame);
	SDL_BlitSurface(ourNewMenu->highScores->ourImages[0].bitmap,NULL,screen,&ourNewMenu->lHighScores);
	SDL_BlitSurface(ourNewMenu->credits->ourImages[0].bitmap,NULL,screen,&ourNewMenu->lCredits);
	SDL_BlitSurface(ourNewMenu->quit->ourImages[0].bitmap,NULL,screen,&ourNewMenu->lQuit);
	//SDL_Flip(screen);
	//CSDL_Ext::update(screen);
	first = true;
}
void CPreGame::initMainMenu()
{
	ourMainMenu = new menuItems();
	ourMainMenu->background = BitmapHandler::loadBitmap("ZPIC1005.bmp"); //SDL_LoadBMP("h3bitmap.lod\\ZPIC1005.bmp");
	 //loading menu buttons
	ourMainMenu->newGame = CDefHandler::giveDef("ZMENUNG.DEF");
	ourMainMenu->loadGame = CDefHandler::giveDef("ZMENULG.DEF");
	ourMainMenu->highScores = CDefHandler::giveDef("ZMENUHS.DEF");
	ourMainMenu->credits = CDefHandler::giveDef("ZMENUCR.DEF");
	ourMainMenu->quit = CDefHandler::giveDef("ZMENUQT.DEF");
	ok = CDefHandler::giveDef("IOKAY.DEF");
	cancel = CDefHandler::giveDef("ICANCEL.DEF");
	// new game button location
	ourMainMenu->lNewGame.h=ourMainMenu->newGame->ourImages[0].bitmap->h;
	ourMainMenu->lNewGame.w=ourMainMenu->newGame->ourImages[0].bitmap->w;
	ourMainMenu->lNewGame.x=540;
	ourMainMenu->lNewGame.y=10;
	ourMainMenu->fNewGame=&CPreGame::showNewMenu;
	//load game location
	ourMainMenu->lLoadGame.h=ourMainMenu->loadGame->ourImages[0].bitmap->h;
	ourMainMenu->lLoadGame.w=ourMainMenu->loadGame->ourImages[0].bitmap->w;
	ourMainMenu->lLoadGame.x=532;
	ourMainMenu->lLoadGame.y=132;
	ourMainMenu->fLoadGame=&CPreGame::showLoadMenu;
	//high scores
	ourMainMenu->lHighScores.h=ourMainMenu->highScores->ourImages[0].bitmap->h;
	ourMainMenu->lHighScores.w=ourMainMenu->highScores->ourImages[0].bitmap->w;
	ourMainMenu->lHighScores.x=524;
	ourMainMenu->lHighScores.y=251;
	ourMainMenu->fHighScores = NULL;
	//credits
	ourMainMenu->lCredits.h=ourMainMenu->credits->ourImages[0].bitmap->h;
	ourMainMenu->lCredits.w=ourMainMenu->credits->ourImages[0].bitmap->w;
	ourMainMenu->lCredits.x=557;
	ourMainMenu->lCredits.y=359;
	ourMainMenu->fCredits = NULL;
	//quit
	ourMainMenu->lQuit.h=ourMainMenu->quit->ourImages[0].bitmap->h;
	ourMainMenu->lQuit.w=ourMainMenu->quit->ourImages[0].bitmap->w;
	ourMainMenu->lQuit.x=586;
	ourMainMenu->lQuit.y=468;
	ourMainMenu->fQuit=&CPreGame::quitAskBox;

	ourMainMenu->highlighted=0;
}
void CPreGame::showMainMenu()
{
	state = mainMenu;
	SDL_BlitSurface(ourMainMenu->background,NULL,screen,NULL);
	SDL_BlitSurface(ourMainMenu->newGame->ourImages[0].bitmap,NULL,screen,&ourMainMenu->lNewGame);
	SDL_BlitSurface(ourMainMenu->loadGame->ourImages[0].bitmap,NULL,screen,&ourMainMenu->lLoadGame);
	SDL_BlitSurface(ourMainMenu->highScores->ourImages[0].bitmap,NULL,screen,&ourMainMenu->lHighScores);
	SDL_BlitSurface(ourMainMenu->credits->ourImages[0].bitmap,NULL,screen,&ourMainMenu->lCredits);
	SDL_BlitSurface(ourMainMenu->quit->ourImages[0].bitmap,NULL,screen,&ourMainMenu->lQuit);
	//SDL_Flip(screen);
	//CSDL_Ext::update(screen);
}
void CPreGame::highlightButton(int which, int on)
{

	menuItems * current = currentItems();
	switch (which)
	{
	case 1:
		{
			SDL_BlitSurface(current->newGame->ourImages[on].bitmap,NULL,screen,&current->lNewGame);
			break;
		}
	case 2:
		{
			SDL_BlitSurface(current->loadGame->ourImages[on].bitmap,NULL,screen,&current->lLoadGame);
			break;
		}
	case 3:
		{
			SDL_BlitSurface(current->highScores->ourImages[on].bitmap,NULL,screen,&current->lHighScores);
			break;
		}
	case 4:
		{
			SDL_BlitSurface(current->credits->ourImages[on].bitmap,NULL,screen,&current->lCredits);
			break;
		}
	case 5:
		{
			SDL_BlitSurface(current->quit->ourImages[on].bitmap,NULL,screen,&current->lQuit);
			break;
		}
	}
	//SDL_Flip(screen);
	//CSDL_Ext::update(screen);
}
void CPreGame::showCenBox (std::string data)
{
	CMessage * cmh = new CMessage();
	SDL_Surface * infoBox = cmh->genMessage(CGI->generaltexth->getTitle(data), CGI->generaltexth->getDescr(data));
	behindCurMes = CSDL_Ext::newSurface(infoBox->w,infoBox->h,screen);
	SDL_Rect pos = genRect(infoBox->h,infoBox->w,
		(screen->w/2)-(infoBox->w/2),(screen->h/2)-(infoBox->h/2));
	SDL_BlitSurface(screen,&pos,behindCurMes,NULL);
	SDL_BlitSurface(infoBox,NULL,screen,&pos);
	SDL_UpdateRect(screen,pos.x,pos.y,pos.w,pos.h);
	SDL_FreeSurface(infoBox);
	currentMessage = new SDL_Rect(pos);
	delete cmh;
}
void CPreGame::showAskBox (std::string data, void(*f1)(),void(*f2)())
{
	CMessage * cmh = new CMessage();
	std::vector<CDefHandler*> * przyciski = new std::vector<CDefHandler*>(0);
	std::vector<SDL_Rect> * btnspos= new std::vector<SDL_Rect>(0);
	przyciski->push_back(ok);
	przyciski->push_back(cancel);
	SDL_Surface * infoBox = cmh->genMessage(CGI->generaltexth->getTitle(data), CGI->generaltexth->getDescr(data), yesOrNO, przyciski, btnspos);
	behindCurMes = CSDL_Ext::newSurface(infoBox->w,infoBox->h,screen);
	SDL_Rect pos = genRect(infoBox->h,infoBox->w,
		(screen->w/2)-(infoBox->w/2),(screen->h/2)-(infoBox->h/2));
	SDL_BlitSurface(screen,&pos,behindCurMes,NULL);
	SDL_BlitSurface(infoBox,NULL,screen,&pos);
	SDL_UpdateRect(screen,pos.x,pos.y,pos.w,pos.h);
	SDL_FreeSurface(infoBox);
	currentMessage = new SDL_Rect(pos);
	(*btnspos)[0].x+=pos.x;
	(*btnspos)[0].y+=pos.y;
	(*btnspos)[1].x+=pos.x;
	(*btnspos)[1].y+=pos.y;
	btns.push_back(new Button((*btnspos)[0],boost::bind(&CPreGame::quit,this),ok,false, NULL,2));
	btns.push_back(new Button((*btnspos)[1],boost::bind(&CPreGame::hideBox,this),cancel,false, NULL,2));
	delete cmh;
	delete przyciski;
	delete btnspos;

}
void CPreGame::hideBox ()
{
	SDL_BlitSurface(behindCurMes,NULL,screen,currentMessage);
	SDL_UpdateRect
		(screen,currentMessage->x,currentMessage->y,currentMessage->w,currentMessage->h);
	for (size_t i=0; i < btns.size(); ++i)
	{
		if (btns[i]->ID==2)
		{
			delete btns[i];
			btns.erase(btns.begin()+i);
			i--;
		}
	}
	SDL_FreeSurface(behindCurMes);
	delete currentMessage;
	currentMessage = NULL;
	behindCurMes=NULL;
}
CPreGame::menuItems * CPreGame::currentItems()
{
	switch (state)
	{
	case mainMenu:
		return ourMainMenu;
	case newGame:
		return ourNewMenu;
	case loadGame:
		return ourLoadMenu;
	default:
		return NULL;
	}
}

void CPreGame::scenHandleEv(SDL_Event& sEvent)
{
	if(currentTab==&ourScenSel->mapsel)
	{
		if(sEvent.button.button == SDL_BUTTON_WHEELUP)
		{
			ourScenSel->mapsel.slid->moveUp();
			return;
		}
		else if(sEvent.button.button == SDL_BUTTON_WHEELDOWN)
		{
			ourScenSel->mapsel.slid->moveDown();
			return;
		}
	}
	if(sEvent.type == SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent.motion.x, sEvent.motion.y);
	}

	if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		for (size_t i=0; i < btns.size(); ++i)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				CGI->soundh->playSound(soundBase::button);
				btns[i]->press(true);
				ourScenSel->pressed=(Button*)btns[i];
			}
		}
		if ((currentTab==&ourScenSel->mapsel) && (sEvent.button.y>121) &&(sEvent.button.y<570)
									&& (sEvent.button.x>55) && (sEvent.button.x<372))
		{
			// New selection. py is the index in selMaps.
			int py = ((sEvent.button.y-121)/25)+ourScenSel->mapsel.slid->whereAreWe;

			if (py < ourScenSel->mapsel.slid->positionsAmnt) {
				CGI->soundh->playSound(soundBase::button);
				ourScenSel->mapsel.select(py);
			}
		}
	}
	else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		Button * prnr=ourScenSel->pressed;
		if (ourScenSel->pressed && ourScenSel->pressed->state==1)
		{
			ourScenSel->pressed->press(false);
			ourScenSel->pressed=NULL;
		}
		for (size_t i=0; i < btns.size(); ++i)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				if (btns[i]->selectable)
					btns[i]->select(true);
				if (btns[i]->type==1 && ((Button*)btns[i])->fun)
					((Button*)btns[i])->fun();
				int zz = btns.size();
				if (i>=zz)
					break;
				if  (btns[i]==prnr && btns[i]->type==2)
				{
					((IntBut*)(btns[i]))->set();
					ourScenSel->mapsel.updateSelection();
					ourScenSel->mapsel.slid->whereAreWe=0;
					ourScenSel->mapsel.slid->positionsAmnt=ourScenSel->mapsel.selMaps.size();
					ourScenSel->mapsel.slid->updateSlid();
					ourScenSel->mapsel.select(0);
					ourScenSel->mapsel.printMaps(0);
				}
			}
		}
	}
	else if (sEvent.type==SDL_MOUSEMOTION)
	{
		if (highlighted)
		{
			if (isItIn(&highlighted->pos,sEvent.motion.x,sEvent.motion.y))
				return;
			else
			{
				highlighted->hover(false);
				highlighted = NULL;
			}
		}
		for (size_t i=0; i < btns.size(); ++i)
		{
			if (!btns[i]->highlightable)
				continue;
			if(isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				btns[i]->hover(true);
				highlighted=btns[i];
				return;
			}
			else if (btns[i]->highlighted)
				btns[i]->hover(false);
		}
	}
}
StartInfo CPreGame::runLoop()
{
	SDL_Event sEvent;
	ret.turnTime = 0;

#ifdef _WIN32
	CGI->videoh->open("ACREDIT.SMK");
#else
	CGI->videoh->open("ACREDIT.SMK", true, false);
#endif

	while(run)
	{
		try
		{
			while(SDL_PollEvent(&sEvent))  //handle all events
			{
				menuItems * current = currentItems();
				if(sEvent.type==SDL_QUIT)
				{
					exit(EXIT_SUCCESS);
					return ret;
				}
				for (size_t i=0; i < interested.size(); ++i) {
					interested[i]->handleIt(sEvent);
                }
				if (!current)
				{
					(this->*handleOther)(sEvent);
				}
				else if (sEvent.type==SDL_KEYDOWN)
				{
					if (sEvent.key.keysym.sym==SDLK_q)
					{
						exit(EXIT_SUCCESS);
					}
					if(sEvent.key.keysym.sym==SDLK_F4 && (sEvent.key.keysym.mod & KMOD_LALT)) //Alt+F4
					{
						exit(EXIT_SUCCESS);
					}
					/*if (state==EState::newGame)
					{
						switch (sEvent.key.keysym.sym)
						{
						case SDLK_LEFT:
							{
								if(currentItems()->lNewGame.x>0)
									currentItems()->lNewGame.x--;
								break;
							}
						case (SDLK_RIGHT):
							{
									currentItems()->lNewGame.x++;
								break;
							}
						case (SDLK_UP):
							{
								if(currentItems()->lNewGame.y>0)
									currentItems()->lNewGame.y--;
								break;
							}
						case (SDLK_DOWN):
							{
									currentItems()->lNewGame.y++;
								break;
							}
						}
						showNewMenu();
					}*/
				}
				else if (sEvent.type==SDL_MOUSEMOTION)
				{
					CGI->curh->cursorMove(sEvent.motion.x, sEvent.motion.y); //for graphical mouse
					if (currentMessage) continue;
					if (current->highlighted)
					{
						switch (current->highlighted)
						{
							case 1:
								{
									if(isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(1,0);
									}
									break;
								}
							case 2:
								{
									if(isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(2,0);
									}
									break;
								}
							case 3:
								{
									if(isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(3,0);
									}
									break;
								}
							case 4:
								{
									if(isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(4,0);
									}
									break;
								}
							case 5:
								{
									if(isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(5,0);
									}
									break;
								}
						} //switch (current->highlighted)
					} // if (current->highlighted)
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						current->highlighted=1;
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						current->highlighted=2;
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						current->highlighted=5;
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (size_t i=0; i < btns.size(); ++i)
					{
						if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
						{
							btns[i]->press(true);
							//SDL_BlitSurface((btns[i].imgs)->ourImages[1].bitmap,NULL,screen,&btns[i].pos);
							//updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,1);
						current->highlighted=1;
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,1);
						current->highlighted=2;
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,1);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,1);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,1);
						current->highlighted=5;
					}
					if (current->highlighted)
						CGI->soundh->playSound(soundBase::button);
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (size_t i=0; i < btns.size(); ++i)
					{
						if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
						((Button*)btns[i])->fun();
						else
						{
							btns[i]->press(false);
							//SDL_BlitSurface((btns[i].imgs)->ourImages[0].bitmap,NULL,screen,&btns[i].pos);
							//updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						current->highlighted=1;
						if(current->fNewGame)
							(this->*(current->fNewGame))();
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						current->highlighted=2;
						if(current->fLoadGame)
							(this->*(current->fLoadGame))();
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						current->highlighted=5;
						if(current->fQuit)
							(this->*(current->fQuit))();
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_RIGHT))
				{
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(0));
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(1));
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(2));
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(3));
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(4));
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_RIGHT) && currentMessage)
				{
					hideBox();
				}
			}
		} HANDLE_EXCEPTION

		if(currentItems())
			CGI->videoh->update(8, 105, screen, true, false);

		CGI->curh->draw1();
		SDL_Flip(screen);
		CGI->curh->draw2();
		SDL_Delay(20); //give time for other apps
	}
	ret.mode = (fromMenu==newGame) ? 0 : 1;

	CGI->videoh->close();

	return ret;
}
std::string CPreGame::buttonText(int which)
{
	if (state==mainMenu)
	{
		switch (which)
		{
		case 0:
			return CGI->generaltexth->zelp[3].second;
		case 1:
			return CGI->generaltexth->zelp[4].second;
		case 2:
			return CGI->generaltexth->zelp[5].second;
		case 3:
			return CGI->generaltexth->zelp[6].second;
		case 4:
			return CGI->generaltexth->zelp[7].second;
		}
	}
	else if (state==newGame || state==loadGame)
	{
		switch (which)
		{
		case 0:
			return CGI->generaltexth->zelp[10].second;
		case 1:
			return CGI->generaltexth->zelp[11].second;
		case 2:
			return CGI->generaltexth->zelp[12].second;
		case 3:
			return CGI->generaltexth->zelp[13].second;
		case 4:
			return CGI->generaltexth->zelp[14].second;
		}
	}
	return std::string();
}
void CPreGame::quitAskBox()
{
	showAskBox("\"{}  Are you sure you want to quit?\"",NULL,NULL);
}

// Sort the maps on the new key (map size, map name, ...)
void CPreGame::sortMaps()
{
	std::vector<CMapInfo*> &sm = ourScenSel->mapsel.selMaps;

	std::sort(sm.begin(), sm.end(), mapSorter(_name));
	if (ourScenSel->mapsel.sortBy != _name)
		std::stable_sort(sm.begin(), sm.end(),mapSorter(ourScenSel->mapsel.sortBy));
	ourScenSel->mapsel.select(0,false,true);
	ourScenSel->mapsel.slid->whereAreWe=0;
	ourScenSel->mapsel.slid->updateSlid();
	printMapsFrom(0);
}

void CPreGame::setTurnLength(int on)
{
	int min;
	switch (on)
	{
	case 0:
		min=1;
		break;
	case 1:
		min=2;
		break;
	case 2:
		min=4;
		break;
	case 3:
		min=6;
		break;
	case 4:
		min=8;
		break;
	case 5:
		min=10;
		break;
	case 6:
		min=15;
		break;
	case 7:
		min=20;
		break;
	case 8:
		min=25;
		break;
	case 9:
		min=30;
		break;
	case 10:
		min=0;
		break;
	default:
		min=0;
		break;
	}
	SDL_BlitSurface(ourOptions->bg,&genRect(23,134,256,547),screen,&genRect(23,134,258,553));
	updateRect(&genRect(23,134,258,553));
	if (min)
	{
		std::ostringstream os;
		os<<min<<" Minutes";
		CSDL_Ext::printAtMiddle(os.str(),323,563,GEOR13);
	}
	else CSDL_Ext::printAtMiddle("Unlimited",323,563,GEOR13);
}

void CPreGame::showLoadMenu()
{
	if (currentTab/*==&ourScenSel->mapsel*/)
		currentTab->hide();
	btns.clear();
	interested.clear();
	handleOther=NULL;
	state = loadGame;
	fromMenu = loadGame;
	ourScenSel->mapsel.slid->capacity = 18;
	SDL_BlitSurface(ourLoadMenu->background,NULL,screen,NULL);
	SDL_BlitSurface(ourLoadMenu->newGame->ourImages[0].bitmap,NULL,screen,&ourLoadMenu->lNewGame);
	SDL_BlitSurface(ourLoadMenu->loadGame->ourImages[0].bitmap,NULL,screen,&ourLoadMenu->lLoadGame);
	SDL_BlitSurface(ourLoadMenu->highScores->ourImages[0].bitmap,NULL,screen,&ourLoadMenu->lHighScores);
	SDL_BlitSurface(ourLoadMenu->credits->ourImages[0].bitmap,NULL,screen,&ourLoadMenu->lCredits);
	SDL_BlitSurface(ourLoadMenu->quit->ourImages[0].bitmap,NULL,screen,&ourLoadMenu->lQuit);
	//SDL_Flip(screen);
	//CSDL_Ext::update(screen);
	first = true;
}

void CPreGame::initLoadMenu()
{
	ourLoadMenu = new menuItems();
	ourLoadMenu->bgAd = BitmapHandler::loadBitmap("ZLOADGAM.bmp");
	ourLoadMenu->background = BitmapHandler::loadBitmap("ZPIC1005.bmp");
	blitAt(ourLoadMenu->bgAd,114,312,ourLoadMenu->background);
	//loading menu buttons
	ourLoadMenu->newGame = CDefHandler::giveDef("ZTSINGL.DEF");
	ourLoadMenu->loadGame = CDefHandler::giveDef("ZTMULTI.DEF");
	ourLoadMenu->highScores = CDefHandler::giveDef("ZTCAMPN.DEF");
	ourLoadMenu->credits = CDefHandler::giveDef("ZTTUTOR.DEF");
	ourLoadMenu->quit = CDefHandler::giveDef("ZTBACK.DEF");
	// single scenario
	ourLoadMenu->lNewGame.h=ourLoadMenu->newGame->ourImages[0].bitmap->h;
	ourLoadMenu->lNewGame.w=ourLoadMenu->newGame->ourImages[0].bitmap->w;
	ourLoadMenu->lNewGame.x=545;
	ourLoadMenu->lNewGame.y=4;
	ourLoadMenu->fNewGame=&CPreGame::showScenSel;
	//multiplayer
	ourLoadMenu->lLoadGame.h=ourLoadMenu->loadGame->ourImages[0].bitmap->h;
	ourLoadMenu->lLoadGame.w=ourLoadMenu->loadGame->ourImages[0].bitmap->w;
	ourLoadMenu->lLoadGame.x=568;
	ourLoadMenu->lLoadGame.y=120;
	ourLoadMenu->fLoadGame = NULL;
	//campaign
	ourLoadMenu->lHighScores.h=ourLoadMenu->highScores->ourImages[0].bitmap->h;
	ourLoadMenu->lHighScores.w=ourLoadMenu->highScores->ourImages[0].bitmap->w;
	ourLoadMenu->lHighScores.x=541;
	ourLoadMenu->lHighScores.y=233;
	ourLoadMenu->fHighScores = NULL;
	//tutorial
	ourLoadMenu->lCredits.h=ourLoadMenu->credits->ourImages[0].bitmap->h;
	ourLoadMenu->lCredits.w=ourLoadMenu->credits->ourImages[0].bitmap->w;
	ourLoadMenu->lCredits.x=545;
	ourLoadMenu->lCredits.y=358;
	ourLoadMenu->fCredits = NULL;
	//back
	ourLoadMenu->lQuit.h=ourLoadMenu->quit->ourImages[0].bitmap->h;
	ourLoadMenu->lQuit.w=ourLoadMenu->quit->ourImages[0].bitmap->w;
	ourLoadMenu->lQuit.x=582;
	ourLoadMenu->lQuit.y=464;
	ourLoadMenu->fQuit=&CPreGame::showMainMenu;

	ourLoadMenu->highlighted=0;
}

CPreGame::~CPreGame()
{
	delete ourMainMenu;
	delete ourNewMenu;
	delete ourLoadMenu;

	delete ok;
	delete cancel;
}

void CPreGame::begin()
{
	if(!ret.mapname.size()) //empty mapname (savename) - do nothing (no map/game selected)
		return;
	run = false;
	ret.difficulty=ourScenSel->selectedDiff;
}

CPreGame::menuItems::menuItems()
{
}

CPreGame::menuItems::~menuItems()
{
	delete this->newGame;
	delete this->loadGame;
	delete this->highScores;
	delete this->credits;
	delete this->quit;
	SDL_FreeSurface(bgAd);
	SDL_FreeSurface(background);
}

ScenSel::ScenSel()
:
	difficulty(new CPoinGroup()),
	bScens(genRect(0,0,414,81),boost::bind(&CPreGame::showScenList,CPG),CDefHandler::giveDef("GSPBUTT.DEF")),
	bOptions(genRect(0,0,414,509),boost::bind(&CPreGame::showOptions,CPG),CDefHandler::giveDef("GSPBUTT.DEF")),
	bRandom(genRect(0,0,414,105),boost::bind(&CPreGame::showScenList,CPG),CDefHandler::giveDef("GSPBUTT.DEF")),
	bBegin(genRect(0,0,414,535),boost::bind(&CPreGame::begin,CPG),CDefHandler::giveDef("SCNRBEG.DEF")),
	bLoad(genRect(0,0,414,535),boost::bind(&CPreGame::begin,CPG),CDefHandler::giveDef("SCNRLOD.DEF")),
	bBack(genRect(0,0,584,535),boost::bind(&CPreGame::showNewMenu,CPG),CDefHandler::giveDef("SCNRBACK.DEF")),
	bEasy(genRect(0,0,506,456),NULL,CDefHandler::giveDef("GSPBUT3.DEF"),true,difficulty,0),
	bNormal(genRect(0,0,538,456),NULL,CDefHandler::giveDef("GSPBUT4.DEF"),true,difficulty,1),
	bHard(genRect(0,0,570,456),NULL,CDefHandler::giveDef("GSPBUT5.DEF"),true,difficulty,2),
	bExpert(genRect(0,0,602,456),NULL,CDefHandler::giveDef("GSPBUT6.DEF"),true,difficulty,3),
	bImpossible(genRect(0,0,634,456),NULL,CDefHandler::giveDef("GSPBUT7.DEF"),true,difficulty,4)
{
	pressed=NULL;
	listShowed=false;
	if (rand()%2) 
		background = BitmapHandler::loadBitmap("ZPIC1000.bmp");
	else 
		background = BitmapHandler::loadBitmap("ZPIC1001.bmp");

	savenameStrip = BitmapHandler::loadBitmap("GSSTRIP.bmp");
	scenInf = BitmapHandler::loadBitmap("GSELPOP1.bmp");
	randMap = BitmapHandler::loadBitmap("RANMAPBK.bmp");
	options = BitmapHandler::loadBitmap("ADVOPTBK.bmp");
	SDL_SetColorKey(scenInf,SDL_SRCCOLORKEY,SDL_MapRGB(scenInf->format,0,255,255));
	SDL_SetColorKey(randMap,SDL_SRCCOLORKEY,SDL_MapRGB(randMap->format,0,255,255));
	SDL_SetColorKey(options,SDL_SRCCOLORKEY,SDL_MapRGB(options->format,0,255,255));

	difficulty->type=1;
	selectedDiff=-77;
	difficulty->gdzie = &selectedDiff;

	for (size_t i=0; i < bScens.imgs->ourImages.size(); ++i) {
		CSDL_Ext::printAt(
            CGI->generaltexth->allTexts[500],
                25+i,
                2+i,
                GEOR13,
                zwykly,
                bScens.imgs->ourImages[i].bitmap
        ); //"Show Available Scenarios"
    }

    for (size_t i=0; i < bRandom.imgs->ourImages.size(); ++i) {
		CSDL_Ext::printAt(
            CGI->generaltexth->allTexts[740],
            25+i,
            2+i,
            GEOR13,
            zwykly,
            bRandom.imgs->ourImages[i].bitmap
        );
    }
	
    for (size_t i=0; i < bOptions.imgs->ourImages.size(); ++i) {
		CSDL_Ext::printAt(
            CGI->generaltexth->allTexts[501],
            25+i,
            2+i,
            GEOR13,
            zwykly,
            bOptions.imgs->ourImages[i].bitmap
        ); //"Show Advanced Options"
    }
}

ScenSel::~ScenSel()
{
	delete difficulty;
	SDL_FreeSurface(scenInf);
	SDL_FreeSurface(randMap);
	SDL_FreeSurface(background);
	SDL_FreeSurface(options);
}
