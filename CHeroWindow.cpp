#include "stdafx.h"
#include "global.h"
#include "CHeroWindow.h"
#include "CGameInfo.h"
#include "hch\CHeroHandler.h"
#include "hch\CGeneralTextHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "hch\CLodHandler.h"
#include "AdventureMapButton.h"
#include "hch\CObjectHandler.h"
#include "CMessage.h"
#include "CCallback.h"
#include <sstream>

extern SDL_Surface * screen;
extern TTF_Font * GEOR16;

CHeroWindow::CHeroWindow(int playerColor):
	backpackPos(0), player(playerColor)
{
	artWorn.resize(19);
	background = CGI->bitmaph->loadBitmap("HEROSCR4.bmp");
	CSDL_Ext::blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;
	curBack = NULL;
	curHero = NULL;
	activeArtPlace = NULL;

	garInt = NULL;
	ourBar = new CStatusBar(72, 567, "ADROLLVR.bmp", 660);

	quitButton = new AdventureMapButton(CGI->generaltexth->heroscrn[17], std::string(), boost::bind(&CHeroWindow::quit,this), 674, 524, "hsbtns.def", false, NULL, false);
	dismissButton = new AdventureMapButton(std::string(), CGI->generaltexth->heroscrn[28], boost::bind(&CHeroWindow::dismissCurrent,this), 519, 437, "hsbtns2.def", false, NULL, false);
	questlogButton = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CHeroWindow::questlog,this), 379, 437, "hsbtns4.def", false, NULL, false);

	gar1button = new AdventureMapButton(CGI->generaltexth->heroscrn[23], CGI->generaltexth->heroscrn[29], boost::bind(&CHeroWindow::gar1,this), 546, 491, "hsbtns6.def", false, NULL, false);
	gar2button = new AdventureMapButton(std::string(), std::string(), boost::bind(&CHeroWindow::gar2,this), 604, 491, "hsbtns8.def", false, NULL, false);
	gar3button = new AdventureMapButton(CGI->generaltexth->heroscrn[24], CGI->generaltexth->heroscrn[30], boost::bind(&CHeroWindow::gar3,this), 546, 527, "hsbtns7.def", false, NULL, false);
	gar4button = new AdventureMapButton(std::string(), CGI->generaltexth->heroscrn[32], boost::function<void()>(), 604, 527, "hsbtns9.def", false, NULL, false);

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

	skillpics = CGI->spriteh->giveDef("pskil42.def");
	flags = CGI->spriteh->giveDef("CREST58.DEF");
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

	delete skillpics;
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

void CHeroWindow::setHero(const CGHeroInstance *hero)
{
	if(!hero) //something strange... no hero? it shouldn't happen
	{
		return; 
	}
	curHero = hero;

	char * prhlp = new char[200];
	sprintf(prhlp, CGI->generaltexth->heroscrn[16].c_str(), curHero->name.c_str(), curHero->type->heroClass->name.c_str());
	dismissButton->name = std::string(prhlp);
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
		std::string hlp;
		switch(hero->secSkills[g].second)
		{
		case 0: //basic level
			hlp = CGI->abilh->abilities[ hero->secSkills[g].first ]->basicText;
			secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);
			break;
		case 1: //adv level
			hlp = CGI->abilh->abilities[ hero->secSkills[g].first ]->advText;
			secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);
			break;
		case 2: //expert level
			hlp = CGI->abilh->abilities[ hero->secSkills[g].first ]->expText;
			secSkillAreas[g]->text = hlp.substr(1, hlp.size()-2);
			break;
		}
		
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

	artWorn[8] = new CArtPlace(hero->artifWorn[8]);
	artWorn[8]->pos.x = 515;
	artWorn[8]->pos.y = 295;
	artWorn[8]->pos.h = artWorn[8]->pos.w = 44;
	if(hero->artifWorn[8])
		artWorn[8]->text = hero->artifWorn[8]->description;
	else
		artWorn[8]->text = std::string();
	artWorn[8]->ourWindow = this;
	artWorn[8]->feet = true;

	artWorn[0] = new CArtPlace(hero->artifWorn[0]);
	artWorn[0]->pos.x = 509;
	artWorn[0]->pos.y = 30;
	artWorn[0]->pos.h = artWorn[0]->pos.w = 44;
	if(hero->artifWorn[0])
		artWorn[0]->text = hero->artifWorn[0]->description;
	else
		artWorn[0]->text = std::string();
	artWorn[0]->ourWindow = this;
	artWorn[0]->head = true;

	artWorn[4] = new CArtPlace(hero->artifWorn[4]);
	artWorn[4]->pos.x = 564;
	artWorn[4]->pos.y = 183;
	artWorn[4]->pos.h = artWorn[4]->pos.w = 44;
	if(hero->artifWorn[4])
		artWorn[4]->text = hero->artifWorn[4]->description;
	else
		artWorn[4]->text = std::string();
	artWorn[4]->ourWindow = this;
	artWorn[4]->lHand = true;

	artWorn[7] = new CArtPlace(hero->artifWorn[7]);
	artWorn[7]->pos.x = 610;
	artWorn[7]->pos.y = 183;
	artWorn[7]->pos.h = artWorn[7]->pos.w = 44;
	if(hero->artifWorn[7])
		artWorn[7]->text = hero->artifWorn[7]->description;
	else
		artWorn[7]->text = std::string();
	artWorn[7]->ourWindow = this;
	artWorn[7]->lRing = true;

	artWorn[13] = new CArtPlace(hero->artifWorn[13]);
	artWorn[13]->pos.x = 564;
	artWorn[13]->pos.y = 30;
	artWorn[13]->pos.h = artWorn[13]->pos.w = 44;
	if(hero->artifWorn[13])
		artWorn[13]->text = hero->artifWorn[13]->description;
	else
		artWorn[13]->text = std::string();
	artWorn[13]->ourWindow = this;
	artWorn[13]->warMachine1 = true;

	artWorn[14] = new CArtPlace(hero->artifWorn[14]);
	artWorn[14]->pos.x = 610;
	artWorn[14]->pos.y = 30;
	artWorn[14]->pos.h = artWorn[14]->pos.w = 44;
	if(hero->artifWorn[14])
		artWorn[14]->text = hero->artifWorn[14]->description;
	else
		artWorn[14]->text = std::string();
	artWorn[14]->ourWindow = this;
	artWorn[14]->warMachine2 = true;

	artWorn[15] = new CArtPlace(hero->artifWorn[15]);
	artWorn[15]->pos.x = 610;
	artWorn[15]->pos.y = 76;
	artWorn[15]->pos.h = artWorn[15]->pos.w = 44;
	if(hero->artifWorn[15])
		artWorn[15]->text = hero->artifWorn[15]->description;
	else
		artWorn[15]->text = std::string();
	artWorn[15]->ourWindow = this;
	artWorn[15]->warMachine3 = true;

	artWorn[16] = new CArtPlace(hero->artifWorn[16]);
	artWorn[16]->pos.x = 610;
	artWorn[16]->pos.y = 122;
	artWorn[16]->pos.h = artWorn[16]->pos.w = 44;
	if(hero->artifWorn[16])
		artWorn[16]->text = hero->artifWorn[16]->description;
	else
		artWorn[16]->text = std::string();
	artWorn[16]->ourWindow = this;
	artWorn[16]->warMachine4 = true;

	artWorn[9] = new CArtPlace(hero->artifWorn[9]);
	artWorn[9]->pos.x = 383;
	artWorn[9]->pos.y = 143;
	artWorn[9]->pos.h = artWorn[9]->pos.w = 44;
	if(hero->artifWorn[9])
		artWorn[9]->text = hero->artifWorn[9]->description;
	else
		artWorn[9]->text = std::string();
	artWorn[9]->ourWindow = this;
	artWorn[9]->misc1 = true;

	artWorn[10] = new CArtPlace(hero->artifWorn[10]);
	artWorn[10]->pos.x = 399;
	artWorn[10]->pos.y = 194;
	artWorn[10]->pos.h = artWorn[10]->pos.w = 44;
	if(hero->artifWorn[10])
		artWorn[10]->text = hero->artifWorn[10]->description;
	else
		artWorn[10]->text = std::string();
	artWorn[10]->ourWindow = this;
	artWorn[10]->misc1 = true;

	artWorn[11] = new CArtPlace(hero->artifWorn[11]);
	artWorn[11]->pos.x = 415;
	artWorn[11]->pos.y = 245;
	artWorn[11]->pos.h = artWorn[11]->pos.w = 44;
	if(hero->artifWorn[11])
		artWorn[11]->text = hero->artifWorn[11]->description;
	else
		artWorn[11]->text = std::string();
	artWorn[11]->ourWindow = this;
	artWorn[11]->misc3 = true;

	artWorn[12] = new CArtPlace(hero->artifWorn[12]);
	artWorn[12]->pos.x = 431;
	artWorn[12]->pos.y = 296;
	artWorn[12]->pos.h = artWorn[12]->pos.w = 44;
	if(hero->artifWorn[12])
		artWorn[12]->text = hero->artifWorn[12]->description;
	else
		artWorn[12]->text = std::string();
	artWorn[12]->ourWindow = this;
	artWorn[12]->misc4 = true;

	artWorn[18] = new CArtPlace(hero->artifWorn[18]);
	artWorn[18]->pos.x = 381;
	artWorn[18]->pos.y = 296;
	artWorn[18]->pos.h = artWorn[18]->pos.w = 44;
	if(hero->artifWorn[18])
		artWorn[18]->text = hero->artifWorn[18]->description;
	else
		artWorn[18]->text = std::string();
	artWorn[18]->ourWindow = this;
	artWorn[18]->misc5 = true;

	artWorn[2] = new CArtPlace(hero->artifWorn[2]);
	artWorn[2]->pos.x = 508;
	artWorn[2]->pos.y = 79;
	artWorn[2]->pos.h = artWorn[2]->pos.w = 44;
	if(hero->artifWorn[2])
		artWorn[2]->text = hero->artifWorn[2]->description;
	else
		artWorn[2]->text = std::string();
	artWorn[2]->ourWindow = this;
	artWorn[2]->neck = true;

	artWorn[3] = new CArtPlace(hero->artifWorn[3]);
	artWorn[3]->pos.x = 383;
	artWorn[3]->pos.y = 68;
	artWorn[3]->pos.h = artWorn[3]->pos.w = 44;
	if(hero->artifWorn[3])
		artWorn[3]->text = hero->artifWorn[3]->description;
	else
		artWorn[3]->text = std::string();
	artWorn[3]->ourWindow = this;
	artWorn[3]->rHand = true;

	artWorn[6] = new CArtPlace(hero->artifWorn[6]);
	artWorn[6]->pos.x = 431;
	artWorn[6]->pos.y = 68;
	artWorn[6]->pos.h = artWorn[6]->pos.w = 44;
	if(hero->artifWorn[6])
		artWorn[6]->text = hero->artifWorn[6]->description;
	else
		artWorn[6]->text = std::string();
	artWorn[6]->ourWindow = this;
	artWorn[6]->rRing = true;

	artWorn[1] = new CArtPlace(hero->artifWorn[1]);
	artWorn[1]->pos.x = 567;
	artWorn[1]->pos.y = 240;
	artWorn[1]->pos.h = artWorn[1]->pos.w = 44;
	if(hero->artifWorn[1])
		artWorn[1]->text = hero->artifWorn[1]->description;
	else
		artWorn[1]->text = std::string();
	artWorn[1]->ourWindow = this;
	artWorn[1]->shoulders = true;

	artWorn[17] = new CArtPlace(hero->artifWorn[17]);
	artWorn[17]->pos.x = 610;
	artWorn[17]->pos.y = 310;
	artWorn[17]->pos.h = artWorn[17]->pos.w = 44;
	if(hero->artifWorn[17])
		artWorn[17]->text = hero->artifWorn[17]->description;
	else
		artWorn[17]->text = std::string();
	artWorn[17]->ourWindow = this;
	artWorn[17]->spellBook = true;

	artWorn[5] = new CArtPlace(hero->artifWorn[5]);
	artWorn[5]->pos.x = 509;
	artWorn[5]->pos.y = 130;
	artWorn[5]->pos.h = artWorn[5]->pos.w = 44;
	if(hero->artifWorn[5])
		artWorn[5]->text = hero->artifWorn[5]->description;
	else
		artWorn[5]->text = std::string();
	artWorn[5]->ourWindow = this;
	artWorn[5]->torso = true;

	for(int g=0; g<artWorn.size(); ++g)
	{
		artWorn[g]->myNumber = g;
		artWorn[g]->backNumber = -1;
		char * hll = new char[200];
		sprintf(hll, CGI->generaltexth->heroscrn[1].c_str(), (artWorn[g]->ourArt ? artWorn[g]->ourArt->name.c_str() : ""));
		artWorn[g]->hoverText = std::string(hll);
		delete [] hll;
	}

	for(int s=0; s<5; ++s)
	{
		CArtPlace * add;
		if( s < curHero->artifacts.size() )
			add = new CArtPlace(curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ]);
		else
			add = new CArtPlace(NULL);
		add->pos.x = 403 + 46*s;
		add->pos.y = 365;
		add->pos.h = add->pos.w = 44;
		if(s<hero->artifacts.size() && hero->artifacts[s])
			add->text = hero->artifacts[s]->description;
		else
			add->text = std::string();
		add->ourWindow = this;
		add->spellBook = true;
		add->warMachine1 = true;
		add->warMachine2 = true;
		add->warMachine3 = true;
		add->warMachine4 = true;
		add->misc1 = true;
		add->misc2 = true;
		add->misc3 = true;
		add->misc4 = true;
		add->misc5 = true;
		add->feet = true;
		add->lRing = true;
		add->rRing = true;
		add->torso = true;
		add->lHand = true;
		add->rHand = true;
		add->neck = true;
		add->shoulders = true;
		add->head = true;
		add->myNumber = -1;
		add->backNumber = s;
		backpack.push_back(add);
	}
	activeArtPlace = NULL;
	redrawCurBack();
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
	deactivate();

	LOCPLINT->adventureInt->activate();

	SDL_FreeSurface(curBack);
	curBack = NULL;
	/*for(int v=0; v<LOCPLINT->lclickable.size(); ++v)
	{
		if(dynamic_cast<CArtPlace*>(LOCPLINT->lclickable[v]))
			LOCPLINT->lclickable.erase(LOCPLINT->lclickable.begin()+v);
	}*/

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
	LOCPLINT->curint = this;
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
	const CGHeroInstance * ch = curHero;
	quit();
	LOCPLINT->cb->dismissHero(ch);
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
	if(curHero->artifacts.size()>5) //if it is <=5, we have nothing to scroll
	{
		backpackPos+=curHero->artifacts.size()-1; //set new offset

		for(int s=0; s<5 && s<curHero->artifacts.size(); ++s) //set new data
		{
			backpack[s]->ourArt = curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ];
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
			backpack[s]->ourArt = curHero->artifacts[(s+backpackPos) % curHero->artifacts.size() ];
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

CArtPlace::CArtPlace(const CArtifact * const & art): ourArt(art), active(false), clicked(false),
	spellBook(false), warMachine1(false), warMachine2(false), warMachine3(false),
	warMachine4(false),misc1(false), misc2(false), misc3(false), misc4(false),
	misc5(false), feet(false), lRing(false), rRing(false), torso(false),
	lHand(false), rHand(false), neck(false), shoulders(false), head(false) {}
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
	if(!down && !clicked) //not clicked before
	{
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
				//swap artifacts
				
				LOCPLINT->cb->swapArifacts(
					ourWindow->curHero,
					this->myNumber>=0,
					this->myNumber>=0 ? this->myNumber : (this->backNumber + ourWindow->backpackPos)%ourWindow->curHero->artifacts.size(),
					ourWindow->curHero,
					ourWindow->activeArtPlace->myNumber>=0,
					ourWindow->activeArtPlace->myNumber>=0 ? ourWindow->activeArtPlace->myNumber : (ourWindow->activeArtPlace->backNumber + ourWindow->backpackPos)%ourWindow->curHero->artifacts.size());

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
		blitAt(CGI->arth->artDefs->ourImages[ourArt->id].bitmap, pos.x, pos.y, to);
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
	if( this->spellBook && art->spellBook || this->warMachine1 && art->warMachine1 ||
		this->warMachine2 && art->warMachine2 || this->warMachine3 && art->warMachine3 ||
		this->warMachine4 && art->warMachine4 || this->misc1 && art->misc1 ||
		this->misc2 && art->misc2 || this->misc3 && art->misc3 ||
		this->misc4 && art->misc4 || this->misc5 && art->misc5 ||
		this->feet && art->feet || this->lRing && art->lRing ||
		this->rRing && art->rRing || this->torso && art->torso ||
		this->lHand && art->lHand || this->rHand && art->rHand ||
		this->neck && art->neck || this->shoulders && art->shoulders ||
		this->head && art->head )
	{
		return true;
	}
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
	if(!down)
	{
		LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	}
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
	if(!down)
	{
		LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	}
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
		const CGHeroInstance * buf = LOCPLINT->cb->getHeroInfo(owner->player, id, false);
		owner->setHero(buf);
		owner->redrawCurBack();
		owner->activate();
	}
}

void LRClickableAreaWTextComp::clickLeft(boost::logic::tribool down)
{
	if((!down) && pressedL)
	{
		LOCPLINT->showInfoDialog(text, std::vector<SComponent*>(1, new SComponent(SComponent::Etype(baseType), type, bonus)));
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
