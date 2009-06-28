#include "../stdafx.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "Graphics.h"
#include "CSpellWindow.h"
#include "CConfigHandler.h"
#include "../global.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CObjectHandler.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/std/vector.hpp>
#include <cstdlib>
#include <sstream>

/*
 * CHeroWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
extern TTF_Font * GEOR16;
using namespace boost::assign;
CHeroWindow::CHeroWindow(int playerColor):
	player(playerColor)
{
	background = BitmapHandler::loadBitmap("HEROSCR4.bmp");
	graphics->blueToPlayersAdv(background, playerColor);
	pos.x = screen->w/2 - background->w/2 - 65;
	pos.y = screen->h/2 - background->h/2 - 8;
	pos.h = background->h;
	pos.w = background->w;
	curBack = NULL;
	curHero = NULL;

	artifs = new CArtifactsOfHero(pos);

	garr = NULL;
	ourBar = new CStatusBar(pos.x+72, pos.y+567, "ADROLLVR.bmp", 660);

	quitButton = new AdventureMapButton(CGI->generaltexth->heroscrn[17], std::string(), boost::function<void()>(), pos.x+674, pos.y+524, "hsbtns.def", SDLK_RETURN);
	dismissButton = new AdventureMapButton(std::string(), CGI->generaltexth->heroscrn[28], boost::bind(&CHeroWindow::dismissCurrent,this), pos.x+519, pos.y+437, "hsbtns2.def", SDLK_d);
	questlogButton = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CHeroWindow::questlog,this), pos.x+379, pos.y+437, "hsbtns4.def", SDLK_q);

	formations = new CHighlightableButtonsGroup(0);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[23]),CGI->generaltexth->heroscrn[29], "hsbtns6.def", pos.x+546, pos.y+491, 0, 0, SDLK_t);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[24]),CGI->generaltexth->heroscrn[30], "hsbtns7.def", pos.x+546, pos.y+527, 1, 0, SDLK_l);


	gar2button = new CHighlightableButton(0, 0, map_list_of(0,CGI->generaltexth->heroscrn[26])(3,CGI->generaltexth->heroscrn[25]), CGI->generaltexth->heroscrn[31], false, "hsbtns8.def", NULL, pos.x+604, pos.y+491, SDLK_b);
	gar4button = new AdventureMapButton(CGI->generaltexth->allTexts[256], CGI->generaltexth->heroscrn[32], boost::function<void()>(), pos.x+604, pos.y+527, "hsbtns9.def", false, NULL, false);
	boost::algorithm::replace_first(gar4button->hoverTexts[0],"%s",CGI->generaltexth->allTexts[43]);


	//right list of heroes
	for(int g=0; g<8; ++g)
	{
		//heroList.push_back(new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::switchHero, 677, 95+g*54, "hsbtns5.def", this));
		heroListMi.push_back(new LClickableAreaHero());
		heroListMi[g]->pos = genRect(32, 48, pos.x+677, pos.y  +  95+g*54);
		heroListMi[g]->owner = this;
		heroListMi[g]->id = g;
	}


	flags = CDefHandler::giveDefEss("CREST58.DEF");
	//areas
	portraitArea = new LRClickableAreaWText();
	portraitArea->pos = genRect(64, 58, pos.x+83, pos.y  +  26);

	for(int v=0; v<4; ++v)
	{
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[v]->pos = genRect(42, 42, pos.x+95 + 70*v, pos.y  +  111);
		primSkillAreas[v]->text = CGI->generaltexth->arraytxt[2+v];
		primSkillAreas[v]->type = v;
		primSkillAreas[v]->bonus = -1; // to be initilized when hero is being set
		primSkillAreas[v]->baseType = 0;
	}
	expArea = new LRClickableAreaWText();
	expArea->pos = genRect(42, 136, pos.x+83, pos.y  +  236);
	expArea->hoverText = CGI->generaltexth->heroscrn[9];

	morale = new LRClickableAreaWTextComp();
	morale->pos = genRect(45,53,pos.x+240,pos.y+187);

	luck = new LRClickableAreaWTextComp();
	luck->pos = genRect(45,53,pos.x+298,pos.y+187);

	spellPointsArea = new LRClickableAreaWText();
	spellPointsArea->pos = genRect(42, 136, pos.x+227, pos.y  +  236);
	spellPointsArea->hoverText = CGI->generaltexth->heroscrn[22];

	for(int i=0; i<8; ++i)
	{
		secSkillAreas.push_back(new LRClickableAreaWTextComp());
		secSkillAreas[i]->pos = genRect(42, 136, pos.x  +  ((i%2==0) ? (83) : (227)), pos.y  +  (284 + 48 * (i/2)));
		secSkillAreas[i]->baseType = 1;
	}
	pos.x += 65;
	pos.y += 8;
}

CHeroWindow::~CHeroWindow()
{
	SDL_FreeSurface(background);
	delete quitButton;
	delete dismissButton;
	delete questlogButton;
	delete formations;
	delete gar2button;
	delete gar4button;

	for(size_t g=0; g<heroListMi.size(); ++g)
	{
		delete heroListMi[g];
	}

	if(curBack)
	{
		SDL_FreeSurface(curBack);
	}

	delete flags;

	delete garr;
	delete ourBar;

	delete artifs;

	delete portraitArea;
	delete expArea;
	delete luck;
	delete morale;
	delete spellPointsArea;
	for(size_t v=0; v<primSkillAreas.size(); ++v)
	{
		delete primSkillAreas[v];
	}
	for(size_t v=0; v<secSkillAreas.size(); ++v)
	{
		delete secSkillAreas[v];
	}
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(curBack)
		blitAt(curBack,pos.x,pos.y,to);
	quitButton->show(to);
	dismissButton->show(to);
	questlogButton->show(to);
	formations->show(to);
	gar2button->show(to);
	gar4button->show(to);

	garr->show(to);
	ourBar->show(to);

	artifs->show(to);
}

void CHeroWindow::setHero(const CGHeroInstance *hero)
{
	char bufor[400];
	//CGHeroInstance *hero = const_cast<CGHeroInstance*>(Hero); //but don't modify hero! - it's only for easy map reading
	if(!hero) //something strange... no hero? it shouldn't happen
	{
		return;
	}
	curHero = hero;

	//pos temporarily switched, restored later
	pos.x -= 65;
	pos.y -= 8;

	gar2button->callback.clear();
	gar2button->callback2.clear();

	sprintf(bufor, CGI->generaltexth->heroscrn[16].c_str(), curHero->name.c_str(), curHero->type->heroClass->name.c_str());
	dismissButton->hoverTexts[0] = std::string(bufor);

	sprintf(bufor, CGI->generaltexth->allTexts[15].c_str(), curHero->name.c_str(), curHero->type->heroClass->name.c_str());
	portraitArea->hoverText = std::string(bufor);

	portraitArea->text = hero->getBiography();

	delete garr;
	/*gar4button->owner = */garr = new CGarrisonInt(pos.x+80, pos.y+493, 8, Point(), curBack, Point(65,8), curHero);
	garr->update = false;
	gar4button->callback =  boost::bind(&CGarrisonInt::splitClick,garr);//actualization of callback function

	//primary skills support
	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonus = hero->getPrimSkillLevel(g);
	}
	//secondary skills support
	for(size_t g=0; g<hero->secSkills.size(); ++g)
	{
		secSkillAreas[g]->type = hero->secSkills[g].first;
		secSkillAreas[g]->bonus = hero->secSkills[g].second;
		std::string hlp = CGI->generaltexth->skillInfoTexts[ hero->secSkills[g].first ][hero->secSkills[g].second-1];
		secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);

		sprintf(bufor, CGI->generaltexth->heroscrn[21].c_str(), CGI->generaltexth->levels[hero->secSkills[g].second-1].c_str(), CGI->generaltexth->skillName[hero->secSkills[g].first].c_str());
		secSkillAreas[g]->hoverText = std::string(bufor);
	}

	//printing experience
	sprintf(bufor, CGI->generaltexth->allTexts[2].c_str(), hero->level, CGI->heroh->reqExp(hero->level+1), hero->exp);
	expArea->text = std::string(bufor);

	//printing spell points
	sprintf(bufor, CGI->generaltexth->allTexts[205].c_str(), hero->name.c_str(), hero->mana, hero->manaLimit());
	spellPointsArea->text = std::string(bufor);

	artifs->setHero(hero);

	dismissButton->block(!!hero->visitedTown);
	if(hero->getSecSkillLevel(19)==0)
		gar2button->block(true);
	else
	{
		gar2button->block(false);
		gar2button->callback = vstd::assigno(hero->tacticFormationEnabled,true);
		gar2button->callback2 = vstd::assigno(hero->tacticFormationEnabled,false);
	}

	//setting formations
	formations->onChange = 0;
	formations->select(hero->army.formation,true);
	formations->onChange = boost::bind(&CCallback::setFormation, LOCPLINT->cb, hero, _1);

	//setting morale
	std::vector<std::pair<int,std::string> > mrl = hero->getCurrentMoraleModifiers();
	int mrlv = hero->getCurrentMorale();
	int mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad morale, 0 - neutral, 1 - good
	morale->hoverText = CGI->generaltexth->heroscrn[4 - mrlt];
	morale->baseType = SComponent::morale;
	morale->bonus = mrlv;
	morale->text = CGI->generaltexth->arraytxt[88];
	boost::algorithm::replace_first(morale->text,"%s",CGI->generaltexth->arraytxt[86-mrlt]);
	for(int it=0; it < mrl.size(); it++)
		morale->text += mrl[it].second;

	//setting luck
	mrl = hero->getCurrentLuckModifiers();
	mrlv = hero->getCurrentLuck();
	mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad luck, 0 - neutral, 1 - good
	luck->hoverText = CGI->generaltexth->heroscrn[7 - mrlt];
	luck->baseType = SComponent::luck;
	luck->bonus = mrlv;
	luck->text = CGI->generaltexth->arraytxt[62];
	boost::algorithm::replace_first(luck->text,"%s",CGI->generaltexth->arraytxt[60-mrlt]);
	for(int it=0; it < mrl.size(); it++)
		luck->text += mrl[it].second;

	//restoring pos
	pos.x += 65;
	pos.y += 8;

	redrawCurBack();
}

void CHeroWindow::quit()
{
	LOCPLINT->popInt(this);
	dispose();
}

void CHeroWindow::activate()
{
	quitButton->activate();
	dismissButton->activate();
	questlogButton->activate();
	gar2button->activate();
	formations->activate();
	gar4button->activate();
	portraitArea->activate();
	expArea->activate();
	spellPointsArea->activate();
	morale->activate();
	luck->activate();

	garr->activate();
	LOCPLINT->statusbar = ourBar;

	for(size_t v=0; v<primSkillAreas.size(); ++v)
	{
		primSkillAreas[v]->activate();
	}
	for(size_t v=0; v<curHero->secSkills.size(); ++v)
	{
		secSkillAreas[v]->activate();
	}
	redrawCurBack();

	artifs->activate();
	
	for(size_t e=0; e<heroListMi.size(); ++e)
	{
		heroListMi[e]->activate();
	}
}

void CHeroWindow::deactivate()
{
	quitButton->deactivate();
	dismissButton->deactivate();
	questlogButton->deactivate();
	gar2button->deactivate();
	formations->deactivate();
	gar4button->deactivate();
	portraitArea->deactivate();
	expArea->deactivate();
	spellPointsArea->deactivate();
	morale->deactivate();
	luck->deactivate();

	garr->deactivate();

	for(size_t v=0; v<primSkillAreas.size(); ++v)
	{
		primSkillAreas[v]->deactivate();
	}
	for(size_t v=0; v<curHero->secSkills.size(); ++v)
	{
		secSkillAreas[v]->deactivate();
	}

	artifs->deactivate();
	
	for(size_t e=0; e<heroListMi.size(); ++e)
	{
		heroListMi[e]->deactivate();
	}
}

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = boost::bind(&CHeroWindow::quit,this);
	ony += boost::bind(&CCallback::dismissHero,LOCPLINT->cb,curHero);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22],std::vector<SComponent*>(), ony, 0, false);
}

void CHeroWindow::questlog()
{
}

void CHeroWindow::redrawCurBack()
{
	if(curBack)
		SDL_FreeSurface(curBack);
	curBack = SDL_DisplayFormat(background);

	//primary skills & exp and mana
	blitAt(graphics->pskillsm->ourImages[0].bitmap, 32, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[1].bitmap, 102, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[2].bitmap, 172, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[5].bitmap, 242, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[4].bitmap, 20, 230, curBack);
	blitAt(graphics->pskillsm->ourImages[3].bitmap, 162, 230, curBack);

	//blitting portrait
	blitAt(graphics->portraitLarge[curHero->portrait], 19, 19, curBack);

	//printing hero's name
	CSDL_Ext::printAtMiddle(curHero->name, 190, 40, GEORXX, tytulowy, curBack);

	//printing hero's level
	std::ostringstream secondLine;
	secondLine<<"Level "<<curHero->level<<" "<<curHero->type->heroClass->name;
	CSDL_Ext::printAtMiddle(secondLine.str(), 190, 66, TNRB16, zwykly, curBack);

	//primary skills names
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
	for(int m=0; m<4; ++m)
	{
		std::ostringstream primarySkill;
		primarySkill<<curHero->getPrimSkillLevel(m);
		CSDL_Ext::printAtMiddle(primarySkill.str(), 53 + 70 * m, 165, TNRB16, zwykly, curBack);
	}

	//morale and luck printing
	blitAt(graphics->luck42->ourImages[curHero->getCurrentLuck()+3].bitmap, 239, 182, curBack);
	blitAt(graphics->morale42->ourImages[curHero->getCurrentMorale()+3].bitmap, 181, 182, curBack);

	blitAt(flags->ourImages[player].bitmap, 606, 8, curBack);

	//hero list blitting

	for(int pos=0, g=0; g<LOCPLINT->wanderingHeroes.size(); ++g)
	{
		const CGHeroInstance * cur = LOCPLINT->wanderingHeroes[g];
		if (cur->inTownGarrison)
			// Only display heroes that are not in garrison
			continue;

		blitAt(graphics->portraitSmall[cur->portrait], 611, 87+pos*54, curBack);
		//printing yellow border
		if(cur->name == curHero->name)
		{
			for(int f=0; f<graphics->portraitSmall[cur->portrait]->w; ++f)
			{
				for(int h=0; h<graphics->portraitSmall[cur->portrait]->h; ++h)
					if(f==0 || h==0 || f==graphics->portraitSmall[cur->portrait]->w-1 || h==graphics->portraitSmall[cur->portrait]->h-1)
					{
						CSDL_Ext::SDL_PutPixelWithoutRefresh(curBack, 611+f, 87+pos*54+h, 240, 220, 120);
					}
			}
		}

		pos ++;
	}

	//secondary skills
	for(int g=1; g<=8; ++g)
	{
		
		if(curHero->secSkills.size()>=g)
		{
			blitAt(graphics->abils44->ourImages[curHero->secSkills[g-1].first*3+3+curHero->secSkills[g-1].second-1].bitmap, g%2 ? 18 : 161, 276 + 48 * ((g-1)/2), curBack);
			CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[g-1].second-1], g%2 ? 69 : 213, 279 + 48 * ((g-1)/2), GEOR13, zwykly, curBack);
			CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[g-1].first], g%2 ? 69 : 213, 299 + 48 * ((g-1)/2), GEOR13, zwykly, curBack);
		}
	}

	//printing special ability
	blitAt(graphics->un44->ourImages[curHero->subID].bitmap, 18, 180, curBack);

	//printing necessery texts
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[6].substr(1, CGI->generaltexth->jktexts[6].size()-2), 69, 231, GEOR13, tytulowy, curBack);
	std::ostringstream expstr;
	expstr<<curHero->exp;
	CSDL_Ext::printAt(expstr.str(), 69, 247, GEOR16, zwykly, curBack);
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[7].substr(1, CGI->generaltexth->jktexts[7].size()-2), 212, 231, GEOR13, tytulowy, curBack);
	std::ostringstream manastr;
	manastr << curHero->mana << '/' << curHero->manaLimit();
	CSDL_Ext::printAt(manastr.str(), 212, 247, GEOR16, zwykly, curBack);
}

void CHeroWindow::dispose()
{
	SDL_FreeSurface(curBack);
	curBack = NULL;
	curHero = NULL;

	artifs->dispose();
}

