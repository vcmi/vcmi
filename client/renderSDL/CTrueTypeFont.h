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

#include <SDL_ttf.h>

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CBitmapFont;

class CTrueTypeFont final : public IFont
{
	const std::pair<std::unique_ptr<ui8[]>, ui64> data;

	const std::unique_ptr<TTF_Font, void (*)(TTF_Font*)> font;
	const bool blended;
	const bool outline;
	const bool dropShadow;

	std::pair<std::unique_ptr<ui8[]>, ui64> loadData(const JsonNode & config);
	TTF_Font * loadFont(const JsonNode & config);
	int getPointSize(const JsonNode & config) const;
	int getFontStyle(const JsonNode & config) const;

	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
	size_t getFontAscentScaled() const override;
public:
	CTrueTypeFont(const JsonNode & fontConfig);
	~CTrueTypeFont();

	bool canRepresentCharacter(const char * data) const override;

	size_t getLineHeightScaled() const override;
	size_t getGlyphWidthScaled(const char * data) const override;
	size_t getStringWidthScaled(const std::string & data) const override;
};
