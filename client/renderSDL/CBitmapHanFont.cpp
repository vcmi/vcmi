/*
 * CBitmapHanFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBitmapHanFont.h"

#include "CBitmapFont.h"
#include "SDL_Extensions.h"

#include "../../lib/JsonNode.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/TextOperations.h"
#include "../../lib/Rect.h"

#include <SDL_surface.h>

size_t CBitmapHanFont::getCharacterDataOffset(size_t index) const
{
	size_t rowSize  = (size + 7) / 8; // 1 bit per pixel, rounded up
	size_t charSize = rowSize * size; // glyph contains "size" rows
	return index * charSize;
}

size_t CBitmapHanFont::getCharacterIndex(ui8 first, ui8 second) const
{
	if (second > 0x7f )
		second--;

	return (first - 0x81) * (12*16 - 2) + (second - 0x40);
}

void CBitmapHanFont::renderCharacter(SDL_Surface * surface, int characterIndex, const SDL_Color & color, int &posX, int &posY) const
{
	//TODO: somewhat duplicated with CBitmapFont::renderCharacter();
	Rect clipRect;
	CSDL_Ext::getClipRect(surface, clipRect);

	CSDL_Ext::TColorPutter colorPutter = CSDL_Ext::getPutterFor(surface, 0);
	uint8_t bpp = surface->format->BytesPerPixel;

	// start of line, may differ from 0 due to end of surface or clipped surface
	int lineBegin = std::max<int>(0, clipRect.y - posY);
	int lineEnd   = std::min((int)size, clipRect.y + clipRect.h - posY);

	// start end end of each row, may differ from 0
	int rowBegin = std::max<int>(0, clipRect.x - posX);
	int rowEnd   = std::min<int>((int)size, clipRect.x + clipRect.w - posX);

	//for each line in symbol
	for(int dy = lineBegin; dy <lineEnd; dy++)
	{
		uint8_t *dstLine = (uint8_t*)surface->pixels;
		uint8_t *source = data.first.get() + getCharacterDataOffset(characterIndex);

		// shift source\destination pixels to current position
		dstLine += (posY+dy) * surface->pitch + posX * bpp;
		source += ((size + 7) / 8) * dy;

		//for each column in line
		for(int dx = rowBegin; dx < rowEnd; dx++)
		{
			// select current bit in bitmap
			int bit = (source[dx / 8] << (dx % 8)) & 0x80;

			uint8_t* dstPixel = dstLine + dx*bpp;
			if (bit != 0)
				colorPutter(dstPixel, color.r, color.g, color.b);
		}
	}
	posX += (int)size + 1;
}

void CBitmapHanFont::renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	int posX = pos.x;
	int posY = pos.y;

	SDL_LockSurface(surface);

	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		std::string localChar = TextOperations::fromUnicode(data.substr(i, TextOperations::getUnicodeCharacterSize(data[i])));

		if (localChar.size() == 1)
			fallback->renderCharacter(surface, fallback->chars[ui8(localChar[0])], color, posX, posY);

		if (localChar.size() == 2)
			renderCharacter(surface, (int)getCharacterIndex(localChar[0], localChar[1]), color, posX, posY);
	}
	SDL_UnlockSurface(surface);
}

CBitmapHanFont::CBitmapHanFont(const JsonNode &config):
	fallback(new CBitmapFont(config["fallback"].String())),
	data(CResourceHandler::get()->load(ResourceID("data/" + config["name"].String(), EResType::OTHER))->readAll()),
	size((size_t)config["size"].Float())
{
	// basic tests to make sure that fonts are OK
	// 1) fonts must contain 190 "sections", 126 symbols each.
	assert(getCharacterIndex(0xfe, 0xff) == 190*126);
	// 2) ensure that font size is correct - enough to fit all possible symbols
	assert(getCharacterDataOffset(getCharacterIndex(0xfe, 0xff)) == data.second);
}

size_t CBitmapHanFont::getLineHeight() const
{
	return std::max(size + 1, fallback->getLineHeight());
}

size_t CBitmapHanFont::getGlyphWidth(const char * data) const
{
	std::string localChar = TextOperations::fromUnicode(std::string(data, TextOperations::getUnicodeCharacterSize(data[0])));

	if (localChar.size() == 1)
		return fallback->getGlyphWidth(data);

	if (localChar.size() == 2)
		return size + 1;

	return 0;
}
