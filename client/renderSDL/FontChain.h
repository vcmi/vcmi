/*
 * FontChain.h, part of VCMI engine
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

class FontChain final : public IFont
{
	struct TextChunk
	{
		const IFont * font;
		std::string text;
	};

	std::vector<TextChunk> splitTextToChunks(const std::string & data) const;

	std::vector<std::unique_ptr<IFont>> chain;

	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
	size_t getFontAscentScaled() const override;
	bool bitmapFontsPrioritized(const std::string & bitmapFontName) const;
public:
	FontChain() = default;

	void addTrueTypeFont(const JsonNode & trueTypeConfig, bool begin);
	void addBitmapFont(const std::string & bitmapFilename);

	size_t getLineHeightScaled() const override;
	size_t getGlyphWidthScaled(const char * data) const override;
	size_t getStringWidthScaled(const std::string & data) const override;
	bool canRepresentCharacter(const char * data) const override;
	void setFontStyle(const FontStyle style) const override;
};
