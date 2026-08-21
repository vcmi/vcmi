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
#include "render/IFont.h"
#include "render/IRenderHandler.h"

#include <SDL3/SDL_surface.h>

/// Comfortably more than a screenful of distinct strings, so nothing in use is evicted
static constexpr size_t cacheSizeLimit = 4096;

TextTextureCache & TextTextureCache::get()
{
	static TextTextureCache instance;
	return instance;
}

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

std::shared_ptr<SDLImageShared> TextTextureCache::getImage(EFonts font, const ColorRGBA & color, const std::string & text)
{
	// not a ColorRGBA: that orders itself by mean brightness, colliding different colours
	const uint32_t packedColor = (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a;
	Key key{ static_cast<int>(font), packedColor, text };

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

	fontPtr->renderText(surface, text, color, Point(0, 0));

	// the image takes its own reference, so the surface created here is released again
	auto result = std::make_shared<SDLImageShared>(surface);
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

void TextTextureCache::clear()
{
	entries.clear();
	order.clear();
}
