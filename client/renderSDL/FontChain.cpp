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
#include "../../lib/texts/TextOperations.h"

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

void FontChain::addTrueTypeFont(const JsonNode & trueTypeConfig)
{
	chain.push_back(std::make_unique<CTrueTypeFont>(trueTypeConfig));
}

void FontChain::addBitmapFont(const std::string & bitmapFilename)
{
	if (settings["video"]["scalableFonts"].Bool())
		chain.push_back(std::make_unique<CBitmapFont>(bitmapFilename));
	else
		chain.insert(chain.begin(), std::make_unique<CBitmapFont>(bitmapFilename));
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
	std::vector<TextChunk> chunks;

	for (size_t i = 0; i < data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		const IFont * currentFont = nullptr;
		for(const auto & font : chain)
		{
			if (font->canRepresentCharacter(data.data() + i))
			{
				currentFont = font.get();
				break;
			}
		}

		if (currentFont == nullptr)
			continue; // not representable

		std::string symbol = data.substr(i, TextOperations::getUnicodeCharacterSize(data[i]));

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
