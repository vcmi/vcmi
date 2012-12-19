#pragma once

/*
 * Fonts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class JsonNode;

struct Point;
struct SDL_Surface;
struct SDL_Color;

typedef struct _TTF_Font TTF_Font;

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
	/// Returns width of a single symbol
	virtual size_t getSymbolWidth(char data) const = 0;
	/// Return width of the string
	virtual size_t getStringWidth(const std::string & data) const = 0;

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

	/**
	 * @param maxWidth -  max width in pixels of one line
	 */
	/// pos = topleft corner of the text
	void renderTextLinesLeft(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
	/// pos = center of the text
	void renderTextLinesRight(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
	/// pos = bottomright corner of the text
	void renderTextLinesCenter(SDL_Surface * surface, const std::vector<std::string> & data, const SDL_Color & color, const Point & pos) const;
};

class CBitmapFont : public IFont
{
	static const size_t totalChars = 256;

	struct Char
	{
		si32 leftOffset;
		ui32 width;
		si32 rightOffset;
		ui8 *pixels; // pixels of this character, part of BitmapFont::data
	};

	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::array<Char, totalChars> chars;
	const ui8 height;

	std::array<Char, totalChars> loadChars() const;

	void renderCharacter(SDL_Surface * surface, const Char & character, const SDL_Color & color, int &posX, int &posY) const;

	void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const;
public:
	CBitmapFont(const std::string & filename);

	size_t getLineHeight() const;
	size_t getSymbolWidth(char data) const;
	size_t getStringWidth(const std::string & data) const;
};

class CTrueTypeFont : public IFont
{
	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::unique_ptr<TTF_Font, void (*)(TTF_Font*)> font;
	const bool blended;

	std::pair<std::unique_ptr<ui8[]>, ui64> loadData(const JsonNode & config);
	TTF_Font * loadFont(const JsonNode & config);
	int getFontStyle(const JsonNode & config);

	void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const;
public:
	CTrueTypeFont(const JsonNode & fontConfig);

	size_t getLineHeight() const;
	size_t getSymbolWidth(char data) const;
	size_t getStringWidth(const std::string & data) const;
};
