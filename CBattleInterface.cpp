#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "hch\CHeroHandler.h"
#include "hch\CDefHandler.h"

extern SDL_Surface * screen;

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2)
{
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
		attackingHero = new CBattleHero;
		attackingHero->dh = CGI->spriteh->giveDef( CGI->mh->battleHeroes[hero1->type->heroType] );
		for(int i=0; i<attackingHero->dh->ourImages.size(); ++i) //transforming images
		{
			attackingHero->dh->ourImages[i].bitmap = CSDL_Ext::alphaTransform(attackingHero->dh->ourImages[i].bitmap);
		}
		attackingHero->dh->alphaTransformed = true;
		attackingHero->phase = 0;
		attackingHero->image = 0;
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, 0, 0);
		//CSDL_Ext::blit8bppAlphaTo24bpp(attackingHero->ourImages[0].bitmap, &attackingHero->ourImages[0].bitmap->clip_rect, screen, &genRect(attackingHero->ourImages[0].bitmap->h, attackingHero->ourImages[0].bitmap->w, 0, 0));
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		defendingHero = new CBattleHero;
		defendingHero->dh = CGI->spriteh->giveDef( CGI->mh->battleHeroes[hero2->type->heroType] );
		for(int i=0; i<defendingHero->dh->ourImages.size(); ++i) //transforming and flipping images
		{
			defendingHero->dh->ourImages[i].bitmap = CSDL_Ext::rotate01(defendingHero->dh->ourImages[i].bitmap);
			defendingHero->dh->ourImages[i].bitmap = CSDL_Ext::alphaTransform(defendingHero->dh->ourImages[i].bitmap);
		}
		defendingHero->dh->alphaTransformed = true;
		defendingHero->phase = 0;
		defendingHero->image = 0;
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, 650, 0);
		//CSDL_Ext::blit8bppAlphaTo24bpp(defendingHero->ourImages[0].bitmap, &defendingHero->ourImages[0].bitmap->clip_rect, screen, &genRect(defendingHero->ourImages[0].bitmap->h, defendingHero->ourImages[0].bitmap->w, 650, 0));
	}
	else
	{
		defendingHero = NULL;
	}
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
	//blitAt(menu, 0, 556, to);

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
	attackingHero->show(to);
	defendingHero->show(to);

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
			CSDL_Ext::blit8bppAlphaTo24bpp(dh->ourImages[i].bitmap, NULL, to, &pos);
			++image;
			if(dh->ourImages[i+1].groupNumber!=phase) //back to appropriate frame
			{
				image = 0;
			}
			break;
		}
	}
}

CBattleHero::~CBattleHero()
{
	delete dh;
}
