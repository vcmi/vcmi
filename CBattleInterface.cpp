#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "hch\CHeroHandler.h"
#include "hch\CDefHandler.h"

extern SDL_Surface * screen;

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2) : printCellBorders(true)
{
	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	for(std::map<int,std::pair<CCreature*,int> >::iterator i=army1->slots.begin(); i!=army1->slots.end(); ++i)
	{
		//creAnim1.push_back(new CCreatureAnimation(i->second.first->animDefName));
		creAnim1.push_back(new CCreatureAnimation(i->second.first->animDefName));
		creAnim1[creAnim1.size()-1]->setType(2);
	}
	for(std::map<int,std::pair<CCreature*,int> >::iterator i=army2->slots.begin(); i!=army2->slots.end(); ++i)
	{
		creAnim2.push_back(new CCreatureAnimation(i->second.first->animDefName));
		creAnim2[creAnim2.size()-1]->setType(2);
	}
	//preparing menu background and terrain
	std::vector< std::string > & backref = CGI->mh->battleBacks[ CGI->mh->ttiles[tile.x][tile.y][tile.z].terType ];
	background = CGI->bitmaph->loadBitmap(backref[ rand() % backref.size()] );
	menu = CGI->bitmaph->loadBitmap("CBAR.BMP");
	CSDL_Ext::blueToPlayersAdv(menu, hero1->tempOwner);

	//blitting menu background and terrain
	blitAt(background, 0, 0);
	blitAt(menu, 0, 556);
	CSDL_Ext::update();
	
	//preparing buttons
	bOptions = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bOptionsf, 3, 561, "icm003.def", this, false, NULL, false);
	bSurrender = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bSurrenderf, 54, 561, "icm001.def", this, false, NULL, false);
	bFlee = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bFleef, 105, 561, "icm002.def", this, false, NULL, false);
	bAutofight  = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bAutofightf, 157, 561, "icm004.def", this, false, NULL, false);
	bSpell = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bSpellf, 645, 561, "icm005.def", this, false, NULL, false);
	bWait = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bWaitf, 696, 561, "icm006.def", this, false, NULL, false);
	bDefence = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bDefencef, 747, 561, "icm007.def", this, false, NULL, false);
	bConsoleUp = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bConsoleUpf, 624, 561, "ComSlide.def", this, false, NULL, false);
	bConsoleDown = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bConsoleDownf, 624, 580, "ComSlide.def", this, false, NULL, false);
	bConsoleDown->bitmapOffset = 2;
	
	//loading hero animations
	if(hero1) // attacking hero
	{
		attackingHero = new CBattleHero(CGI->mh->battleHeroes[hero1->type->heroType], 0, 0, false, hero1->tempOwner);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, -40, 0);
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		defendingHero = new CBattleHero(CGI->mh->battleHeroes[hero2->type->heroType], 0, 0, true, hero2->tempOwner);
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, 690, 0);
	}
	else
	{
		defendingHero = NULL;
	}

	//preparing cells and hexes
	cellBorder = CGI->bitmaph->loadBitmap("CCELLGRD.BMP");
	cellBorder = CSDL_Ext::alphaTransform(cellBorder);
	cellShade = CGI->bitmaph->loadBitmap("CCELLSHD.BMP");
	cellShade = CSDL_Ext::alphaTransform(cellShade);
}

CBattleInterface::~CBattleInterface()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(menu);
	delete bOptions;
	delete bSurrender;
	delete bFlee;
	delete bAutofight;
	delete bSpell;
	delete bWait;
	delete bDefence;
	delete bConsoleUp;
	delete bConsoleDown;

	delete attackingHero;
	delete defendingHero;

	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	for(int g=0; g<creAnim1.size(); ++g)
		delete creAnim1[g];
	for(int g=0; g<creAnim2.size(); ++g)
		delete creAnim2[g];
}

void CBattleInterface::activate()
{
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();
	bConsoleUp->activate();
	bConsoleDown->activate();
}

void CBattleInterface::deactivate()
{
	bOptions->deactivate();
	bSurrender->deactivate();
	bFlee->deactivate();
	bAutofight->deactivate();
	bSpell->deactivate();
	bWait->deactivate();
	bDefence->deactivate();
	bConsoleUp->deactivate();
	bConsoleDown->deactivate();
}

void CBattleInterface::show(SDL_Surface * to)
{
	if(!to) //"evaluating" to
		to = screen;

	//showing background
	blitAt(background, 0, 0, to);
	if(printCellBorders) //printing cell borders
	{
		for(int i=0; i<11; ++i) //rows
		{
			for(int j=0; j<15; ++j) //columns
			{
				int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
				int y = 86 + 42 * i;
				CSDL_Ext::blit8bppAlphaTo24bpp(cellBorder, NULL, to, &genRect(cellBorder->h, cellBorder->w, x, y));
			}
		}
	}
	//showing menu background
	blitAt(menu, 0, 556, to);

	//showing buttons
	bOptions->show(to);
	bSurrender->show(to);
	bFlee->show(to);
	bAutofight->show(to);
	bSpell->show(to);
	bWait->show(to);
	bDefence->show(to);
	bConsoleUp->show(to);
	bConsoleDown->show(to);

	//showing hero animations
	if(attackingHero)
		attackingHero->show(to);
	if(defendingHero)
		defendingHero->show(to);

	//showing units //a lot of work...

	creAnim1[0]->nextFrame(to, -94, -139);

	//units shown

	CSDL_Ext::update();
}

void CBattleInterface::bOptionsf()
{
}

void CBattleInterface::bSurrenderf()
{
}

void CBattleInterface::bFleef()
{
	for(int i=0; i<LOCPLINT->objsToBlit.size(); ++i)
	{
		if( dynamic_cast<CBattleInterface*>( LOCPLINT->objsToBlit[i] ) )
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+i);
		}
	}
	deactivate();

	LOCPLINT->adventureInt->activate();
	delete this;
}

void CBattleInterface::bAutofightf()
{
}

void CBattleInterface::bSpellf()
{
}

void CBattleInterface::bWaitf()
{
}

void CBattleInterface::bDefencef()
{
}

void CBattleInterface::bConsoleUpf()
{
}

void CBattleInterface::bConsoleDownf()
{
}

void CBattleHero::show(SDL_Surface *to)
{
	int tick=-1;
	for(int i=0; i<dh->ourImages.size(); ++i)
	{
		if(dh->ourImages[i].groupNumber==phase)
			++tick;
		if(tick==image)
		{
			SDL_Rect posb = pos;
			CSDL_Ext::blit8bppAlphaTo24bpp(dh->ourImages[i].bitmap, NULL, to, &posb);
			++image;
			if(dh->ourImages[i+1].groupNumber!=phase) //back to appropriate frame
			{
				image = 0;
			}
			break;
		}
	}
	if(flip)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(flag->ourImages[flagAnim].bitmap, NULL, screen, &genRect(flag->ourImages[flagAnim].bitmap->h, flag->ourImages[flagAnim].bitmap->w, 752, 39));
	}
	else
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(flag->ourImages[flagAnim].bitmap, NULL, screen, &genRect(flag->ourImages[flagAnim].bitmap->h, flag->ourImages[flagAnim].bitmap->w, 31, 39));
	}
	//++flagAnimCount;
	//if(flagAnimCount%4==0)
	{
		++flagAnim;
		flagAnim %= flag->ourImages.size();
	}
}

CBattleHero::CBattleHero(std::string defName, int phaseG, int imageG, bool flipG, unsigned char player): phase(phaseG), image(imageG), flip(flipG), flagAnim(0)
{
	dh = CGI->spriteh->giveDef( defName );
	for(int i=0; i<dh->ourImages.size(); ++i) //transforming images
	{
		if(flip)
			dh->ourImages[i].bitmap = CSDL_Ext::rotate01(dh->ourImages[i].bitmap); 
		dh->ourImages[i].bitmap = CSDL_Ext::alphaTransform(dh->ourImages[i].bitmap);
	}
	dh->alphaTransformed = true;

	if(flip)
		flag = CGI->spriteh->giveDef("CMFLAGR.DEF");
	else
		flag = CGI->spriteh->giveDef("CMFLAGL.DEF");

	for(int i=0; i<flag->ourImages.size(); ++i)
	{
		flag->ourImages[i].bitmap = CSDL_Ext::alphaTransform(flag->ourImages[i].bitmap);
		CSDL_Ext::blueToPlayersAdv(flag->ourImages[i].bitmap, player);
	}
}

CBattleHero::~CBattleHero()
{
	delete dh;
	delete flag;
}
