/*
 * CBitmapHanFont.h, part of VCMI engine
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

	void renderCharacter(SDL_Surface * surface, int characterIndex, const ColorRGBA & color, int &posX, int &posY) const;
	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
public:
	CBitmapHanFont(const JsonNode & config);

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;
};
