#include "stdafx.h"
#include "CHeroWindow.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"

extern SDL_Surface * ekran;

CHeroWindow::CHeroWindow(int playerColor)
{
	background = SDL_LoadBMP("Data\\HEROSCR4.bmp");
	CSDL_Ext::blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;

	quitButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::quit, 674, 524, "hsbtns.def", this);
}

CHeroWindow::~CHeroWindow()
{
	SDL_FreeSurface(background);
	delete quitButton;
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(!to)
		to=ekran;
	blitAt(background,pos.x,pos.y,to);
	quitButton->show();
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
	LOCPLINT->adventureInt->show();
}

void CHeroWindow::activate()
{
	quitButton->activate();
}
