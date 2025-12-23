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

#include "../GameEngine.h"

#include "../render/IScreenHandler.h"

#include "../../lib/Point.h"
#include "../../lib/texts/TextOperations.h"

int IFont::getScalingFactor() const
{
	return ENGINE->screenHandler().getScalingFactor();
}

size_t IFont::getLineHeight() const
{
	return getLineHeightScaled() / getScalingFactor();
}

size_t IFont::getGlyphWidth(const char * data) const
{
	return getGlyphWidthScaled(data) / getScalingFactor();
}

size_t IFont::getStringWidth(const std::string & data) const
{
	return getStringWidthScaled(data) / getScalingFactor();
}

size_t IFont::getFontAscent() const
{
	return getFontAscentScaled() / getScalingFactor();
}

size_t IFont::getStringWidthScaled(const std::string & data) const
{
	size_t width = 0;

	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		width += getGlyphWidthScaled(data.data() + i);
	}
	return width;
}

void IFont::renderTextLeft(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	renderText(surface, data, color, pos);
}

void IFont::renderTextRight(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	Point size = Point(getStringWidth(data), getLineHeight()) * getScalingFactor();
	renderText(surface, data, color, pos - size);
}

void IFont::renderTextCenter(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	Point size = Point(getStringWidth(data), getLineHeight()) * getScalingFactor();
	renderText(surface, data, color, pos - size / 2);
}

void IFont::renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const
{
	Point currPos = pos;

	for(const std::string & line : data)
	{
		renderTextLeft(surface, line, color, currPos);
		currPos.y += getLineHeight() * getScalingFactor();
	}
}

void IFont::renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= data.size() * getLineHeight() * getScalingFactor();

	for(const std::string & line : data)
	{
		renderTextRight(surface, line, color, currPos);
		currPos.y += getLineHeight() * getScalingFactor();
	}
}

void IFont::renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const
{
	Point currPos = pos;
	currPos.y -= data.size() * getLineHeight() / 2 * getScalingFactor();

	for(const std::string & line : data)
	{
		renderTextCenter(surface, line, color, currPos);
		currPos.y += getLineHeight() * getScalingFactor();
	}
}

