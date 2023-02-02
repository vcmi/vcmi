/*
 * CursorSoftware.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CursorSoftware.h"

#include "SDL_Extensions.h"

#include "../render/IImage.h"

#include <SDL_render.h>
#include <SDL_events.h>

void CursorSoftware::render()
{
	//texture must be updated in the main (renderer) thread, but changes to cursor type may come from other threads
	if (needUpdate)
		updateTexture();

	Point renderPos = pos - pivot;

	SDL_Rect destRect;
	destRect.x = renderPos.x;
	destRect.y = renderPos.y;
	destRect.w = 40;
	destRect.h = 40;

	SDL_RenderCopy(mainRenderer, cursorTexture, nullptr, &destRect);
}

void CursorSoftware::createTexture(const Point & dimensions)
{
	if(cursorTexture)
		SDL_DestroyTexture(cursorTexture);

	if (cursorSurface)
		SDL_FreeSurface(cursorSurface);

	cursorSurface = CSDL_Ext::newSurface(dimensions.x, dimensions.y);
	cursorTexture = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, dimensions.x, dimensions.y);

	SDL_SetSurfaceBlendMode(cursorSurface, SDL_BLENDMODE_NONE);
	SDL_SetTextureBlendMode(cursorTexture, SDL_BLENDMODE_BLEND);
}

void CursorSoftware::updateTexture()
{
	Point dimensions(-1, -1);

	if (!cursorSurface ||  Point(cursorSurface->w, cursorSurface->h) != cursorImage->dimensions())
		createTexture(cursorImage->dimensions());

	CSDL_Ext::fillSurface(cursorSurface, Colors::TRANSPARENCY);

	cursorImage->draw(cursorSurface);
	SDL_UpdateTexture(cursorTexture, NULL, cursorSurface->pixels, cursorSurface->pitch);
	needUpdate = false;
}

void CursorSoftware::setImage(std::shared_ptr<IImage> image, const Point & pivotOffset)
{
	assert(image != nullptr);
	cursorImage = image;
	pivot = pivotOffset;
	needUpdate = true;
}

void CursorSoftware::setCursorPosition( const Point & newPos )
{
	pos = newPos;
}

void CursorSoftware::setVisible(bool on)
{
	visible = on;
}

CursorSoftware::CursorSoftware():
	cursorTexture(nullptr),
	cursorSurface(nullptr),
	needUpdate(false),
	visible(false),
	pivot(0,0)
{
	SDL_ShowCursor(SDL_DISABLE);
}

CursorSoftware::~CursorSoftware()
{
	if(cursorTexture)
		SDL_DestroyTexture(cursorTexture);

	if (cursorSurface)
		SDL_FreeSurface(cursorSurface);
}

