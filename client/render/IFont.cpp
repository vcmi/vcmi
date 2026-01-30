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
#include "Colors.h"
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

std::pair<std::optional<ColorRGBA>, std::optional<IFont::FontStyle>> IFont::parseColorAndFontStyle(const std::string & text)
{
	if(text.empty())
		return {std::nullopt, std::nullopt};

	// split by ';' — if no ';' then only color is parsed
	std::vector<std::string> parts;
	boost::split(parts, text, boost::is_any_of(";"));

	std::optional<ColorRGBA> color;
	std::optional<IFont::FontStyle> style;

	if(!parts.empty() && !parts[0].empty())
		color = Colors::parseColor(parts[0]);

	if(parts.size() > 1 && !parts[1].empty())
	{
		std::string s = boost::to_lower_copy(parts[1]);
		if(s == "bold")
			style = IFont::FontStyle::BOLD;
		else if(s == "italic")
			style = IFont::FontStyle::ITALIC;
		else if(s == "underscore" || s == "underline")
			style = IFont::FontStyle::UNDERLINE;
		else if(s == "strikethrough" || s == "strike")
			style = IFont::FontStyle::STRIKETHROUGH;
		else if(s == "normal")
			style = IFont::FontStyle::NORMAL;
		// unknown style -> leave std::nullopt
	}

	return {color, style};
}
