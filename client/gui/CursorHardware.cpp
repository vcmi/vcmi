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
#include "CursorHandler.h"

#include "SDL_Extensions.h"
#include "CGuiHandler.h"
#include "CAnimation.h"
#include "../../lib/CConfigHandler.h"

#include <SDL_render.h>
#include <SDL_events.h>

#ifdef VCMI_APPLE
#include <dispatch/dispatch.h>
#endif

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
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif
	if (on)
		SDL_ShowCursor(SDL_ENABLE);
	else
		SDL_ShowCursor(SDL_DISABLE);
#ifdef VCMI_APPLE
	});
#endif
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
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif
	SDL_SetCursor(cursor);

	if (oldCursor)
		SDL_FreeCursor(oldCursor);
#ifdef VCMI_APPLE
	});
#endif
}

void CursorHardware::setCursorPosition( const Point & newPos )
{
	//no-op
}

void CursorHardware::render()
{
	//no-op
}
