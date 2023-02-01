/*
 * CBitmapFont.h, part of VCMI engine
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
class Point;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;

typedef struct _TTF_Font TTF_Font;

class CBitmapFont;
class CBitmapHanFont;

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

