#include "stdafx.h"
#include "CCursorHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "hch/CDefHandler.h"

extern SDL_Surface * screen;

void CCursorHandler::initCursor()
{
	mode = number = xpos = ypos = 0;
	help = CSDL_Ext::newSurface(32,32);
	cursors.push_back(CDefHandler::giveDef("CRADVNTR.DEF"));
	cursors.push_back(CDefHandler::giveDef("CRCOMBAT.DEF"));
	cursors.push_back(CDefHandler::giveDef("CRDEFLT.DEF"));
	cursors.push_back(CDefHandler::giveDef("CRSPELL.DEF"));
	SDL_ShowCursor(SDL_DISABLE);
}

void CCursorHandler::changeGraphic(int type, int no)
{
	mode = type;
	number = no;
}

void CCursorHandler::cursorMove(int x, int y)
{
	xpos = x;
	ypos = y;
}
void CCursorHandler::draw1()
{
	SDL_BlitSurface(screen,&genRect(32,32,xpos,ypos),help,&genRect(32,32,0,0));
	blitAt(cursors[mode]->ourImages[number].bitmap,xpos,ypos);
}
void CCursorHandler::draw2()
{
	blitAt(help,xpos,ypos);
}
