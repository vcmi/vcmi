/*
 * TextTextureCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TextTextureCache.h"

#include "SDLImage.h"
#include "SDL_Extensions.h"

#include "GameEngine.h"
#include "render/Colors.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

#include <SDL3/SDL_surface.h>

/// Comfortably more than a screenful of distinct strings, so nothing in use is evicted
static constexpr size_t cacheSizeLimit = 4096;

Point TextTextureCache::getAlignmentOffset(EFonts font, ETextAlignment alignment, const std::string & text)
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	Point size(fontPtr->getStringWidthScaled(text), fontPtr->getLineHeightScaled());

	switch(alignment)
	{
		case ETextAlignment::TOPCENTER:
		case ETextAlignment::CENTER:      return -size / 2;
		case ETextAlignment::BOTTOMRIGHT: return -size;
		default:                          return Point(0, 0);
	}
}

std::shared_ptr<SDLImageShared> TextTextureCache::getImage(EFonts font, const std::string & text)
{
	Key key{ static_cast<int>(font), text };

	auto it = entries.find(key);
	if(it != entries.end())
	{
		order.splice(order.begin(), order, it->second);
		return it->second->image;
	}

	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	Point size(fontPtr->getStringWidthScaled(text), fontPtr->getLineHeightScaled());

	if(size.x <= 0 || size.y <= 0)
		return nullptr;

	SDL_Surface * surface = CSDL_Ext::newSurface(size);
	if(!surface)
		return nullptr;

	// rendered white so the same texture can be tinted to any color with SDL_SetTextureColorMod
	fontPtr->renderText(surface, text, Colors::WHITE_TRUE, Point(0, 0));

	// Blending onto a transparent surface leaves the color multiplied by its alpha, so the image
	// is marked as premultiplied - drawing it as straight alpha would apply the glyph coverage a
	// second time and wash out every antialiased edge.
	// the image takes its own reference, so the surface created here is released again
	auto result = std::make_shared<SDLImageShared>(surface, true);
	SDL_DestroySurface(surface);

	order.push_front(Entry{key, result});
	entries[key] = order.begin();

	// Every string held here keeps a GPU texture alive, and the tile text overlay produces a
	// fresh one per tile - so the cache has to have an end, or it grows until the driver
	// runs out of memory.
	while(order.size() > cacheSizeLimit)
	{
		entries.erase(order.back().key);
		order.pop_back();
	}

	return result;
}

