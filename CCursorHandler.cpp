#include "stdafx.h"
#include "CCursorHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "hch/CDefHandler.h"

/*
 * CCursorHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

void CCursorHandler::changeGraphic(const int & type, const int & no)
{
	mode = type;
	number = no;
}

void CCursorHandler::cursorMove(const int & x, const int & y)
{
	xpos = x;
	ypos = y;
}
void CCursorHandler::draw1()
{
	if(!Show) return;
	int x = xpos, y = ypos;
	if((mode==1 && number!=6) || mode ==3)
	{
		x-=16;
		y-=16;
	}
	else if(mode==0 && number>0)
	{
		x-=12;
		y-=10;
	}
	SDL_BlitSurface(screen, &genRect(32,32,x,y), help, &genRect(32,32,0,0));
	blitAt(cursors[mode]->ourImages[number].bitmap,x,y);
}
void CCursorHandler::draw2()
{
	if(!Show) return;
	int x = xpos, y = ypos;
	if((mode==1 && number!=6) || mode == 3)
	{
		x-=16;
		y-=16;
	}
	else if(mode==0 && number>0)
	{
		x-=12;
		y-=10;
	}
	blitAt(help,x,y);
}
