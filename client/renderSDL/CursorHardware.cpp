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

#include "SDL_Extensions.h"

#include "../gui/CGuiHandler.h"
#include "../render/IScreenHandler.h"
#include "../render/Colors.h"
#include "../render/IImage.h"

#include "../../lib/CConfigHandler.h"

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
	int videoScalingSettings = GH.screenHandler().getInterfaceScalingPercentage();
	float cursorScalingSettings = settings["video"]["cursorScalingFactor"].Float();
	int cursorScalingPercent = videoScalingSettings * cursorScalingSettings;
	Point cursorDimensions = image->dimensions() * GH.screenHandler().getScalingFactor();
	Point cursorDimensionsScaled = image->dimensions() * cursorScalingPercent / 100;
	Point pivotOffsetScaled = pivotOffset * cursorScalingPercent / 100 / GH.screenHandler().getScalingFactor();

	auto cursorSurface = CSDL_Ext::newSurface(cursorDimensions);

	CSDL_Ext::fillSurface(cursorSurface, CSDL_Ext::toSDL(Colors::TRANSPARENCY));

	image->draw(cursorSurface, Point(0,0));
	auto cursorSurfaceScaled = CSDL_Ext::scaleSurface(cursorSurface, cursorDimensionsScaled.x, cursorDimensionsScaled.y );

	auto oldCursor = cursor;
	cursor = SDL_CreateColorCursor(cursorSurfaceScaled, pivotOffsetScaled.x, pivotOffsetScaled.y);

	if (!cursor)
		logGlobal->error("Failed to set cursor! SDL says %s", SDL_GetError());

	SDL_FreeSurface(cursorSurface);
	SDL_FreeSurface(cursorSurfaceScaled);

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
