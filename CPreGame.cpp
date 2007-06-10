#include "stdafx.h"
#include "CPreGame.h"
#include "SDL.h";
extern SDL_Surface * ekran;
CPreGame::CPreGame()
{
	initMainMenu();
	showMainMenu();
}
void CPreGame::initMainMenu()
{
	ourMainMenu = new menuItems();
	ourMainMenu->background = SDL_LoadBMP("h3bitmap.lod\\ZPIC1005.bmp");
	CSemiLodHandler * slh = new CSemiLodHandler();
	slh->openLod("H3sprite.lod");
	ourMainMenu->newGame = slh->giveDef("ZMENUNG.DEF");
	handledLods.push_back(slh);
	delete slh;
}
void CPreGame::showMainMenu()
{
	SDL_BlitSurface(ourMainMenu->background,NULL,ekran,NULL);
	SDL_Flip(ekran);
	SDL_BlitSurface(ourMainMenu->newGame->ourImages[0].bitmap,NULL,ekran,NULL);
	SDL_Flip(ekran);
}
void CPreGame::runLoop()
{}