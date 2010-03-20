#include "../stdafx.h"
#include "CCursorHandler.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "../hch/CDefHandler.h"

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
	dndImage = NULL;
	help = CSDL_Ext::newSurface(40,40);
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

/**
 * Replaces the cursor with a custom image.
 *
 * @param image Image to replace cursor with or NULL to use the normal
 * cursor.
 */
void CCursorHandler::dragAndDropCursor(SDL_Surface* image)
{
	dndImage = image;
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
	shiftPos(x, y);
	SDL_BlitSurface(screen, &genRect(40,40,x,y), help, &genRect(40,40,0,0));
	if (dndImage)
		blitAt(dndImage, x - dndImage->w/2, y - dndImage->h/2);
	else
		blitAt(cursors[mode]->ourImages[number].bitmap,x,y);
}
void CCursorHandler::draw2()
{
	if(!Show) return;
	int x = xpos, y = ypos;
	shiftPos(x, y);
	blitAt(help,x,y);
}

void CCursorHandler::shiftPos( int &x, int &y )
{
	if((mode==1 && number!=6) || mode ==3)
	{
		x-=16;
		y-=16;

		// Properly align the melee attack cursors.
		if (mode == 1) 
		{
			switch (number) 
			{
			case 7: // Bottom left
				x -= 6;
				y += 16;
				break;
			case 8: // Left
				x -= 16;
				y += 10;
				break;
			case 9: // Top left
				x -= 6;
				y -= 6;
				break;
			case 10: // Top right
				x += 16;
				y -= 6;
				break;
			case 11: // Right
				x += 16;
				y += 11;
				break;
			case 12: // Bottom right
				x += 16;
				y += 16;
				break;
			case 13: // Below
				x += 9;
				y += 16;
				break;
			case 14: // Above
				x += 9;
				y -= 15;
				break;
			}
		}
	}
	else if(mode==0)
	{
		if(number == 0); //to exclude
		else if(number == 2)
		{
			x -= 12;
			y -= 10;
		}
		else if(number == 3)
		{
			x -= 12;
			y -= 12;
		}
		else if(number < 27)
		{
			int hlpNum = (number - 4)%6;
			if(hlpNum == 0)
			{
				x -= 15;
				y -= 13;
			}
			else if(hlpNum == 1)
			{
				x -= 13;
				y -= 13;
			}
			else if(hlpNum == 2)
			{
				x -= 20;
				y -= 20;
			}
			else if(hlpNum == 3)
			{
				x -= 13;
				y -= 16;
			}
			else if(hlpNum == 4)
			{
				x -= 8;
				y -= 9;
			}
			else if(hlpNum == 5)
			{
				x -= 14;
				y -= 16;
			}
		}
		else if(number == 41)
		{
			x -= 14;
			y -= 16;
		}
		else if(number < 31 || number == 42)
		{
			x -= 20;
			y -= 20;
		}
	}
}

CCursorHandler::~CCursorHandler()
{
	if(help)
		SDL_FreeSurface(help);

	for(int g=0; g<cursors.size(); ++g)
		delete cursors[g];
}
