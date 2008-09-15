#include "stdafx.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "CCallback.h"
#include "CCastleInterface.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "client/CBitmapHandler.h"
#include "client/Graphics.h"
#include "client/CSpellWindow.h"
#include "global.h"
#include "hch/CAbilityHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CDefHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CLodHandler.h"
#include "hch/CObjectHandler.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/std/vector.hpp>
#include <cstdlib>
#include <sstream>
extern SDL_Surface * screen;
extern TTF_Font * GEOR16;
using namespace boost::assign;
CHeroWindow::CHeroWindow(int playerColor):
	backpackPos(0), player(playerColor)
{
	artWorn.resize(19);
	background = BitmapHandler::loadBitmap("HEROSCR4.bmp");
	graphics->blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;
	curBack = NULL;
	curHero = NULL;
	activeArtPlace = NULL;

	garInt = NULL;
	ourBar = new CStatusBar(72, 567, "ADROLLVR.bmp", 660);

	quitButton = new AdventureMapButton(CGI->generaltexth->heroscrn[17], std::string(), boost::function<void()>(), 674, 524, "hsbtns.def", false, NULL, false);
	dismissButton = new AdventureMapButton(std::string(), CGI->generaltexth->heroscrn[28], boost::bind(&CHeroWindow::dismissCurrent,this), 519, 437, "hsbtns2.def", false, NULL, false);
	questlogButton = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CHeroWindow::questlog,this), 379, 437, "hsbtns4.def", false, NULL, false);

	gar1button = new AdventureMapButton(CGI->generaltexth->heroscrn[23], CGI->generaltexth->heroscrn[29], boost::bind(&CHeroWindow::gar1,this), 546, 491, "hsbtns6.def", false, NULL, false);
	gar3button = new AdventureMapButton(CGI->generaltexth->heroscrn[24], CGI->generaltexth->heroscrn[30], boost::bind(&CHeroWindow::gar3,this), 546, 527, "hsbtns7.def", false, NULL, false);
	gar2button = new CHighlightableButton(0, 0, map_list_of(0,CGI->generaltexth->heroscrn[26])(3,CGI->generaltexth->heroscrn[25]), CGI->generaltexth->heroscrn[31], false, "hsbtns8.def", NULL, 604, 491, false);
	gar4button = new AdventureMapButton(CGI->generaltexth->allTexts[256], CGI->generaltexth->heroscrn[32], boost::function<void()>(), 604, 527, "hsbtns9.def", false, NULL, false);
	boost::algorithm::replace_first(gar4button->hoverTexts[0],"%s",CGI->generaltexth->allTexts[43]);
	leftArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CHeroWindow::leftArtRoller,this), 379, 364, "hsbtns3.def", false, NULL, false);
	rightArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CHeroWindow::rightArtRoller,this), 632, 364, "hsbtns5.def", false, NULL, false);

	for(int g=0; g<8; ++g)
	{
		//heroList.push_back(new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::switchHero, 677, 95+g*54, "hsbtns5.def", this));
		heroListMi.push_back(new LClickableAreaHero());
		heroListMi[g]->pos.x = 677;
		heroListMi[g]->pos.y = 95+g*54;
		heroListMi[g]->pos.h = 32;
		heroListMi[g]->pos.w = 48;
		heroListMi[g]->owner = this;
		heroListMi[g]->id = g;
	}

	flags = CDefHandler::giveDef("CREST58.DEF");
	//areas
	portraitArea = new LRClickableAreaWText();
	portraitArea->pos.x = 83;
	portraitArea->pos.y = 26;
	portraitArea->pos.w = 58;
	portraitArea->pos.h = 64;
	for(int v=0; v<4; ++v)
	{
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[v]->pos.x = 95 + 70*v;
		primSkillAreas[v]->pos.y = 111;
		primSkillAreas[v]->pos.w = 42;
		primSkillAreas[v]->pos.h = 42;
		primSkillAreas[v]->text = CGI->generaltexth->arraytxt[2+v].substr(1, CGI->generaltexth->arraytxt[2+v].size()-2);
		primSkillAreas[v]->type = v;
		primSkillAreas[v]->bonus = -1; // to be initilized when hero is being set
		primSkillAreas[v]->baseType = 0;
	}
	expArea = new LRClickableAreaWText();
	expArea->pos.x = 83;
	expArea->pos.y = 236;
	expArea->pos.w = 136;
	expArea->pos.h = 42;
	expArea->hoverText = CGI->generaltexth->heroscrn[9];

	spellPointsArea = new LRClickableAreaWText();
	spellPointsArea->pos.x = 227;
	spellPointsArea->pos.y = 236;
	spellPointsArea->pos.w = 136;
	spellPointsArea->pos.h = 42;
	spellPointsArea->hoverText = CGI->generaltexth->heroscrn[22];

	for(int i=0; i<8; ++i)
	{
		secSkillAreas.push_back(new LRClickableAreaWTextComp());
		secSkillAreas[i]->pos.x = (i%2==0) ? (83) : (227);
		secSkillAreas[i]->pos.y = 284 + 48 * (i/2);
		secSkillAreas[i]->pos.w = 136;
		secSkillAreas[i]->pos.h = 42;
		secSkillAreas[i]->baseType = 1;
	}
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

	for(int g=0; g<heroListMi.size(); ++g)
		delete heroListMi[g];

	if(curBack)
		SDL_FreeSurface(curBack);

	delete flags;

	delete garInt;
	delete ourBar;

	for(int g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
	}
	artWorn.clear();
	for(int g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
	}
	backpack.clear();

	delete portraitArea;
	delete expArea;
	delete spellPointsArea;
	for(int v=0; v<primSkillAreas.size(); ++v)
	{
		delete primSkillAreas[v];
	}
	for(int v=0; v<secSkillAreas.size(); ++v)
	{
		delete secSkillAreas[v];
	}
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(!to)
		to=screen;
	if(curBack)
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

	garInt->show();
	ourBar->show();

	for(int d=0; d<artWorn.size(); ++d)
	{
		artWorn[d]->show(to);
	}
	for(int d=0; d<backpack.size(); ++d)
	{
		backpack[d]->show(to);
	}
}

void CHeroWindow::setHero(const CGHeroInstance *Hero)
{
	CGHeroInstance *hero = const_cast<CGHeroInstance*>(Hero); //but don't modify hero! - it's only for easy map reading
	if(!hero) //something strange... no hero? it shouldn't happen
	{
		return;
	}
	curHero = hero;

	gar2button->callback.clear();
	gar2button->callback2.clear();

	char * prhlp = new char[200];
	sprintf(prhlp, CGI->generaltexth->heroscrn[16].c_str(), curHero->name.c_str(), curHero->type->heroClass->name.c_str());
	dismissButton->hoverTexts[0] = std::string(prhlp);
	delete [] prhlp;

	prhlp = new char[200];
	sprintf(prhlp, CGI->generaltexth->allTexts[15].c_str(), curHero->name.c_str(), curHero->type->heroClass->name.c_str());
	portraitArea->hoverText = std::string(prhlp);
	delete [] prhlp;

	portraitArea->text = hero->biography;

	delete garInt;
	/*gar4button->owner = */garInt = new CGarrisonInt(80, 493, 8, 0, curBack, 13, 482, curHero);
	garInt->update = false;
	gar4button->callback =  boost::bind(&CGarrisonInt::splitClick,garInt);//actualization of callback function

	for(int g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonus = hero->primSkills[g];
	}
	for(int g=0; g<hero->secSkills.size(); ++g)
	{
		secSkillAreas[g]->type = hero->secSkills[g].first;
		secSkillAreas[g]->bonus = hero->secSkills[g].second;
		std::string hlp = CGI->abilh->abilities[ hero->secSkills[g].first ]->infoTexts[hero->secSkills[g].second-1];
		secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);

		char * hlpp = new char[200];
		sprintf(hlpp, CGI->generaltexth->heroscrn[21].c_str(), CGI->abilh->levels[hero->secSkills[g].second].c_str(), CGI->abilh->abilities[hero->secSkills[g].first]->name.c_str());
		secSkillAreas[g]->hoverText = std::string(hlpp);
		delete [] hlpp;
	}

	char * th = new char[200];
	sprintf(th, CGI->generaltexth->allTexts[2].substr(1, CGI->generaltexth->allTexts[2].size()-2).c_str(), hero->level, CGI->heroh->reqExp(hero->level+1), hero->exp);
	expArea->text = std::string(th);
	delete [] th;
	th = new char[400];
	sprintf(th, CGI->generaltexth->allTexts[205].substr(1, CGI->generaltexth->allTexts[205].size()-2).c_str(), hero->name.c_str(), hero->mana, hero->primSkills[3]*10);
	spellPointsArea->text = std::string(th);
	delete [] th;

	for(int g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
	}
	for(int g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
	}
	backpack.clear();

	std::vector<SDL_Rect> slotPos;

	slotPos += genRect(44,44,509,30), genRect(44,44,567,240), genRect(44,44,509,30), genRect(44,44,383,68),
		genRect(44,44,564,183), genRect(44,44,509,130), genRect(44,44,431,68), genRect(44,44,610,183),
		genRect(44,44,515,295), genRect(44,44,383,143), genRect(44,44,399,194), genRect(44,44,415,245),
		genRect(44,44,431,296), genRect(44,44,564,30), genRect(44,44,610,30), genRect(44,44,610,76),
		genRect(44,44,610,122), genRect(44,44,610,310),	genRect(44,44,381,296);

	for (int g = 0; g < 19 ; g++)
	{	
		artWorn[g] = new CArtPlace(hero->getArt(g));
		artWorn[g]->pos = slotPos[g];
		if(hero->getArt(g))
			artWorn[g]->text = hero->getArt(g)->description;
		artWorn[g]->ourWindow = this;
	}

	for(int g=0; g<artWorn.size(); ++g)
	{
		artWorn[g]->slotID = g;
		char * hll = new char[200];
		sprintf(hll, CGI->generaltexth->heroscrn[1].c_str(), (artWorn[g]->ourArt ? artWorn[g]->ourArt->name.c_str() : ""));
		artWorn[g]->hoverText = std::string(hll);
		delete [] hll;
	}

	for(int s=0; s<5; ++s)
	{
		CArtPlace * add;
		if( s < curHero->artifacts.size() )
			add = new CArtPlace(&CGI->arth->artifacts[curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ]]);
		else
			add = new CArtPlace(NULL);
		add->pos.x = 403 + 46*s;
		add->pos.y = 365;
		add->pos.h = add->pos.w = 44;
		if(s<hero->artifacts.size() && hero->artifacts[s])
			add->text = hero->getArt(19+s)->description;
		else
			add->text = std::string();
		add->ourWindow = this;
		add->slotID = 19+s;
		backpack.push_back(add);
	}
	activeArtPlace = NULL;
	dismissButton->block(hero->visitedTown);
	leftArtRoll->block(hero->artifacts.size()<6);
	rightArtRoll->block(hero->artifacts.size()<6);
	if(hero->getSecSkillLevel(19)<0)
		gar2button->block(true);
	else
	{
		gar2button->block(false);
		gar2button->callback = boost::bind(vstd::assign<bool,bool>,boost::ref(hero->tacticFormationEnabled), true);
		gar2button->callback2 = boost::bind(vstd::assign<bool,bool>,boost::ref(hero->tacticFormationEnabled), true);
	}
	redrawCurBack();
}

void CHeroWindow::quit()
{
	LOCPLINT->curint->subInt = NULL;
	LOCPLINT->objsToBlit -= this;

	if(LOCPLINT->curint == LOCPLINT->castleInt)
		LOCPLINT->castleInt->subInt = NULL;
	LOCPLINT->curint->activate();

	SDL_FreeSurface(curBack);
	curBack = NULL;

	for(int g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
		artWorn[g] = NULL;
	}
	for(int g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
		backpack[g] = NULL;
	}
	backpack.clear();
	activeArtPlace = NULL;
}

void CHeroWindow::activate()
{
	LOCPLINT->curint->subInt = this;
	quitButton->activate();
	dismissButton->activate();
	questlogButton->activate();
	gar1button->activate();
	gar2button->activate();
	gar3button->activate();
	gar4button->activate();
	leftArtRoll->activate();
	rightArtRoll->activate();
	portraitArea->activate();
	expArea->activate();
	spellPointsArea->activate();

	garInt->activate();
	LOCPLINT->statusbar = ourBar;

	for(int v=0; v<primSkillAreas.size(); ++v)
	{
		primSkillAreas[v]->activate();
	}
	for(int v=0; v<curHero->secSkills.size(); ++v)
	{
		secSkillAreas[v]->activate();
	}
	redrawCurBack();

	for(int f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->activate();
	}
	for(int f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->activate();
	}
	for(int e=0; e<heroListMi.size(); ++e)
	{
		heroListMi[e]->activate();
	}

	//LOCPLINT->lclickable.push_back(artFeet);
}

void CHeroWindow::deactivate()
{
	quitButton->deactivate();
	dismissButton->deactivate();
	questlogButton->deactivate();
	gar1button->deactivate();
	gar2button->deactivate();
	gar3button->deactivate();
	gar4button->deactivate();
	leftArtRoll->deactivate();
	rightArtRoll->deactivate();
	portraitArea->deactivate();
	expArea->deactivate();
	spellPointsArea->deactivate();

	garInt->deactivate();

	for(int v=0; v<primSkillAreas.size(); ++v)
	{
		primSkillAreas[v]->deactivate();
	}
	for(int v=0; v<curHero->secSkills.size(); ++v)
	{
		secSkillAreas[v]->deactivate();
	}

	for(int f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->deactivate();
	}
	for(int f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->deactivate();
	}
	for(int e=0; e<heroListMi.size(); ++e)
	{
		heroListMi[e]->deactivate();
	}
}

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = boost::bind(&CHeroWindow::quit,this);
	ony += boost::bind(&CCallback::dismissHero,LOCPLINT->cb,curHero);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22],std::vector<SComponent*>(), ony, boost::bind(&CHeroWindow::activate,this), true, false);
}

void CHeroWindow::questlog()
{
}

void CHeroWindow::gar1()
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
	if(curHero->artifacts.size()>5) //if it is <=5, we have nothing to scroll
	{
		backpackPos+=curHero->artifacts.size()-1; //set new offset

		for(int s=0; s<5 && s<curHero->artifacts.size(); ++s) //set new data
		{
			backpack[s]->ourArt = &CGI->arth->artifacts[curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ]];
			if(backpack[s]->ourArt)
				backpack[s]->text = backpack[s]->ourArt->description;
			else
				backpack[s]->text = std::string();
		}
	}
}

void CHeroWindow::rightArtRoller()
{
	if(curHero->artifacts.size()>5) //if it is <=5, we have nothing to scroll
	{
		backpackPos+=1; //set new offset

		for(int s=0; s<5 && s<curHero->artifacts.size(); ++s) //set new data
		{
			backpack[s]->ourArt = &CGI->arth->artifacts[curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ] ];
			if(backpack[s]->ourArt)
				backpack[s]->text = backpack[s]->ourArt->description;
			else
				backpack[s]->text = std::string();
		}
	}
}

void CHeroWindow::switchHero()
{
	//int y;
	//SDL_GetMouseState(NULL, &y);
	//for(int g=0; g<heroListMi.size(); ++g)
	//{
	//	if(y>=94+54*g)
	//	{
	//		//quit();
	//		setHero(LOCPLINT->cb->getHeroInfo(player, g, false));
	//		//LOCPLINT->openHeroWindow(curHero);
	//		redrawCurBack();
	//	}
	//}
}

void CHeroWindow::redrawCurBack()
{
	if(curBack)
		SDL_FreeSurface(curBack);
	curBack = SDL_DisplayFormat(background);

	blitAt(graphics->pskillsm->ourImages[0].bitmap, 32, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[1].bitmap, 102, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[2].bitmap, 172, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[5].bitmap, 242, 111, curBack);
	blitAt(graphics->pskillsm->ourImages[4].bitmap, 20, 230, curBack);
	blitAt(graphics->pskillsm->ourImages[3].bitmap, 162, 230, curBack);

	blitAt(graphics->portraitLarge[curHero->portrait], 19, 19, curBack);

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

	blitAt(graphics->luck42->ourImages[curHero->getCurrentLuck()+3].bitmap, 239, 182, curBack);
	blitAt(graphics->morale42->ourImages[curHero->getCurrentMorale()+3].bitmap, 181, 182, curBack);

	blitAt(flags->ourImages[player].bitmap, 606, 8, curBack);

	//hero list blitting
	for(int g=0; g<LOCPLINT->cb->howManyHeroes(); ++g)
	{
		const CGHeroInstance * cur = LOCPLINT->cb->getHeroInfo(g, false);
		blitAt(graphics->portraitSmall[cur->portrait], 611, 87+g*54, curBack);
		//printing yellow border
		if(cur->name == curHero->name)
		{
			for(int f=0; f<graphics->portraitSmall[cur->portrait]->w; ++f)
			{
				for(int h=0; h<graphics->portraitSmall[cur->portrait]->h; ++h)
					if(f==0 || h==0 || f==graphics->portraitSmall[cur->portrait]->w-1 || h==graphics->portraitSmall[cur->portrait]->h-1)
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
	blitAt(graphics->un44->ourImages[curHero->subID].bitmap, 18, 180, curBack);

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

CArtPlace::CArtPlace(const CArtifact* Art): ourArt(Art), active(false), clicked(false)/*,
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
			ourWindow->deactivate();

			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, 90, 2), ourWindow->curHero);
			spellWindow->activate();
			LOCPLINT->objsToBlit.push_back(spellWindow);
		}
	}
	if(!down && !clicked) //not clicked before
	{
		if(ourArt && ourArt->id == 0)
			return; //this is handled separately
		if(!ourWindow->activeArtPlace) //nothing has benn clicked
		{
			if(ourArt) //to prevent selecting empty slots (bugfix to what GrayFace reported)
			{
				clicked = true;
				ourWindow->activeArtPlace = this;
			}
		}
		else //perform artifact substitution
		{
			//chceck if swap is possible
			if(this->fitsHere(ourWindow->activeArtPlace->ourArt) && ourWindow->activeArtPlace->fitsHere(this->ourArt))
			{
				LOCPLINT->cb->swapArifacts(ourWindow->curHero,slotID,ourWindow->curHero,ourWindow->activeArtPlace->slotID);

				const CArtifact * pmh = ourArt;
				ourArt = ourWindow->activeArtPlace->ourArt;
				ourWindow->activeArtPlace->ourArt = pmh;

				//set texts
				if(pmh)
					ourWindow->activeArtPlace->text = pmh->description;
				else
					ourWindow->activeArtPlace->text = std::string();
				if(ourArt)
					text = ourArt->description;
				else
					text = std::string();

				ourWindow->activeArtPlace->clicked = false;
				ourWindow->activeArtPlace = NULL;
			}
			else
			{
				int backID = -1;
				for(int g=0; g<ourWindow->backpack.size(); ++g)
				{
					if(ourWindow->backpack[g]==this) //if user wants to put something to backpack
					{
						backID = g;
						break;
					}
				}
				if(backID>=0) //put to backpack
				{
					/*ourWindow->activeArtPlace->ourArt = NULL;
					ourWindow->activeArtPlace->clicked = false;
					ourWindow->activeArtPlace = NULL;*/
				}
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
					CSDL_Ext::SDL_PutPixel(to, pos.x+j, pos.y+i, 240, 220, 120);
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
	if(!down)
	{
		LOCPLINT->showInfoDialog(text, std::vector<SComponent*>());
	}
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
		const CGHeroInstance * buf = LOCPLINT->cb->getHeroInfo(id, false);
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
		LOCPLINT->showInfoDialog(text, comp);
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
