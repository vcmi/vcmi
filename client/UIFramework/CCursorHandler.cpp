#include "StdInc.h"
#include <SDL.h>
#include "CCursorHandler.h"
#include "GL2D.h"
#include "../Gfx/Animations.h"
#include "../Gfx/Images.h"

#include "../CAnimation.h"
#include "CGuiHandler.h"

/*
 * CCursorHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCursorHandler::CCursorHandler()
{
	xpos = ypos = 0;
	type = ECursor::DEFAULT;
	dndObject = nullptr;
	currentCursor = nullptr;

//*	help = CSDL_Ext::newSurface(40,40);
	SDL_ShowCursor(SDL_DISABLE);

	changeGraphic(ECursor::ADVENTURE, 0);
}


void CCursorHandler::changeGraphic(ECursor::ECursorTypes type, int index)
{
	const std::string cursorDefs[4] = { "CRADVNTR", "CRCOMBAT", "CRDEFLT", "CRSPELL" };

	if (type != this->type)
	{
		BLOCK_CAPTURING; // not used here

		this->type = type;
		this->frame = index;

		currentCursor = Gfx::CManager::getAnimation(cursorDefs[type]);
	}
	frame = index;
}


void CCursorHandler::dragAndDropCursor(CAnimImage * object)
{
	if (dndObject) delete dndObject;

	dndObject = object;
}

void CCursorHandler::cursorMove(int x, int y)
{
	xpos = x;
	ypos = y;
}

void CCursorHandler::drawWithScreenRestore()
{
	if(!showing) return;
	int x = xpos, y = ypos;
	shiftPos(x, y);

	if (dndObject)
	{
		dndObject->moveTo(Point(x - dndObject->pos.w/2, y - dndObject->pos.h/2));
		dndObject->showAll();
	}
	else
	{
		currentCursor->getFrame(frame)->putAt(Gfx::Point(x, y));
	}
}

void CCursorHandler::drawRestored()
{
	if(!showing)
		return;

	int x = xpos, y = ypos;
	shiftPos(x, y);
}

void CCursorHandler::draw()
{
	currentCursor->getFrame(frame)->putAt(Gfx::Point(xpos, ypos));
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
	this->xpos = (GL2D::getScreenWidth()  - currentCursor->getWidth()) / 2;
	this->ypos = (GL2D::getScreenHeight() - currentCursor->getHeight()) / 2;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);	
	SDL_WarpMouse(this->xpos, this->ypos);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

CCursorHandler::~CCursorHandler()
{
	delete dndObject;
}
