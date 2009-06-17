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
	backpackPos(0), player(playerColor)
{
	artWorn.resize(19);
	background = BitmapHandler::loadBitmap("HEROSCR4.bmp");
	graphics->blueToPlayersAdv(background, playerColor);
	pos.x = screen->w/2 - background->w/2 - 65;
	pos.y = screen->h/2 - background->h/2 - 8;
	pos.h = background->h;
	pos.w = background->w;
	curBack = NULL;
	curHero = NULL;
	activeArtPlace = NULL;

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
	leftArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CHeroWindow::scrollBackpack,this,-1), pos.x+379, pos.y+364, "hsbtns3.def", SDLK_LEFT);
	rightArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CHeroWindow::scrollBackpack,this,+1), pos.x+632, pos.y+364, "hsbtns5.def", SDLK_RIGHT);


	for(int g=0; g<8; ++g)
	{
		//heroList.push_back(new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::switchHero, 677, 95+g*54, "hsbtns5.def", this));
		heroListMi.push_back(new LClickableAreaHero());
		heroListMi[g]->pos.x = pos.x+677;
		heroListMi[g]->pos.y = pos.y  +  95+g*54;
		heroListMi[g]->pos.h = 32;
		heroListMi[g]->pos.w = 48;
		heroListMi[g]->owner = this;
		heroListMi[g]->id = g;
	}


	flags = CDefHandler::giveDef("CREST58.DEF");
	//areas
	portraitArea = new LRClickableAreaWText();
	portraitArea->pos.x = pos.x+83;
	portraitArea->pos.y = pos.y  +  26;
	portraitArea->pos.w = 58;
	portraitArea->pos.h = 64;
	for(int v=0; v<4; ++v)
	{
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[v]->pos.x = pos.x+95 + 70*v;
		primSkillAreas[v]->pos.y = pos.y  +  111;
		primSkillAreas[v]->pos.w = 42;
		primSkillAreas[v]->pos.h = 42;
		primSkillAreas[v]->text = CGI->generaltexth->arraytxt[2+v];
		primSkillAreas[v]->type = v;
		primSkillAreas[v]->bonus = -1; // to be initilized when hero is being set
		primSkillAreas[v]->baseType = 0;
	}
	expArea = new LRClickableAreaWText();
	expArea->pos.x = pos.x+83;
	expArea->pos.y = pos.y  +  236;
	expArea->pos.w = 136;
	expArea->pos.h = 42;
	expArea->hoverText = CGI->generaltexth->heroscrn[9];

	morale = new LRClickableAreaWTextComp();
	morale->pos = genRect(45,53,pos.x+240,pos.y+187);

	luck = new LRClickableAreaWTextComp();
	luck->pos = genRect(45,53,pos.x+298,pos.y+187);

	spellPointsArea = new LRClickableAreaWText();
	spellPointsArea->pos.x = pos.x+227;
	spellPointsArea->pos.y = pos.y  +  236;
	spellPointsArea->pos.w = 136;
	spellPointsArea->pos.h = 42;
	spellPointsArea->hoverText = CGI->generaltexth->heroscrn[22];

	for(int i=0; i<8; ++i)
	{
		secSkillAreas.push_back(new LRClickableAreaWTextComp());
		secSkillAreas[i]->pos.x = pos.x  +  ((i%2==0) ? (83) : (227));
		secSkillAreas[i]->pos.y = pos.y  +  (284 + 48 * (i/2));
		secSkillAreas[i]->pos.w = 136;
		secSkillAreas[i]->pos.h = 42;
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
	delete leftArtRoll;
	delete rightArtRoll;

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

	for(size_t g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
	}
	artWorn.clear();
	for(size_t g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
	}
	backpack.clear();

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
	leftArtRoll->show(to);
	rightArtRoll->show(to);

	garr->show(to);
	ourBar->show(to);

	for(size_t d=0; d<artWorn.size(); ++d)
	{
		artWorn[d]->show(to);
	}
	for(size_t d=0; d<backpack.size(); ++d)
	{
		backpack[d]->show(to);
	}
}

void CHeroWindow::setHero(const CGHeroInstance *Hero)
{
	char bufor[400];
	CGHeroInstance *hero = const_cast<CGHeroInstance*>(Hero); //but don't modify hero! - it's only for easy map reading
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
	/*gar4button->owner = */garr = new CGarrisonInt(pos.x+80, pos.y+493, 8, 0, curBack, 15, 485, curHero);
	garr->update = false;
	gar4button->callback =  boost::bind(&CGarrisonInt::splitClick,garr);//actualization of callback function

	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonus = hero->getPrimSkillLevel(g);
	}
	for(size_t g=0; g<hero->secSkills.size(); ++g)
	{
		secSkillAreas[g]->type = hero->secSkills[g].first;
		secSkillAreas[g]->bonus = hero->secSkills[g].second;
		std::string hlp = CGI->generaltexth->skillInfoTexts[ hero->secSkills[g].first ][hero->secSkills[g].second-1];
		secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);

		sprintf(bufor, CGI->generaltexth->heroscrn[21].c_str(), CGI->generaltexth->levels[hero->secSkills[g].second-1].c_str(), CGI->generaltexth->skillName[hero->secSkills[g].first].c_str());
		secSkillAreas[g]->hoverText = std::string(bufor);
	}

	sprintf(bufor, CGI->generaltexth->allTexts[2].c_str(), hero->level, CGI->heroh->reqExp(hero->level+1), hero->exp);
	expArea->text = std::string(bufor);

	sprintf(bufor, CGI->generaltexth->allTexts[205].c_str(), hero->name.c_str(), hero->mana, hero->manaLimit());
	spellPointsArea->text = std::string(bufor);

	for(size_t g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
	}
	for(size_t g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
	}
	backpack.clear();

	std::vector<SDL_Rect> slotPos;
	backpackPos = 0;

	slotPos += genRect(44,44,pos.x+509,pos.y+30), genRect(44,44,pos.x+567,pos.y+240), genRect(44,44,pos.x+509,pos.y+80), 
		genRect(44,44,pos.x+383,pos.y+68), genRect(44,44,pos.x+564,pos.y+183), genRect(44,44,pos.x+509,pos.y+130), 
		genRect(44,44,pos.x+431,pos.y+68), genRect(44,44,pos.x+610,pos.y+183), genRect(44,44,pos.x+515,pos.y+295), 
		genRect(44,44,pos.x+383,pos.y+143), genRect(44,44,pos.x+399,pos.y+194), genRect(44,44,pos.x+415,pos.y+245),
		genRect(44,44,pos.x+431,pos.y+296), genRect(44,44,pos.x+564,pos.y+30), genRect(44,44,pos.x+610,pos.y+30), 
		genRect(44,44,pos.x+610,pos.y+76), genRect(44,44,pos.x+610,pos.y+122), genRect(44,44,pos.x+610,pos.y+310),	
		genRect(44,44,pos.x+381,pos.y+296);

	for (int g = 0; g < 19 ; g++)
	{	
		artWorn[g] = new CArtPlace(hero->getArt(g));
		artWorn[g]->pos = slotPos[g];
		if(hero->getArt(g))
			artWorn[g]->text = hero->getArt(g)->Description();
		artWorn[g]->ourWindow = this;
	}

	for(size_t g=0; g<artWorn.size(); ++g)
	{
		artWorn[g]->slotID = g;
		if(artWorn[g]->ourArt)
		{
			sprintf(bufor, CGI->generaltexth->heroscrn[1].c_str(), artWorn[g]->ourArt->Name().c_str());
			artWorn[g]->hoverText = std::string(bufor);
		}
		else
		{
			artWorn[g]->hoverText = CGI->generaltexth->allTexts[507];
		}
	}

	for(size_t s=0; s<5; ++s)
	{
		CArtPlace * add;
		if( s < curHero->artifacts.size() )
		{
			add = new CArtPlace(&CGI->arth->artifacts[curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ]]);
			sprintf(bufor, CGI->generaltexth->heroscrn[1].c_str(), add->ourArt->Name().c_str());
			add->hoverText = bufor;
		}
		else
		{
			add = new CArtPlace(NULL);
			add->hoverText = CGI->generaltexth->allTexts[507];
		}
		add->pos.x = pos.x + 403 + 46*s;
		add->pos.y = pos.y + 365;
		add->pos.h = add->pos.w = 44;
		if(s<hero->artifacts.size() && hero->artifacts[s])
		{
			add->text = hero->getArt(19+s)->Description();
		}
		else
		{
			add->text = std::string();
		}
		add->ourWindow = this;
		add->slotID = 19+s;
		backpack.push_back(add);
	}
	activeArtPlace = NULL;
	dismissButton->block(!!hero->visitedTown);
	leftArtRoll->block(hero->artifacts.size()<6);
	rightArtRoll->block(hero->artifacts.size()<6);
	if(hero->getSecSkillLevel(19)==0)
		gar2button->block(true);
	else
	{
		gar2button->block(false);
		gar2button->callback = vstd::assigno(hero->tacticFormationEnabled,true);
		gar2button->callback2 = vstd::assigno(hero->tacticFormationEnabled,false);
	}

	formations->onChange = 0;
	formations->select(hero->army.formation,true);
	formations->onChange = boost::bind(&CCallback::setFormation, LOCPLINT->cb, Hero, _1);

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
	leftArtRoll->activate();
	rightArtRoll->activate();
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

	for(size_t f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->activate();
	}
	for(size_t f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->activate();
	}
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
	leftArtRoll->deactivate();
	rightArtRoll->deactivate();
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

	for(size_t f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->deactivate();
	}
	for(size_t f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->deactivate();
	}
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

void CHeroWindow::scrollBackpack(int dir)
{
	backpackPos += dir + curHero->artifacts.size();
	backpackPos %= curHero->artifacts.size();


	for(size_t s=0; s<5 && s<curHero->artifacts.size(); ++s) //set new data
	{
		CArtPlace *cur = backpack[s];
		cur->slotID = 19+((s+backpackPos)%curHero->artifacts.size());
		cur->ourArt = curHero->getArt(cur->slotID);

		if(cur->ourArt)
			cur->text = cur->ourArt->Description();
		else
			cur->text = std::string();
	}
}

void CHeroWindow::redrawCurBack()
{
	if(curBack)
		SDL_FreeSurface(curBack);
	curBack = SDL_DisplayFormat(background);

	//primary skills & morale and luck graphics
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
	std::stringstream secondLine;
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
	std::stringstream primarySkill1;
	primarySkill1<<curHero->getPrimSkillLevel(0);
	CSDL_Ext::printAtMiddle(primarySkill1.str(), 53, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill2;
	primarySkill2<<curHero->getPrimSkillLevel(1);
	CSDL_Ext::printAtMiddle(primarySkill2.str(), 123, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill3;
	primarySkill3<<curHero->getPrimSkillLevel(2);
	CSDL_Ext::printAtMiddle(primarySkill3.str(), 193, 165, TNRB16, zwykly, curBack);

	std::stringstream primarySkill4;
	primarySkill4<<curHero->getPrimSkillLevel(3);
	CSDL_Ext::printAtMiddle(primarySkill4.str(), 263, 165, TNRB16, zwykly, curBack);

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
	if(curHero->secSkills.size()>=1)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[0].first*3+3+curHero->secSkills[0].second-1].bitmap, 18, 276, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[0].second-1], 69, 279, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[0].first], 69, 299, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=2)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[1].first*3+3+curHero->secSkills[1].second-1].bitmap, 161, 276, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[1].second-1], 213, 279, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[1].first], 213, 299, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=3)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[2].first*3+3+curHero->secSkills[2].second-1].bitmap, 18, 324, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[2].second-1], 69, 327, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[2].first], 69, 347, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=4)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[3].first*3+3+curHero->secSkills[3].second-1].bitmap, 161, 324, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[3].second-1], 213, 327, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[3].first], 213, 347, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=5)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[4].first*3+3+curHero->secSkills[4].second-1].bitmap, 18, 372, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[4].second-1], 69, 375, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[4].first], 69, 395, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=6)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[5].first*3+3+curHero->secSkills[5].second-1].bitmap, 161, 372, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[5].second-1], 213, 375, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[5].first], 213, 395, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=7)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[6].first*3+3+curHero->secSkills[6].second-1].bitmap, 18, 420, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[6].second-1], 69, 423, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[6].first], 69, 443, GEOR13, zwykly, curBack);
	}
	if(curHero->secSkills.size()>=8)
	{
		blitAt(graphics->abils44->ourImages[curHero->secSkills[7].first*3+3+curHero->secSkills[7].second-1].bitmap, 161, 420, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->levels[curHero->secSkills[7].second-1], 213, 423, GEOR13, zwykly, curBack);
		CSDL_Ext::printAt(CGI->generaltexth->skillName[curHero->secSkills[7].first], 213, 443, GEOR13, zwykly, curBack);
	}

	//printing special ability
	blitAt(graphics->un44->ourImages[curHero->subID].bitmap, 18, 180, curBack);

	//printing necessery texts
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[6].substr(1, CGI->generaltexth->jktexts[6].size()-2), 69, 231, GEOR13, tytulowy, curBack);
	std::stringstream expstr;
	expstr<<curHero->exp;
	CSDL_Ext::printAt(expstr.str(), 69, 247, GEOR16, zwykly, curBack);
	CSDL_Ext::printAt(CGI->generaltexth->jktexts[7].substr(1, CGI->generaltexth->jktexts[7].size()-2), 212, 231, GEOR13, tytulowy, curBack);
	std::stringstream manastr;
	manastr << curHero->mana << '/' << curHero->manaLimit();
	CSDL_Ext::printAt(manastr.str(), 212, 247, GEOR16, zwykly, curBack);
}

void CHeroWindow::dispose()
{
	SDL_FreeSurface(curBack);
	curBack = NULL;
	curHero = NULL;

	for(size_t g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
		artWorn[g] = NULL;
	}
	for(size_t g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
		backpack[g] = NULL;
	}
	backpack.clear();
	activeArtPlace = NULL;
}

CArtPlace::CArtPlace(const CArtifact* Art): active(false), clicked(false), ourArt(Art)/*,
	spellBook(false), warMachine1(false), warMachine2(false), warMachine3(false),
	warMachine4(false),misc1(false), misc2(false), misc3(false), misc4(false),
	misc5(false), feet(false), lRing(false), rRing(false), torso(false),
	lHand(false), rHand(false), neck(false), shoulders(false), head(false) */{}
void CArtPlace::activate()
{
	if(!active)
	{
		//ClickableL::activate();
		LRClickableAreaWTextComp::activate();
		active = true;
	}
}
void CArtPlace::clickLeft(boost::logic::tribool down)
{
	//LRClickableAreaWTextComp::clickLeft(down);
	
	if(ourArt && !down) //we are spellbook
	{
		if(ourArt->id == 0)
		{
			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), ourWindow->curHero);
			LOCPLINT->pushInt(spellWindow);
		}
	}
	if(!down && !clicked && pressedL) //not clicked before
	{
		if(ourArt && ourArt->id == 0)
			return; //this is handled separately
		if(!ourWindow->activeArtPlace) //nothing has bewn clicked
		{
			if(ourArt) //to prevent selecting empty slots (bugfix to what GrayFace reported)
			{
				clicked = true;
				ourWindow->activeArtPlace = this;
			}
		}
		else //perform artifact substitution
		{
			if(slotID >= 19)	//we are an backpack slot - remove active artifact and put it to the last free pos in backpack
			{					//TODO: putting artifacts in the middle of backpack (pushing following arts)
				
				LOCPLINT->cb->swapArtifacts(ourWindow->curHero,ourWindow->activeArtPlace->slotID,ourWindow->curHero,ourWindow->curHero->artifacts.size()+19);
			}
			//check if swap is possible
			else if(this->fitsHere(ourWindow->activeArtPlace->ourArt) && ourWindow->activeArtPlace->fitsHere(this->ourArt))
			{
				int destSlot = slotID,
					srcSlot = ourWindow->activeArtPlace->slotID;

				LOCPLINT->cb->swapArtifacts(ourWindow->curHero,destSlot,ourWindow->curHero,srcSlot);

				ourWindow->activeArtPlace->clicked = false;
				ourWindow->activeArtPlace = NULL;
			}
		}
	}
	else if(!down && clicked)
	{
		if(ourArt && ourArt->id == 0)
			return; //this is handled separately
		clicked = false;
		ourWindow->activeArtPlace = NULL;
	}
	ClickableL::clickLeft(down);
}
void CArtPlace::clickRight(boost::logic::tribool down)
{
	if(text.size()) //if there is no description, do nothing ;]
		LRClickableAreaWTextComp::clickRight(down);
}
void CArtPlace::deactivate()
{
	if(active)
	{
		active = false;
		//ClickableL::deactivate();
		LRClickableAreaWTextComp::deactivate();
	}
}
void CArtPlace::show(SDL_Surface *to)
{
	if(ourArt)
	{
		blitAt(graphics->artDefs->ourImages[ourArt->id].bitmap, pos.x, pos.y, to);
	}
	if(clicked && active)
	{
		for(int i=0; i<pos.h; ++i)
		{
			for(int j=0; j<pos.w; ++j)
			{
				if(i==0 || j==0 || i==pos.h-1 || j==pos.w-1)
				{
					CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x+j, pos.y+i, 240, 220, 120);
				}
			}
		}
	}
}
bool CArtPlace::fitsHere(const CArtifact * art)
{
	if(!art)
		return true; //you can have no artifact somewhere
	if(slotID > 18   ||   vstd::contains(art->possibleSlots,slotID)) //backpack or right slot
		return true;
	return false;
}
CArtPlace::~CArtPlace()
{
	deactivate();
}

void LClickableArea::activate()
{
	ClickableL::activate();
}
void LClickableArea::deactivate()
{
	ClickableL::deactivate();
}
void LClickableArea::clickLeft(boost::logic::tribool down)
{
	//if(!down)
	//{
	//	LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	//}
}

void RClickableArea::activate()
{
	ClickableR::activate();
}
void RClickableArea::deactivate()
{
	ClickableR::deactivate();
}
void RClickableArea::clickRight(boost::logic::tribool down)
{
	//if(!down)
	//{
	//	LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	//}
}

void LRClickableAreaWText::clickLeft(boost::logic::tribool down)
{
	if(!down && pressedL)
	{
		LOCPLINT->showInfoDialog(text, std::vector<SComponent*>(), soundBase::sound_todo);
	}
	ClickableL::clickLeft(down);
}
void LRClickableAreaWText::clickRight(boost::logic::tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(text, down, this);
}
void LRClickableAreaWText::activate()
{
	LClickableArea::activate();
	RClickableArea::activate();
	Hoverable::activate();
}
void LRClickableAreaWText::deactivate()
{
	LClickableArea::deactivate();
	RClickableArea::deactivate();
	Hoverable::deactivate();
}
void LRClickableAreaWText::hover(bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->statusbar->print(hoverText);
	else if (LOCPLINT->statusbar->getCurrent()==hoverText)
		LOCPLINT->statusbar->clear();
}

void LClickableAreaHero::clickLeft(boost::logic::tribool down)
{
	if(!down)
	{
		owner->deactivate();
		const CGHeroInstance * buf = LOCPLINT->getWHero(id);
		owner->setHero(buf);
		owner->redrawCurBack();
		owner->activate();
	}
}

void LRClickableAreaWTextComp::clickLeft(boost::logic::tribool down)
{
	if((!down) && pressedL)
	{
		std::vector<SComponent*> comp(1, new SComponent(SComponent::Etype(baseType), type, bonus));
		LOCPLINT->showInfoDialog(text, comp, soundBase::sound_todo);
	}
	ClickableL::clickLeft(down);
}
void LRClickableAreaWTextComp::clickRight(boost::logic::tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(text, down, this);
}
void LRClickableAreaWTextComp::activate()
{
	LClickableArea::activate();
	RClickableArea::activate();
	Hoverable::activate();
}
void LRClickableAreaWTextComp::deactivate()
{
	LClickableArea::deactivate();
	RClickableArea::deactivate();
	Hoverable::deactivate();
}
void LRClickableAreaWTextComp::hover(bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->statusbar->print(hoverText);
	else if (LOCPLINT->statusbar->getCurrent()==hoverText)
		LOCPLINT->statusbar->clear();
}
