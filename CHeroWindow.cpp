#include "stdafx.h"
#include "global.h"
#include "CHeroWindow.h"
#include "CGameInfo.h"
#include "hch\CHeroHandler.h"
#include "hch\CGeneralTextHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include <sstream>

extern SDL_Surface * ekran;

CHeroWindow::CHeroWindow(int playerColor)
{
	player = playerColor;
	background = CGI->bitmaph->loadBitmap("HEROSCR4.bmp");
	CSDL_Ext::blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;

	quitButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::quit, 674, 524, "hsbtns.def", this);
	dismissButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::dismissCurrent, 519, 437, "hsbtns2.def", this);
	questlogButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::questlog, 379, 437, "hsbtns4.def", this);

	gar1button = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::gar1, 546, 491, "hsbtns6.def", this);
	gar2button = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::gar2, 604, 491, "hsbtns8.def", this);
	gar3button = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::gar3, 546, 527, "hsbtns7.def", this);
	gar4button = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::gar4, 604, 527, "hsbtns9.def", this);

	leftArtRoll = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::leftArtRoller, 379, 364, "hsbtns3.def", this);
	rightArtRoll = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::rightArtRoller, 632, 364, "hsbtns5.def", this);

	for(int g=0; g<8; ++g)
	{
		heroList.push_back(new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::switchHero, 677, 95+g*54, "hsbtns5.def", this));
	}

	skillpics = CGI->spriteh->giveDef("pskil42.def");
	flags = CGI->spriteh->giveDef("CREST58.DEF");
}

CHeroWindow::~CHeroWindow()
{
	SDL_FreeSurface(background);
	delete quitButton;
	delete dismissButton;
	delete questlogButton;
	delete gar1button;
	delete gar2button;
	delete gar3button;
	delete gar4button;
	delete leftArtRoll;
	delete rightArtRoll;

	for(int g=0; g<heroList.size(); ++g)
		delete heroList[g];

	if(curBack)
		SDL_FreeSurface(curBack);

	delete skillpics;
	delete flags;
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(!to)
		to=ekran;
	blitAt(curBack,pos.x,pos.y,to);
	quitButton->show();
	dismissButton->show();
	questlogButton->show();
	gar1button->show();
	gar2button->show();
	gar3button->show();
	gar4button->show();
	leftArtRoll->show();
	rightArtRoll->show();
}

void CHeroWindow::setHero(const CGHeroInstance *hero)
{
	curHero = hero;
}

void CHeroWindow::quit()
{
	for(int i=0; i<LOCPLINT->objsToBlit.size(); ++i)
	{
		if( dynamic_cast<CHeroWindow*>( LOCPLINT->objsToBlit[i] ) )
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+i);
		}
	}
	quitButton->deactivate();
	dismissButton->deactivate();
	questlogButton->deactivate();
	gar1button->deactivate();
	gar2button->deactivate();
	gar3button->deactivate();
	gar4button->deactivate();
	leftArtRoll->deactivate();
	rightArtRoll->deactivate();
	for(int g=0; g<heroList.size(); ++g)
	{
		heroList[g]->deactivate();
	}

	LOCPLINT->adventureInt->show();

	SDL_FreeSurface(curBack);
	curBack = NULL;
}

void CHeroWindow::activate()
{
	quitButton->activate();
	dismissButton->activate();
	questlogButton->activate();
	gar1button->activate();
	gar2button->activate();
	gar3button->activate();
	gar4button->activate();
	leftArtRoll->activate();
	rightArtRoll->activate();
	for(int g=0; g<heroList.size(); ++g)
	{
		heroList[g]->activate();
	}

	curBack = CSDL_Ext::copySurface(background);
	blitAt(skillpics->ourImages[0].bitmap, 32, 111, curBack);
	blitAt(skillpics->ourImages[1].bitmap, 102, 111, curBack);
	blitAt(skillpics->ourImages[2].bitmap, 172, 111, curBack);
	blitAt(skillpics->ourImages[5].bitmap, 242, 111, curBack);
	blitAt(skillpics->ourImages[4].bitmap, 20, 230, curBack);
	blitAt(skillpics->ourImages[3].bitmap, 162, 230, curBack);

	blitAt(curHero->type->portraitLarge, 19, 19, curBack);

	CSDL_Ext::printAtMiddle(curHero->name, 190, 40, GEORXX, tytulowy, curBack);

	std::stringstream secondLine;
	secondLine<<"Level "<<curHero->level<<" "<<curHero->type->heroClass->name;
	CSDL_Ext::printAtMiddle(secondLine.str(), 190, 66, TNRB16, zwykly, curBack);

	//TODO: find missing texts and remove these ugly hardcoded names
	CSDL_Ext::printAtMiddle("Attack", 53, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle("Defence", 123, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle("Power", 193, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle("Knowledge", 263, 98, GEOR13, tytulowy, curBack);

	std::stringstream primarySkill1;
	primarySkill1<<curHero->primSkills[0];
	CSDL_Ext::printAtMiddle(primarySkill1.str(), 53, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill2;
	primarySkill2<<curHero->primSkills[1];
	CSDL_Ext::printAtMiddle(primarySkill2.str(), 123, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill3;
	primarySkill3<<curHero->primSkills[2];
	CSDL_Ext::printAtMiddle(primarySkill3.str(), 193, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill4;
	primarySkill4<<curHero->primSkills[3];
	CSDL_Ext::printAtMiddle(primarySkill4.str(), 263, 165, TNRB16, zwykly, curBack);

	blitAt(LOCPLINT->luck42->ourImages[curHero->getCurrentLuck()+3].bitmap, 239, 182, curBack);
	blitAt(LOCPLINT->morale42->ourImages[curHero->getCurrentMorale()+3].bitmap, 181, 182, curBack);

	blitAt(flags->ourImages[player].bitmap, 606, 8, curBack);

	//hero list blitting
	for(int g=0; g<LOCPLINT->cb->howManyHeroes(); ++g)
	{
		const CGHeroInstance * cur = LOCPLINT->cb->getHeroInfo(player, g, false);
		blitAt(cur->type->portraitSmall, 611, 87+g*54, curBack);
		//printing yellow border
		if(cur->name == curHero->name)
		{
			for(int f=0; f<cur->type->portraitSmall->w; ++f)
			{
				for(int h=0; h<cur->type->portraitSmall->h; ++h)
					if(f==0 || h==0 || f==cur->type->portraitSmall->w-1 || h==cur->type->portraitSmall->h-1)
					{
						CSDL_Ext::SDL_PutPixel(curBack, 611+f, 87+g*54+h, 240, 220, 120);
					}
			}
		}
	}
}

void CHeroWindow::dismissCurrent()
{
}

void CHeroWindow::questlog()
{
}

void CHeroWindow::gar1()
{
}

void CHeroWindow::gar2()
{
}

void CHeroWindow::gar3()
{
}

void CHeroWindow::gar4()
{
}

void CHeroWindow::leftArtRoller()
{
}

void CHeroWindow::rightArtRoller()
{
}

void CHeroWindow::switchHero()
{
	int y;
	SDL_GetMouseState(NULL, &y);
	for(int g=0; g<heroList.size(); ++g)
	{
		if(y>=94+54*g)
		{
			quit();
			setHero(LOCPLINT->cb->getHeroInfo(player, g, false));
			LOCPLINT->openHeroWindow(curHero);
		}
	}
}
