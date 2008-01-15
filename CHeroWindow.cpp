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
#include "CMessage.h"
#include <sstream>

extern SDL_Surface * ekran;
extern TTF_Font * GEOR16;

CHeroWindow::CHeroWindow(int playerColor)
{
	player = playerColor;
	background = CGI->bitmaph->loadBitmap("HEROSCR4.bmp");
	CSDL_Ext::blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;
	curBack = NULL;
	curHero = NULL;

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
	redrawCurBack();
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
			//quit();
			setHero(LOCPLINT->cb->getHeroInfo(player, g, false));
			//LOCPLINT->openHeroWindow(curHero);
			redrawCurBack();
		}
	}
}

void CHeroWindow::redrawCurBack()
{
	if(curBack)
		SDL_FreeSurface(curBack);
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

	//primary skliis names
	CSDL_Ext::printAtMiddle(CGI->generaltexth->jktexts[1], 53, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->jktexts[2], 123, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->jktexts[3], 193, 98, GEOR13, tytulowy, curBack);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->jktexts[4], 263, 98, GEOR13, tytulowy, curBack);

	//dismiss / quest log
	std::vector<std::string> * toPrin = CMessage::breakText(CGI->generaltexth->jktexts[8].substr(1, CGI->generaltexth->jktexts[8].size()-2));
	if(toPrin->size()==1)
	{
		CSDL_Ext::printAt((*toPrin)[0], 372, 440, GEOR13, zwykly, curBack);
	}
	else
	{
		CSDL_Ext::printAt((*toPrin)[0], 372, 431, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt((*toPrin)[1], 372, 447, GEOR13, zwykly, curBack);
	}
	delete toPrin;

	toPrin = CMessage::breakText(CGI->generaltexth->jktexts[9].substr(1, CGI->generaltexth->jktexts[9].size()-2));
	if(toPrin->size()==1)
	{
		CSDL_Ext::printAt((*toPrin)[0], 512, 440, GEOR13, zwykly, curBack);
	}
	else
	{
		CSDL_Ext::printAt((*toPrin)[0], 512, 431, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt((*toPrin)[1], 512, 447, GEOR13, zwykly, curBack);
	}
	delete toPrin;

	//printing primary skills' amounts
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

	//secondary skills
	if(curHero->secSkills.size()>=1)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[0].first*3+3+curHero->secSkills[0].second].bitmap, 18, 276, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[0].second], 69, 279, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[0].first]->name, 69, 299, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=2)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[1].first*3+3+curHero->secSkills[1].second].bitmap, 161, 276, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[1].second], 213, 279, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[1].first]->name, 213, 299, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=3)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[2].first*3+3+curHero->secSkills[2].second].bitmap, 18, 324, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[2].second], 69, 327, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[2].first]->name, 69, 347, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=4)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[3].first*3+3+curHero->secSkills[3].second].bitmap, 161, 324, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[3].second], 213, 327, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[3].first]->name, 213, 347, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=5)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[4].first*3+3+curHero->secSkills[4].second].bitmap, 18, 372, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[4].second], 69, 375, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[4].first]->name, 69, 395, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=6)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[5].first*3+3+curHero->secSkills[5].second].bitmap, 161, 372, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[5].second], 213, 375, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[5].first]->name, 213, 395, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=7)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[6].first*3+3+curHero->secSkills[6].second].bitmap, 18, 420, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[6].second], 69, 423, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[6].first]->name, 69, 443, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=8)
	{
		blitAt(CGI->abilh->abils44->ourImages[curHero->secSkills[7].first*3+3+curHero->secSkills[7].second].bitmap, 161, 420, curBack);
		CSDL_Ext::printAt(CGI->abilh->levels[curHero->secSkills[7].second], 213, 423, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->abilh->abilities[curHero->secSkills[7].first]->name, 213, 443, GEOR13, zwykly, curBack);
	}

	//printing special ability
	blitAt(CGI->heroh->un44->ourImages[curHero->subID].bitmap, 18, 180, curBack);

	//printing necessery texts
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[6].substr(1, CGI->generaltexth->jktexts[6].size()-2), 69, 231, GEOR13, tytulowy, curBack);
	std::stringstream expstr;
	expstr<<curHero->exp;
	CSDL_Ext::printAt(expstr.str(), 69, 247, GEOR16, zwykly, curBack);
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[7].substr(1, CGI->generaltexth->jktexts[7].size()-2), 212, 231, GEOR13, tytulowy, curBack);
	std::stringstream manastr;
	manastr<<curHero->mana<<'/'<<curHero->primSkills[3]*10;
	CSDL_Ext::printAt(manastr.str(), 212, 247, GEOR16, zwykly, curBack);
}