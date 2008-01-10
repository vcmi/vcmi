#include "stdafx.h"
#include "CHeroWindow.h"
#include "SDL.h"
#include "SDL_Extensions.h"

extern SDL_Surface * ekran;

CHeroWindow::CHeroWindow()
{
	background = SDL_LoadBMP("Data\\HEROSCR4.bmp");
	pos.x = 0;
	pos.y = 0;
	pos.h = background->h;
	pos.w = background->w;
}

CHeroWindow::~CHeroWindow()
{
	SDL_FreeSurface(background);
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(!to)
		to=ekran;
	blitAt(background,pos.x,pos.y,to);
}

void CHeroWindow::setHero(const CGHeroInstance *hero)
{
	curHero = hero;
}