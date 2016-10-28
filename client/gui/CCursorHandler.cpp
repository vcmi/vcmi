/*
 * CCursorHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCursorHandler.h"

#include <SDL.h>

#include "SDL_Extensions.h"
#include "CGuiHandler.h"
#include "../widgets/Images.h"

#include "../CMT.h"

void CCursorHandler::initCursor()
{
	xpos = ypos = 0;
	type = ECursor::DEFAULT;
	dndObject = nullptr;

	cursors =
	{
		make_unique<CAnimImage>("CRADVNTR", 0),
		make_unique<CAnimImage>("CRCOMBAT", 0),
		make_unique<CAnimImage>("CRDEFLT",  0),
		make_unique<CAnimImage>("CRSPELL",  0)
	};

	currentCursor = cursors.at(int(ECursor::DEFAULT)).get();

	help = CSDL_Ext::newSurface(40,40);
	//No blending. Ensure, that we are copying pixels during "screen restore draw"
	SDL_SetSurfaceBlendMode(help,SDL_BLENDMODE_NONE);
	SDL_ShowCursor(SDL_DISABLE);

	changeGraphic(ECursor::ADVENTURE, 0);
}

void CCursorHandler::changeGraphic(ECursor::ECursorTypes type, int index)
{
	if(type != this->type)
	{
		this->type = type;
		this->frame = index;
		currentCursor = cursors.at(int(type)).get();
		currentCursor->setFrame(index);
	}
	else if(index != this->frame)
	{
		this->frame = index;
		currentCursor->setFrame(index);
	}
}

void CCursorHandler::dragAndDropCursor(std::unique_ptr<CAnimImage> object)
{
	dndObject = std::move(object);
}

void CCursorHandler::cursorMove(const int & x, const int & y)
{
	xpos = x;
	ypos = y;
}

void CCursorHandler::drawWithScreenRestore()
{
	if(!showing) return;
	int x = xpos, y = ypos;
	shiftPos(x, y);

	SDL_Rect temp_rect1 = genRect(40,40,x,y);
	SDL_Rect temp_rect2 = genRect(40,40,0,0);
	SDL_BlitSurface(screen, &temp_rect1, help, &temp_rect2);

	if (dndObject)
	{
		dndObject->moveTo(Point(x - dndObject->pos.w/2, y - dndObject->pos.h/2));
		dndObject->showAll(screen);
	}
	else
	{
		currentCursor->moveTo(Point(x,y));
		currentCursor->showAll(screen);
	}
}

void CCursorHandler::drawRestored()
{
	if(!showing)
		return;

	int x = xpos, y = ypos;
	shiftPos(x, y);

	SDL_Rect temp_rect = genRect(40, 40, x, y);
	SDL_BlitSurface(help, nullptr, screen, &temp_rect);
}

void CCursorHandler::shiftPos( int &x, int &y )
{
	if(( type == ECursor::COMBAT && frame != ECursor::COMBAT_POINTER) || type == ECursor::SPELLBOOK)
	{
		x-=16;
		y-=16;

		// Properly align the melee attack cursors.
		if (type == ECursor::COMBAT)
		{
			switch (frame)
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
	else if(type == ECursor::ADVENTURE)
	{
		if (frame == 0); //to exclude
		else if(frame == 2)
		{
			x -= 12;
			y -= 10;
		}
		else if(frame == 3)
		{
			x -= 12;
			y -= 12;
		}
		else if(frame < 27)
		{
			int hlpNum = (frame - 4)%6;
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
		else if(frame == 41)
		{
			x -= 14;
			y -= 16;
		}
		else if(frame < 31 || frame == 42)
		{
			x -= 20;
			y -= 20;
		}
	}
}

void CCursorHandler::centerCursor()
{
	this->xpos = (screen->w / 2.) - (currentCursor->pos.w / 2.);
	this->ypos = (screen->h / 2.) - (currentCursor->pos.h / 2.);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouse(this->xpos, this->ypos);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

void CCursorHandler::render()
{
	drawWithScreenRestore();
	CSDL_Ext::update(screen);
	drawRestored();
}

CCursorHandler::CCursorHandler() = default;

CCursorHandler::~CCursorHandler()
{
	if(help)
		SDL_FreeSurface(help);
}
