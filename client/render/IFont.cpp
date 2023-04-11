/*
 * IFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "IFont.h"

#include "../../lib/Point.h"
#include "../../lib/TextOperations.h"

size_t IFont::getStringWidth(const std::string & data) const
{
	size_t width = 0;

	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		width += getGlyphWidth(data.data() + i);
	}
	return width;
}

void IFont::renderTextLeft(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	renderText(surface, data, color, pos);
}

void IFont::renderTextRight(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	Point size((int)getStringWidth(data), (int)getLineHeight());
	renderText(surface, data, color, pos - size);
}

void IFont::renderTextCenter(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const
{
	Point size((int)getStringWidth(data), (int)getLineHeight());
	renderText(surface, data, color, pos - size / 2);
}

void IFont::renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;

	for(const std::string & line : data)
	{
		renderTextLeft(surface, line, color, currPos);
		currPos.y += (int)getLineHeight();
	}
}

void IFont::renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= (int)data.size() * (int)getLineHeight();

	for(const std::string & line : data)
	{
		renderTextRight(surface, line, color, currPos);
		currPos.y += (int)getLineHeight();
	}
}

void IFont::renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= (int)data.size() * (int)getLineHeight() / 2;

	for(const std::string & line : data)
	{
		renderTextCenter(surface, line, color, currPos);
		currPos.y += (int)getLineHeight();
	}
}

