/*
 * CBitmapFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBitmapFont.h"

#include "SDL_Extensions.h"

#include "../../lib/vcmi_endian.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/TextOperations.h"
#include "../../lib/Rect.h"

#include <SDL_surface.h>

std::array<CBitmapFont::BitmapChar, CBitmapFont::totalChars> CBitmapFont::loadChars() const
{
	std::array<BitmapChar, totalChars> ret;

	size_t offset = 32;

	for (auto & elem : ret)
	{
		elem.leftOffset =  read_le_u32(data.first.get() + offset); offset+=4;
		elem.width =       read_le_u32(data.first.get() + offset); offset+=4;
		elem.rightOffset = read_le_u32(data.first.get() + offset); offset+=4;
	}

	for (auto & elem : ret)
	{
		int pixelOffset =  read_le_u32(data.first.get() + offset); offset+=4;
		elem.pixels = data.first.get() + 4128 + pixelOffset;

		assert(pixelOffset + 4128 < data.second);
	}
	return ret;
}

CBitmapFont::CBitmapFont(const std::string & filename):
	data(CResourceHandler::get()->load(ResourceID("data/" + filename, EResType::BMP_FONT))->readAll()),
	chars(loadChars()),
	height(data.first.get()[5])
{}

size_t CBitmapFont::getLineHeight() const
{
	return height;
}

size_t CBitmapFont::getGlyphWidth(const char * data) const
{
	std::string localChar = TextOperations::fromUnicode(std::string(data, TextOperations::getUnicodeCharacterSize(data[0])));

	if (localChar.size() == 1)
	{
		const BitmapChar & ch = chars[ui8(localChar[0])];
		return ch.leftOffset + ch.width + ch.rightOffset;
	}
	return 0;
}

void CBitmapFont::renderCharacter(SDL_Surface * surface, const BitmapChar & character, const SDL_Color & color, int &posX, int &posY) const
{
	Rect clipRect;
	CSDL_Ext::getClipRect(surface, clipRect);

	posX += character.leftOffset;

	CSDL_Ext::TColorPutter colorPutter = CSDL_Ext::getPutterFor(surface, 0);

	uint8_t bpp = surface->format->BytesPerPixel;

	// start of line, may differ from 0 due to end of surface or clipped surface
	int lineBegin = std::max<int>(0, clipRect.y - posY);
	int lineEnd   = std::min<int>(height, clipRect.y + clipRect.h - posY - 1);

	// start end end of each row, may differ from 0
	int rowBegin = std::max<int>(0, clipRect.x - posX);
	int rowEnd   = std::min<int>(character.width, clipRect.x + clipRect.w - posX - 1);

	//for each line in symbol
	for(int dy = lineBegin; dy <lineEnd; dy++)
	{
		uint8_t *dstLine = (uint8_t*)surface->pixels;
		uint8_t *srcLine = character.pixels;

		// shift source\destination pixels to current position
		dstLine += (posY+dy) * surface->pitch + posX * bpp;
		srcLine += dy * character.width;

		//for each column in line
		for(int dx = rowBegin; dx < rowEnd; dx++)
		{
			uint8_t* dstPixel = dstLine + dx*bpp;
			switch(srcLine[dx])
			{
			case 1: //black "shadow"
				colorPutter(dstPixel, 0, 0, 0);
				break;
			case 255: //text colour
				colorPutter(dstPixel, color.r, color.g, color.b);
				break;
			default :
				break; //transparency
			}
		}
	}
	posX += character.width;
	posX += character.rightOffset;
}

void CBitmapFont::renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	if (data.empty())
		return;

	assert(surface);

	int posX = pos.x;
	int posY = pos.y;

	// Should be used to detect incorrect text parsing. Disabled right now due to some old UI code (mostly pregame and battles)
	//assert(data[0] != '{');
	//assert(data[data.size()-1] != '}');

	SDL_LockSurface(surface);

	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		std::string localChar = TextOperations::fromUnicode(data.substr(i, TextOperations::getUnicodeCharacterSize(data[i])));

		if (localChar.size() == 1)
			renderCharacter(surface, chars[ui8(localChar[0])], color, posX, posY);
	}
	SDL_UnlockSurface(surface);
}

