/*
 * Fonts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

struct Point;
struct SDL_Surface;
struct SDL_Color;

typedef struct _TTF_Font TTF_Font;

class CBitmapFont;
class CBitmapHanFont;

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

class CBitmapFont : public IFont
{
	static const size_t totalChars = 256;

	struct BitmapChar
	{
		si32 leftOffset;
		ui32 width;
		si32 rightOffset;
		ui8 *pixels; // pixels of this character, part of BitmapFont::data
	};

	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::array<BitmapChar, totalChars> chars;
	const ui8 height;

	std::array<BitmapChar, totalChars> loadChars() const;

	void renderCharacter(SDL_Surface * surface, const BitmapChar & character, const SDL_Color & color, int &posX, int &posY) const;

	void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const override;
public:
	CBitmapFont(const std::string & filename);

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;

	friend class CBitmapHanFont;
};

/// supports multi-byte characters for such languages like Chinese
class CBitmapHanFont : public IFont
{
	std::unique_ptr<CBitmapFont> fallback;
	// data, directly copied from file
	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	// size of the font. Not available in file but needed for proper rendering
	const size_t size;

	size_t getCharacterDataOffset(size_t index) const;
	size_t getCharacterIndex(ui8 first, ui8 second) const;

	void renderCharacter(SDL_Surface * surface, int characterIndex, const SDL_Color & color, int &posX, int &posY) const;
	void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const override;
public:
	CBitmapHanFont(const JsonNode & config);

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;
};

class CTrueTypeFont : public IFont
{
	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::unique_ptr<TTF_Font, void (*)(TTF_Font*)> font;
	const bool blended;

	std::pair<std::unique_ptr<ui8[]>, ui64> loadData(const JsonNode & config);
	TTF_Font * loadFont(const JsonNode & config);
	int getFontStyle(const JsonNode & config);

	void renderText(SDL_Surface * surface, const std::string & data, const SDL_Color & color, const Point & pos) const override;
public:
	CTrueTypeFont(const JsonNode & fontConfig);

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;
	size_t getStringWidth(const std::string & data) const override;
};
