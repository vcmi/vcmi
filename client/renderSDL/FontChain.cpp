/*
 * FontChain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "FontChain.h"

#include "CTrueTypeFont.h"
#include "CBitmapFont.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/GameLibrary.h"

void FontChain::renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	auto chunks = splitTextToChunks(data);
	int maxAscent = getFontAscentScaled();
	Point currentPos = pos;
	for (auto const & chunk : chunks)
	{
		Point chunkPos = currentPos;
		int currAscent = chunk.font->getFontAscentScaled();
		chunkPos.y += maxAscent - currAscent;
		chunk.font->renderText(surface, chunk.text, color, chunkPos);
		currentPos.x += chunk.font->getStringWidthScaled(chunk.text);
	}
}

size_t FontChain::getFontAscentScaled() const
{
	size_t maxHeight = 0;
	for(const auto & font : chain)
		maxHeight = std::max(maxHeight, font->getFontAscentScaled());
	return maxHeight;
}

bool FontChain::bitmapFontsPrioritized(const std::string & bitmapFontName) const
{
	const std::string & fontType = settings["video"]["fontsType"].String();
	if (fontType == "original")
		return true;
	if (fontType == "scalable")
		return false;

	// else - autoselection.

	if (getScalingFactor() != 1)
		return false; // If xbrz in use ttf/scalable fonts are preferred

	if (!vstd::isAlmostEqual(1.0, settings["video"]["fontScalingFactor"].Float()))
		return false; // If player requested non-100% scaling - use scalable fonts

	std::string gameLanguage = LIBRARY->generaltexth->getPreferredLanguage();
	std::string gameEncoding = Languages::getLanguageOptions(gameLanguage).encoding;
	std::string fontEncoding = LIBRARY->modh->findResourceEncoding(ResourcePath("data/" + bitmapFontName, EResType::BMP_FONT));

	// player uses language with different encoding than his bitmap fonts
	// for example, Polish language with English fonts or Chinese language which can't use H3 fonts at all
	// this may result in unintended mixing of ttf and bitmap fonts, which may have a bit different look
	// so in this case prefer ttf fonts that are likely to cover target language better than H3 fonts
	if (fontEncoding != gameEncoding)
		return false;

	return true; // else - use original bitmap fonts
}

void FontChain::addTrueTypeFont(const JsonNode & trueTypeConfig, bool begin)
{
	if(begin)
		chain.insert(chain.begin(), std::make_unique<CTrueTypeFont>(trueTypeConfig));
	else
		chain.push_back(std::make_unique<CTrueTypeFont>(trueTypeConfig));
}

void FontChain::addBitmapFont(const std::string & bitmapFilename)
{
	if (bitmapFontsPrioritized(bitmapFilename))
		chain.insert(chain.begin(), std::make_unique<CBitmapFont>(bitmapFilename));
	else
		chain.push_back(std::make_unique<CBitmapFont>(bitmapFilename));
}

bool FontChain::canRepresentCharacter(const char * data) const
{
	for(const auto & font : chain)
		if (font->canRepresentCharacter(data))
			return true;
	return false;
}

size_t FontChain::getLineHeightScaled() const
{
	size_t maxHeight = 0;
	for(const auto & font : chain)
		maxHeight = std::max(maxHeight, font->getLineHeightScaled());
	return maxHeight;
}

size_t FontChain::getGlyphWidthScaled(const char * data) const
{
	for(const auto & font : chain)
		if (font->canRepresentCharacter(data))
			return font->getGlyphWidthScaled(data);
	return 0;
}

std::vector<FontChain::TextChunk> FontChain::splitTextToChunks(const std::string & data) const
{
	// U+FFFD - replacement character (question mark in rhombus, '�')
	static const std::string replacementCharacter = reinterpret_cast<const char *>(u8"\ufffd");

	std::vector<TextChunk> chunks;

	const auto & selectFont = [this](const char * characterPtr) -> const IFont *
	{
		for(const auto & font : chain)
			if (font->canRepresentCharacter(characterPtr))
				return font.get();
		return nullptr;
	};

	for (size_t i = 0; i < data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		std::string symbol = data.substr(i, TextOperations::getUnicodeCharacterSize(data[i]));
		const IFont * currentFont = selectFont(symbol.data());

		if (currentFont == nullptr)
		{
			symbol = replacementCharacter;
			currentFont = selectFont(symbol.data());
		}

		if (currentFont == nullptr)
			continue; // Still nothing - neither desired character nor fallback can be rendered

		if (chunks.empty() || chunks.back().font != currentFont)
			chunks.push_back({currentFont, symbol});
		else
			chunks.back().text += symbol;
	}

	return chunks;
}

size_t FontChain::getStringWidthScaled(const std::string & data) const
{
	size_t result = 0;
	auto chunks = splitTextToChunks(data);
	for (auto const & chunk : chunks)
		result += chunk.font->getStringWidthScaled(chunk.text);

	return result;
}
