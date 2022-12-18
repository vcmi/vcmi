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

void CCursorHandler::clearBuffer()
{
	Uint32 fillColor = SDL_MapRGBA(buffer->format, 0, 0, 0, 0);
	CSDL_Ext::fillRect(buffer, nullptr, fillColor);
}

void CCursorHandler::updateBuffer(CIntObject * payload)
{
	payload->moveTo(Point(0,0));
	payload->showAll(buffer);

	needUpdate = true;
}

void CCursorHandler::replaceBuffer(CIntObject * payload)
{
	clearBuffer();
	updateBuffer(payload);
}

CCursorHandler::CCursorHandler()
	: needUpdate(true)
	, buffer(nullptr)
	, cursorLayer(nullptr)
	, frameTime(0.f)
	, showing(false)
{
	cursorLayer = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 40, 40);
	SDL_SetTextureBlendMode(cursorLayer, SDL_BLENDMODE_BLEND);

	xpos = ypos = 0;
	type = Cursor::Type::DEFAULT;
	dndObject = nullptr;

	cursors =
	{
		make_unique<CAnimImage>("CRADVNTR", 0),
		make_unique<CAnimImage>("CRCOMBAT", 0),
		make_unique<CAnimImage>("CRDEFLT",  0),
		make_unique<CAnimImage>("CRSPELL",  0)
	};

	currentCursor = cursors.at(static_cast<size_t>(Cursor::Type::DEFAULT)).get();

	buffer = CSDL_Ext::newSurface(40,40);

	SDL_SetSurfaceBlendMode(buffer, SDL_BLENDMODE_NONE);
	SDL_ShowCursor(SDL_DISABLE);

	set(Cursor::Map::POINTER);
}

Point CCursorHandler::position() const
{
	return Point(xpos, ypos);
}

void CCursorHandler::changeGraphic(Cursor::Type type, size_t index)
{
	if(type != this->type)
	{
		this->type = type;
		this->frame = index;
		currentCursor = cursors.at(static_cast<size_t>(type)).get();
		currentCursor->setFrame(index);
	}
	else if(index != this->frame)
	{
		this->frame = index;
		currentCursor->setFrame(index);
	}

	replaceBuffer(currentCursor);
}

void CCursorHandler::set(Cursor::Default index)
{
	changeGraphic(Cursor::Type::DEFAULT, static_cast<size_t>(index));
}

void CCursorHandler::set(Cursor::Map index)
{
	changeGraphic(Cursor::Type::ADVENTURE, static_cast<size_t>(index));
}

void CCursorHandler::set(Cursor::Combat index)
{
	changeGraphic(Cursor::Type::COMBAT, static_cast<size_t>(index));
}

void CCursorHandler::set(Cursor::Spellcast index)
{
	//Note: this is animated cursor, ignore specified frame and only change type
	changeGraphic(Cursor::Type::SPELLBOOK, frame);
}

void CCursorHandler::dragAndDropCursor(std::unique_ptr<CAnimImage> object)
{
	dndObject = std::move(object);
	if(dndObject)
		replaceBuffer(dndObject.get());
	else
		replaceBuffer(currentCursor);
}

void CCursorHandler::cursorMove(const int & x, const int & y)
{
	xpos = x;
	ypos = y;
}

void CCursorHandler::shiftPos( int &x, int &y )
{
	if(( type == Cursor::Type::COMBAT && frame != static_cast<size_t>(Cursor::Combat::POINTER)) || type == Cursor::Type::SPELLBOOK)
	{
		x-=16;
		y-=16;

		// Properly align the melee attack cursors.
		if (type == Cursor::Type::COMBAT)
		{
			switch (static_cast<Cursor::Combat>(frame))
			{
			case Cursor::Combat::HIT_NORTHEAST:
				x -= 6;
				y += 16;
				break;
			case Cursor::Combat::HIT_EAST:
				x -= 16;
				y += 10;
				break;
			case Cursor::Combat::HIT_SOUTHEAST:
				x -= 6;
				y -= 6;
				break;
			case Cursor::Combat::HIT_SOUTHWEST:
				x += 16;
				y -= 6;
				break;
			case Cursor::Combat::HIT_WEST:
				x += 16;
				y += 11;
				break;
			case Cursor::Combat::HIT_NORTHWEST:
				x += 16;
				y += 16;
				break;
			case Cursor::Combat::HIT_NORTH:
				x += 9;
				y += 16;
				break;
			case Cursor::Combat::HIT_SOUTH:
				x += 9;
				y -= 15;
				break;
			}
		}
	}
	else if(type == Cursor::Type::ADVENTURE)
	{
		if (frame == 0)
		{
			//no-op
		}
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
	this->xpos = static_cast<int>((screen->w / 2.) - (currentCursor->pos.w / 2.));
	this->ypos = static_cast<int>((screen->h / 2.) - (currentCursor->pos.h / 2.));
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouse(this->xpos, this->ypos);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

void CCursorHandler::render()
{
	if(!showing)
		return;

	if (type == Cursor::Type::SPELLBOOK)
	{
		static const float frameDisplayDuration = 0.1f;

		frameTime += GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
		size_t newFrame = frame;

		while (frameTime > frameDisplayDuration)
		{
			frameTime -= frameDisplayDuration;
			newFrame++;
		}

		auto & animation = cursors.at(static_cast<size_t>(type));

		while (newFrame > animation->size())
			newFrame -= animation->size();

		changeGraphic(Cursor::Type::SPELLBOOK, newFrame);
	}

	//the must update texture in the main (renderer) thread, but changes to cursor type may come from other threads
	updateTexture();

	int x = xpos;
	int y = ypos;
	shiftPos(x, y);

	if(dndObject)
	{
		x -= dndObject->pos.w/2;
		y -= dndObject->pos.h/2;
	}

	SDL_Rect destRect;
	destRect.x = x;
	destRect.y = y;
	destRect.w = 40;
	destRect.h = 40;

	SDL_RenderCopy(mainRenderer, cursorLayer, nullptr, &destRect);
}

void CCursorHandler::updateTexture()
{
	if(needUpdate)
	{
		SDL_UpdateTexture(cursorLayer, nullptr, buffer->pixels, buffer->pitch);
		needUpdate = false;
	}
}

CCursorHandler::~CCursorHandler()
{
	if(buffer)
		SDL_FreeSurface(buffer);

	if(cursorLayer)
		SDL_DestroyTexture(cursorLayer);
}
