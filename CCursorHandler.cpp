#include "stdafx.h"
#include "CCursorHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "hch/CDefHandler.h"

extern SDL_Surface * screen;

void CCursorHandler::initCursor()
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000;
    int gmask = 0x00ff0000;
    int bmask = 0x0000ff00;
    int amask = 0x000000ff;
#else
    int rmask = 0x000000ff;
    int gmask = 0x0000ff00;
    int bmask = 0x00ff0000;
    int amask = 0xff000000;
#endif
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