#include "StdInc.h"
#include "Fonts.h"

#include <SDL_ttf.h>

#include "SDL_Pixels.h"
#include "../../lib/JsonNode.h"
#include "../../lib/vcmi_endian.h"
#include "../../lib/filesystem/CResourceLoader.h"

/*
 * Fonts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


void IFont::renderTextLeft(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	renderText(surface, data, color, pos);
}

void IFont::renderTextRight(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	Point size(getStringWidth(data), getLineHeight());
	renderText(surface, data, color, pos - size);
}

void IFont::renderTextCenter(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	Point size(getStringWidth(data), getLineHeight());
	renderText(surface, data, color, pos - size / 2);
}

void IFont::renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;

	BOOST_FOREACH(const std::string & line, data)
	{
		renderTextLeft(surface, line, color, currPos);
		currPos.y += getLineHeight();
	}
}

void IFont::renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= data.size() * getLineHeight();

	BOOST_FOREACH(const std::string & line, data)
	{
		renderTextRight(surface, line, color, currPos);
		currPos.y += getLineHeight();
	}
}

void IFont::renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= data.size() * getLineHeight()/2;

	BOOST_FOREACH(const std::string & line, data)
	{
		renderTextCenter(surface, line, color, currPos);
		currPos.y += getLineHeight();
	}
}

std::array<CBitmapFont::Char, CBitmapFont::totalChars> CBitmapFont::loadChars() const
{
	std::array<Char, totalChars> ret;

	size_t offset = 32;

	for (size_t i=0; i< ret.size(); i++)
	{
		ret[i].leftOffset =  read_le_u32(data.first.get() + offset); offset+=4;
		ret[i].width =       read_le_u32(data.first.get() + offset); offset+=4;
		ret[i].rightOffset = read_le_u32(data.first.get() + offset); offset+=4;
	}

	for (size_t i=0; i< ret.size(); i++)
	{
		int pixelOffset =  read_le_u32(data.first.get() + offset); offset+=4;
		ret[i].pixels = data.first.get() + 4128 + pixelOffset;

		assert(pixelOffset + 4128 < data.second);
	}
	return ret;
}

CBitmapFont::CBitmapFont(const std::string & filename):
    data(CResourceHandler::get()->loadData(ResourceID("data/" + filename, EResType::BMP_FONT))),
    chars(loadChars()),
    height(data.first.get()[5])
{}

size_t CBitmapFont::getLineHeight() const
{
	return height;
}

size_t CBitmapFont::getSymbolWidth(char data) const
{
	const Char & ch = chars[ui8(data)];
	return ch.leftOffset + ch.width + ch.rightOffset;
}

size_t CBitmapFont::getStringWidth(const std::string & data) const
{
	size_t width = 0;

	BOOST_FOREACH(auto & ch, data)
	{
		width += getSymbolWidth(ch);
	}
	return width;
}

void CBitmapFont::renderCharacter(SDL_Surface * surface, const Char & character, const SDL_Color & color, int &posX, int &posY) const
{
	Rect clipRect;
	SDL_GetClipRect(surface, &clipRect);

	posX += character.leftOffset;

	TColorPutter colorPutter = CSDL_Ext::getPutterFor(surface, 0);

	Uint8 bpp = surface->format->BytesPerPixel;

	// start of line, may differ from 0 due to end of surface or clipped surface
	int lineBegin = std::max<int>(0, clipRect.y - posY);
	int lineEnd   = std::min<int>(height, clipRect.y + clipRect.h - posY - 1);

	// start end end of each row, may differ from 0
	int rowBegin = std::max<int>(0, clipRect.x - posX);
	int rowEnd   = std::min<int>(character.width, clipRect.x + clipRect.w - posX - 1);

	//for each line in symbol
	for(int dy = lineBegin; dy <lineEnd; dy++)
	{
		Uint8 *dstLine = (Uint8*)surface->pixels;
		Uint8 *srcLine = character.pixels;

		// shift source\destination pixels to current position
		dstLine += (posY+dy) * surface->pitch + posX * bpp;
		srcLine += dy * character.width;

		//for each column in line
		for(int dx = rowBegin; dx < rowEnd; dx++)
		{
			Uint8* dstPixel = dstLine + dx*bpp;
			switch(srcLine[dx])
			{
			case 1: //black "shadow"
				std::fill(dstPixel, dstPixel + bpp, 0);
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
	// for each symbol
	for(size_t index = 0; index < data.size(); index++)
	{
		renderCharacter(surface, chars[ui8(data[index])], color, posX, posY);
	}
	SDL_UnlockSurface(surface);
}

std::pair<std::unique_ptr<ui8[]>, ui64> CTrueTypeFont::loadData(const JsonNode & config)
{
	std::string filename = "Data/" + config["file"].String();
	return CResourceHandler::get()->loadData(ResourceID(filename, EResType::TTF_FONT));
}

TTF_Font * CTrueTypeFont::loadFont(const JsonNode &config)
{
	int pointSize = config["size"].Float();

	if(!TTF_WasInit() && TTF_Init()==-1)
		throw std::runtime_error(std::string("Failed to initialize true type support: ") + TTF_GetError() + "\n");

	return TTF_OpenFontRW(SDL_RWFromConstMem(data.first.get(), data.second), 1, pointSize);
}

int CTrueTypeFont::getFontStyle(const JsonNode &config)
{
	const JsonVector & names = config["style"].Vector();
	int ret = 0;
	BOOST_FOREACH(const JsonNode & node, names)
	{
		if (node.String() == "bold")
			ret |= TTF_STYLE_BOLD;
		else if (node.String() == "italic")
			ret |= TTF_STYLE_ITALIC;
	}
	return ret;
}

CTrueTypeFont::CTrueTypeFont(const JsonNode & fontConfig):
    data(loadData(fontConfig)),
    font(loadFont(fontConfig), TTF_CloseFont),
    blended(fontConfig["blend"].Bool())
{
	assert(font);

	TTF_SetFontStyle(font.get(), getFontStyle(fontConfig));
}

size_t CTrueTypeFont::getLineHeight() const
{
	return TTF_FontHeight(font.get());
}

size_t CTrueTypeFont::getSymbolWidth(char data) const
{
	int advance;
	TTF_GlyphMetrics(font.get(), data, nullptr, nullptr, nullptr, nullptr, &advance);
	return advance;
}

size_t CTrueTypeFont::getStringWidth(const std::string & data) const
{
	int width;
	TTF_SizeText(font.get(), data.c_str(), &width, nullptr);
	return width;
}

void CTrueTypeFont::renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	if (color.r != 0 && color.g != 0 && color.b != 0) // not black - add shadow
	{
		SDL_Color black = { 0, 0, 0, SDL_ALPHA_OPAQUE};
		renderText(surface, data, black, Point(pos.x + 1, pos.y + 1));
	}

	if (!data.empty())
	{
		SDL_Surface * rendered;
		if (blended)
			rendered = TTF_RenderText_Blended(font.get(), data.c_str(), color);
		else
			rendered = TTF_RenderText_Solid(font.get(), data.c_str(), color);

		assert(rendered);

		Rect rect(pos.x, pos.y, rendered->w, rendered->h);
		SDL_BlitSurface(rendered, nullptr, surface, &rect);
		SDL_FreeSurface(rendered);
	}
}
