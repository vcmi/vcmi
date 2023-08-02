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
#include "../CGameInfo.h"
#include "../render/Colors.h"

#include "../../lib/Languages.h"
#include "../../lib/Rect.h"
#include "../../lib/TextOperations.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/vcmi_endian.h"
#include "../../lib/VCMI_Lib.h"

#include <SDL_surface.h>

void CBitmapFont::loadModFont(const std::string & modName, const ResourceID & resource)
{
	auto data = CResourceHandler::get(modName)->load(resource)->readAll();
	std::string modLanguage = CGI->modh->getModLanguage(modName);
	std::string modEncoding = Languages::getLanguageOptions(modLanguage).encoding;

	uint32_t dataHeight = data.first[5];

	maxHeight = std::max(maxHeight, dataHeight);

	constexpr size_t symbolsInFile = 0x100;
	constexpr size_t baseIndex = 32;
	constexpr size_t offsetIndex = baseIndex + symbolsInFile*12;
	constexpr size_t dataIndex = offsetIndex + symbolsInFile*4;

	for (uint32_t charIndex = 0; charIndex < symbolsInFile; ++charIndex)
	{
		CodePoint codepoint = TextOperations::getUnicodeCodepoint(static_cast<char>(charIndex), modEncoding);

		BitmapChar symbol;

		symbol.leftOffset =  read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 0);
		symbol.width =       read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 4);
		symbol.rightOffset = read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 8);
		symbol.height = dataHeight;

		uint32_t pixelDataOffset = read_le_u32(data.first.get() + offsetIndex + charIndex * 4);
		uint32_t pixelsCount = dataHeight * symbol.width;

		symbol.pixels.resize(pixelsCount);

		uint8_t * pixelData = data.first.get() + dataIndex + pixelDataOffset;

		std::copy_n(pixelData, pixelsCount, symbol.pixels.data() );

		chars[codepoint] = symbol;
	}
}

CBitmapFont::CBitmapFont(const std::string & filename):
	maxHeight(0)
{
	ResourceID resource("data/" + filename, EResType::BMP_FONT);

	loadModFont("core", resource);

	for(const auto & modName : VLC->modh->getActiveMods())
	{
		if (CResourceHandler::get(modName)->existsResource(resource))
			loadModFont(modName, resource);
	}
}

size_t CBitmapFont::getLineHeight() const
{
	return maxHeight;
}

size_t CBitmapFont::getGlyphWidth(const char * data) const
{
	CodePoint localChar = TextOperations::getUnicodeCodepoint(data, 4);

	auto iter = chars.find(localChar);

	if (iter == chars.end())
		return 0;

	return iter->second.leftOffset + iter->second.width + iter->second.rightOffset;
}

bool CBitmapFont::canRepresentCharacter(const char *data) const
{
	CodePoint localChar = TextOperations::getUnicodeCodepoint(data, 4);

	auto iter = chars.find(localChar);

	return iter != chars.end();
}

bool CBitmapFont::canRepresentString(const std::string & data) const
{
	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
		if (!canRepresentCharacter(data.data() + i))
			return false;

	return true;
}

void CBitmapFont::renderCharacter(SDL_Surface * surface, const BitmapChar & character, const ColorRGBA & color, int &posX, int &posY) const
{
	Rect clipRect;
	CSDL_Ext::getClipRect(surface, clipRect);

	posX += character.leftOffset;

	CSDL_Ext::TColorPutter colorPutter = CSDL_Ext::getPutterFor(surface, 0);

	uint8_t bpp = surface->format->BytesPerPixel;

	// start of line, may differ from 0 due to end of surface or clipped surface
	int lineBegin = std::max<int>(0, clipRect.y - posY);
	int lineEnd   = std::min<int>(character.height, clipRect.y + clipRect.h - posY - 1);

	// start end end of each row, may differ from 0
	int rowBegin = std::max<int>(0, clipRect.x - posX);
	int rowEnd   = std::min<int>(character.width, clipRect.x + clipRect.w - posX - 1);

	//for each line in symbol
	for(int dy = lineBegin; dy <lineEnd; dy++)
	{
		uint8_t *dstLine = (uint8_t*)surface->pixels;
		const uint8_t *srcLine = character.pixels.data();

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

void CBitmapFont::renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
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
		CodePoint codepoint = TextOperations::getUnicodeCodepoint(data.data() + i, data.size() - i);

		auto iter = chars.find(codepoint);

		if (iter != chars.end())
			renderCharacter(surface, iter->second, color, posX, posY);
	}
	SDL_UnlockSurface(surface);
}

