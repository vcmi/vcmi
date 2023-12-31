/*
 * CTrueTypeFont.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IFont.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CBitmapFont;

using TTF_Font = struct _TTF_Font;

class CTrueTypeFont : public IFont
{
	std::unique_ptr<CBitmapFont> fallbackFont;
	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::unique_ptr<TTF_Font, void (*)(TTF_Font*)> font;
	const bool blended;
	const bool dropShadow;

	std::pair<std::unique_ptr<ui8[]>, ui64> loadData(const JsonNode & config);
	TTF_Font * loadFont(const JsonNode & config);
	int getFontStyle(const JsonNode & config);

	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
public:
	CTrueTypeFont(const JsonNode & fontConfig);
	~CTrueTypeFont();

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;
	size_t getStringWidth(const std::string & data) const override;
};
