/*
 * CBitmapFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBitmapFont.h"

#include "SDL_Extensions.h"
#include "SDLImageScaler.h"

#include "../GameEngine.h"
#include "../render/Colors.h"
#include "../render/IImage.h"
#include "../render/IScreenHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/Rect.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/vcmi_endian.h"

#include <SDL_surface.h>
#include <SDL_image.h>

struct AtlasLayout
{
	Point dimensions;
	std::map<int, Rect> images;
};

/// Attempts to pack provided list of images into 2d box of specified size
/// Returns resulting layout on success and empty optional on failure
static std::optional<AtlasLayout> tryAtlasPacking(Point dimensions, const std::map<int, Point> & images)
{
	// Simple atlas packing algorithm. Can be extended if needed, however optimal solution is NP-complete problem, so 'perfect' solution is too costly

	AtlasLayout result;
	result.dimensions = dimensions;

	// a little interval to prevent potential 'bleeding' into adjacent symbols
	// should be unnecessary for base game, but may be needed for upscaled filters
	constexpr int interval = 1;
	int currentHeight = 0;
	int nextHeight = 0;
	int currentWidth = 0;

	for (auto const & image : images)
	{
		int nextWidth = currentWidth + image.second.x + interval;

		if (nextWidth > dimensions.x)
		{
			currentHeight = nextHeight;
			currentWidth = 0;
			nextWidth = currentWidth + image.second.x + interval;
		}

		nextHeight = std::max(nextHeight, currentHeight + image.second.y + interval);
		if (nextHeight > dimensions.y)
			return std::nullopt; // failure - ran out of space

		result.images[image.first] = Rect(Point(currentWidth, currentHeight), image.second);

		currentWidth = nextWidth;
	}

	return result;
}

/// Arranges images to fit into texture atlas with automatic selection of image size
/// Returns images arranged into 2d box
static AtlasLayout doAtlasPacking(const std::map<int, Point> & images)
{
	// initial size of an atlas. Smaller size won't even fit tiniest H3 font
	Point dimensions(128, 128);

	for (;;)
	{
		auto result = tryAtlasPacking(dimensions, images);

		if (result)
			return *result;

		// else - packing failed. Increase atlas size and try again
		// increase width and height in alternating form: (64,64) -> (128,64) -> (128,128) ...
		if (dimensions.x > dimensions.y)
			dimensions.y *= 2;
		else
			dimensions.x *= 2;
	}
}

void CBitmapFont::loadFont(const ResourcePath & resource, std::unordered_map<CodePoint, EntryFNT> & loadedChars)
{
	auto data = CResourceHandler::get()->load(resource)->readAll();
	std::string modEncoding = LIBRARY->modh->findResourceEncoding(resource);

	height = data.first[5];

	constexpr size_t symbolsInFile = 0x100;
	constexpr size_t baseIndex = 32;
	constexpr size_t offsetIndex = baseIndex + symbolsInFile*12;
	constexpr size_t dataIndex = offsetIndex + symbolsInFile*4;

	for (uint32_t charIndex = 0; charIndex < symbolsInFile; ++charIndex)
	{
		CodePoint codepoint = TextOperations::getUnicodeCodepoint(static_cast<char>(charIndex), modEncoding);

		EntryFNT symbol;

		symbol.leftOffset =  read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 0);
		symbol.width =       read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 4);
		symbol.rightOffset = read_le_u32(data.first.get() + baseIndex + charIndex * 12 + 8);
		symbol.height = height;

		uint32_t pixelDataOffset = read_le_u32(data.first.get() + offsetIndex + charIndex * 4);
		uint32_t pixelsCount = height * symbol.width;

		symbol.pixels.resize(pixelsCount);

		uint8_t * pixelData = data.first.get() + dataIndex + pixelDataOffset;

		std::copy_n(pixelData, pixelsCount, symbol.pixels.data() );

		loadedChars[codepoint] = symbol;
	}

	// Try to use symbol 'L' to detect font 'ascent' - number of pixels above text baseline
	const auto & symbolL = loadedChars['L'];
	uint32_t lastNonEmptyRow = 0;
	for (uint32_t row = 0; row < symbolL.height; ++row)
	{
		for (uint32_t col = 0; col < symbolL.width; ++col)
			if (symbolL.pixels.at(row * symbolL.width + col) == 255)
				lastNonEmptyRow = std::max(lastNonEmptyRow, row);
	}

	ascent = lastNonEmptyRow + 1;
}

CBitmapFont::CBitmapFont(const std::string & filename):
	height(0)
{
	ResourcePath resource("data/" + filename, EResType::BMP_FONT);

	std::unordered_map<CodePoint, EntryFNT> loadedChars;
	loadFont(resource, loadedChars);

	std::map<int, Point> atlasSymbol;
	for (auto const & symbol : loadedChars)
		atlasSymbol[symbol.first] = Point(symbol.second.width, symbol.second.height);

	auto atlas = doAtlasPacking(atlasSymbol);

	atlasImage = SDL_CreateRGBSurface(0, atlas.dimensions.x, atlas.dimensions.y, 8, 0, 0, 0, 0);

	assert(atlasImage->format->palette != nullptr);
	assert(atlasImage->format->palette->ncolors == 256);

	atlasImage->format->palette->colors[0] = { 0, 255, 255, SDL_ALPHA_OPAQUE }; // transparency
	atlasImage->format->palette->colors[1] = { 0, 0, 0, SDL_ALPHA_OPAQUE }; // black shadow

	CSDL_Ext::fillSurface(atlasImage, CSDL_Ext::toSDL(Colors::CYAN));
	CSDL_Ext::setColorKey(atlasImage, CSDL_Ext::toSDL(Colors::CYAN));

	for (size_t i = 2; i < atlasImage->format->palette->ncolors; ++i)
		atlasImage->format->palette->colors[i] = { 255, 255, 255, SDL_ALPHA_OPAQUE };

	for (auto const	& symbol : loadedChars)
	{
		BitmapChar storedEntry;

		storedEntry.leftOffset = symbol.second.leftOffset;
		storedEntry.rightOffset = symbol.second.rightOffset;
		storedEntry.positionInAtlas = atlas.images.at(symbol.first);

		// Copy pixel data to atlas
		uint8_t *dstPixels = static_cast<uint8_t*>(atlasImage->pixels);
		uint8_t *dstLine   = dstPixels + storedEntry.positionInAtlas.y * atlasImage->pitch;
		uint8_t *dst = dstLine + storedEntry.positionInAtlas.x;

		for (size_t i = 0; i < storedEntry.positionInAtlas.h; ++i)
		{
			const uint8_t *srcPtr = symbol.second.pixels.data() + i * storedEntry.positionInAtlas.w;
			uint8_t * dstPtr = dst + i * atlasImage->pitch;

			std::copy_n(srcPtr, storedEntry.positionInAtlas.w, dstPtr);
		}

		chars[symbol.first] = storedEntry;
	}

	if (ENGINE->screenHandler().getScalingFactor() != 1)
	{
		static const std::map<std::string, EScalingAlgorithm> filterNameToEnum = {
			{ "nearest", EScalingAlgorithm::NEAREST},
			{ "bilinear", EScalingAlgorithm::BILINEAR},
			{ "xbrz", EScalingAlgorithm::XBRZ_ALPHA}
		};

		auto filterName = settings["video"]["fontUpscalingFilter"].String();
		EScalingAlgorithm algorithm = filterNameToEnum.at(filterName);
		SDLImageScaler scaler(atlasImage);
		scaler.scaleSurfaceIntegerFactor(ENGINE->screenHandler().getScalingFactor(), algorithm);
		SDL_FreeSurface(atlasImage);
		atlasImage = scaler.acquireResultSurface();
	}

	logGlobal->debug("Loaded BMP font: '%s', height %d, ascent %d",
					 filename,
					 getLineHeightScaled(),
					 getFontAscentScaled()
					 );
}

CBitmapFont::~CBitmapFont()
{
	SDL_FreeSurface(atlasImage);
}

size_t CBitmapFont::getLineHeightScaled() const
{
	return height * getScalingFactor();
}

size_t CBitmapFont::getGlyphWidthScaled(const char * data) const
{
	CodePoint localChar = TextOperations::getUnicodeCodepoint(data, 4);

	auto iter = chars.find(localChar);

	if (iter == chars.end())
		return 0;

	return (iter->second.leftOffset + iter->second.positionInAtlas.w + iter->second.rightOffset) * getScalingFactor();
}

size_t CBitmapFont::getFontAscentScaled() const
{
	return ascent * getScalingFactor();
}

bool CBitmapFont::canRepresentCharacter(const char *data) const
{
	CodePoint localChar = TextOperations::getUnicodeCodepoint(data, 4);

	auto iter = chars.find(localChar);

	return iter != chars.end();
}

bool CBitmapFont::canRepresentString(const std::string & data) const
{
	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
		if (!canRepresentCharacter(data.data() + i))
			return false;

	return true;
}

void CBitmapFont::renderCharacter(SDL_Surface * surface, const BitmapChar & character, const ColorRGBA & color, int &posX, int &posY) const
{
	int scalingFactor = ENGINE->screenHandler().getScalingFactor();

	posX += character.leftOffset * scalingFactor;

	auto sdlColor = CSDL_Ext::toSDL(color);

	if (atlasImage->format->palette)
		SDL_SetPaletteColors(atlasImage->format->palette, &sdlColor, 255, 1);
	else
		SDL_SetSurfaceColorMod(atlasImage, color.r, color.g, color.b);

	CSDL_Ext::blitSurface(atlasImage, character.positionInAtlas * scalingFactor, surface, Point(posX, posY));

	posX += character.positionInAtlas.w * scalingFactor;
	posX += character.rightOffset * scalingFactor;
}

void CBitmapFont::renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	if (data.empty())
		return;

	assert(surface);

	int posX = pos.x;
	int posY = pos.y;

	for(size_t i=0; i<data.size(); i += TextOperations::getUnicodeCharacterSize(data[i]))
	{
		CodePoint codepoint = TextOperations::getUnicodeCodepoint(data.data() + i, data.size() - i);

		auto iter = chars.find(codepoint);

		if (iter != chars.end())
			renderCharacter(surface, iter->second, color, posX, posY);
	}
}

