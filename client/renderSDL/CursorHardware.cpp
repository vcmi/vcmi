/*
 * CursorHardware.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CursorHardware.h"

#include "../gui/CGuiHandler.h"
#include "../render/Colors.h"
#include "../render/IImage.h"
#include "SDL_Extensions.h"

#include <SDL_render.h>
#include <SDL_events.h>

CursorHardware::CursorHardware():
	cursor(nullptr)
{
	SDL_ShowCursor(SDL_DISABLE);
}

CursorHardware::~CursorHardware()
{
	if(cursor)
		SDL_FreeCursor(cursor);
}

void CursorHardware::setVisible(bool on)
{
	GH.dispatchMainThread([on]()
	{
		if (on)
			SDL_ShowCursor(SDL_ENABLE);
		else
			SDL_ShowCursor(SDL_DISABLE);
	});
}

void CursorHardware::setImage(std::shared_ptr<IImage> image, const Point & pivotOffset)
{
	auto cursorSurface = CSDL_Ext::newSurface(image->dimensions().x, image->dimensions().y);

	CSDL_Ext::fillSurface(cursorSurface, Colors::TRANSPARENCY);

	image->draw(cursorSurface);

	auto oldCursor = cursor;
	cursor = SDL_CreateColorCursor(cursorSurface, pivotOffset.x, pivotOffset.y);

	if (!cursor)
		logGlobal->error("Failed to set cursor! SDL says %s", SDL_GetError());

	SDL_FreeSurface(cursorSurface);

	GH.dispatchMainThread([this, oldCursor](){
		SDL_SetCursor(cursor);

		if (oldCursor)
			SDL_FreeCursor(oldCursor);
	});
}

void CursorHardware::setCursorPosition( const Point & newPos )
{
	//no-op
}

void CursorHardware::render()
{
	//no-op
}
