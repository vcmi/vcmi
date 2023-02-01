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
class Point;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;

typedef struct _TTF_Font TTF_Font;

class CBitmapFont;
class CBitmapHanFont;

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
