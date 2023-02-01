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
VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;

class IFont
{
protected:
	/// Internal function to render font, see renderTextLeft
	virtual void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const = 0;

public:
	virtual ~IFont()
	{}

	/// Returns height of font
	virtual size_t getLineHeight() const = 0;
	/// Returns width, in pixels of a character glyph. Pointer must contain at least characterSize valid bytes
	virtual size_t getGlyphWidth(const char * data) const = 0;
	/// Return width of the string
	virtual size_t getStringWidth(const std::string & data) const;

	/**
	 * @param surface - destination to print text on
	 * @param data - string to print
	 * @param color - font color
	 * @param pos - position of rendered font
	 */
	/// pos = topleft corner of the text
	void renderTextLeft(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const;
	/// pos = center of the text
	void renderTextRight(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const;
	/// pos = bottomright corner of the text
	void renderTextCenter(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const;

	/// pos = topleft corner of the text
	void renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
	/// pos = center of the text
	void renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
	/// pos = bottomright corner of the text
	void renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
};

