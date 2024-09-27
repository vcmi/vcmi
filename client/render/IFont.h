/*
 * IFont.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Point;
class ColorRGBA;
VCMI_LIB_NAMESPACE_END

struct SDL_Surface;

class IFont : boost::noncopyable
{
protected:

	int getScalingFactor() const;

public:
	virtual ~IFont()
	{}

	/// Returns height of font
	virtual size_t getLineHeightScaled() const = 0;
	/// Returns width, in pixels of a character glyph. Pointer must contain at least characterSize valid bytes
	virtual size_t getGlyphWidthScaled(const char * data) const = 0;
	/// Return width of the string
	virtual size_t getStringWidthScaled(const std::string & data) const;
	/// Returns distance from top of the font glyphs to baseline
	virtual size_t getFontAscentScaled() const = 0;

	/// Returns height of font
	size_t getLineHeight() const;
	/// Returns width, in pixels of a character glyph. Pointer must contain at least characterSize valid bytes
	size_t getGlyphWidth(const char * data) const;
	/// Return width of the string
	size_t getStringWidth(const std::string & data) const;
	/// Returns distance from top of the font glyphs to baseline
	size_t getFontAscent() const;

	/// Internal function to render font, see renderTextLeft
	virtual void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const = 0;

	virtual bool canRepresentCharacter(const char * data) const	= 0;

	/**
	 * @param surface - destination to print text on
	 * @param data - string to print
	 * @param color - font color
	 * @param pos - position of rendered font
	 */
	/// pos = topleft corner of the text
	void renderTextLeft(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const;
	/// pos = center of the text
	void renderTextRight(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const;
	/// pos = bottomright corner of the text
	void renderTextCenter(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const;

	/// pos = topleft corner of the text
	void renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const;
	/// pos = center of the text
	void renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const;
	/// pos = bottomright corner of the text
	void renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const ColorRGBA & color, const Point & pos) const;
};

