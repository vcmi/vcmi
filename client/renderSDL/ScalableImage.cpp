/*
 * ScalableImage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScalableImage.h"

#include "SDLImage.h"
#include "SDL_Extensions.h"

#include "../GameEngine.h"

#include "../render/ColorFilter.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"
#include "../render/CanvasImage.h"

#include "../../lib/constants/EntityIdentifiers.h"

#include <SDL_surface.h>

//First 8 colors in def palette used for transparency
static constexpr std::array<SDL_Color, 8> sourcePalette = {{
	{0,   255, 255, SDL_ALPHA_OPAQUE},
	{255, 150, 255, SDL_ALPHA_OPAQUE},
	{255, 100, 255, SDL_ALPHA_OPAQUE},
	{255, 50,  255, SDL_ALPHA_OPAQUE},
	{255, 0,   255, SDL_ALPHA_OPAQUE},
	{255, 255, 0,   SDL_ALPHA_OPAQUE},
	{180, 0,   255, SDL_ALPHA_OPAQUE},
	{0,   255, 0,   SDL_ALPHA_OPAQUE}
}};

static constexpr std::array<ColorRGBA, 8> targetPalette = {{
	{0, 0, 0, 0  }, // 0 - transparency                  ( used in most images )
	{0, 0, 0, 64 }, // 1 - shadow border                 ( used in battle, adventure map def's )
	{0, 0, 0, 64 }, // 2 - shadow border                 ( used in fog-of-war def's )
	{0, 0, 0, 128}, // 3 - shadow body                   ( used in fog-of-war def's )
	{0, 0, 0, 128}, // 4 - shadow body                   ( used in battle, adventure map def's )
	{0, 0, 0, 0  }, // 5 - selection / owner flag        ( used in battle, adventure map def's )
	{0, 0, 0, 128}, // 6 - shadow body   below selection ( used in battle def's )
	{0, 0, 0, 64 }  // 7 - shadow border below selection ( used in battle def's )
}};

static ui8 mixChannels(ui8 c1, ui8 c2, ui8 a1, ui8 a2)
{
	return c1*a1 / 256 + c2*a2*(255 - a1) / 256 / 256;
}

static ColorRGBA addColors(const ColorRGBA & base, const ColorRGBA & over)
{
	return ColorRGBA(
		mixChannels(over.r, base.r, over.a, base.a),
		mixChannels(over.g, base.g, over.a, base.a),
		mixChannels(over.b, base.b, over.a, base.a),
		static_cast<ui8>(over.a + base.a * (255 - over.a) / 256)
		);
}
static bool colorsSimilar (const SDL_Color & lhs, const SDL_Color & rhs)
{
	// it seems that H3 does not requires exact match to replace colors -> (255, 103, 255) gets interpreted as shadow
	// exact logic is not clear and requires extensive testing with image editing
	// potential reason is that H3 uses 16-bit color format (565 RGB bits), meaning that 3 least significant bits are lost in red and blue component
	static const int threshold = 8;

	int diffR = static_cast<int>(lhs.r) - rhs.r;
	int diffG = static_cast<int>(lhs.g) - rhs.g;
	int diffB = static_cast<int>(lhs.b) - rhs.b;
	int diffA = static_cast<int>(lhs.a) - rhs.a;

	return std::abs(diffR) < threshold && std::abs(diffG) < threshold && std::abs(diffB) < threshold && std::abs(diffA) < threshold;
}

ScalableImageParameters::ScalableImageParameters(const SDL_Palette * originalPalette, EImageBlitMode blitMode)
{
	if (originalPalette)
	{
		palette = SDL_AllocPalette(256);
		SDL_SetPaletteColors(palette, originalPalette->colors, 0, originalPalette->ncolors);
		preparePalette(originalPalette, blitMode);
	}
}

ScalableImageParameters::~ScalableImageParameters()
{
	if (palette)
		SDL_FreePalette(palette);
}

void ScalableImageParameters::preparePalette(const SDL_Palette * originalPalette, EImageBlitMode blitMode)
{
	switch(blitMode)
	{
		case EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR:
		case EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION:
		case EImageBlitMode::ONLY_FLAG_COLOR:
		case EImageBlitMode::ONLY_SELECTION:
			adjustPalette(originalPalette, blitMode, ColorFilter::genAlphaShifter(0), 0);
			break;
		case EImageBlitMode::GRAYSCALE_BODY_HIDE_SELECTION:
			adjustPalette(originalPalette, blitMode, ColorFilter::genMuxerShifter( { 0.299, 0.587, 0.114, 0}, { 0.299, 0.587, 0.114, 0}, { 0.299, 0.587, 0.114, 0}, 1), 0);
			break;

	}

	switch(blitMode)
	{
		case EImageBlitMode::SIMPLE:
		case EImageBlitMode::WITH_SHADOW:
		case EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR:
		case EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION:
		case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
		case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
			setShadowTransparency(originalPalette, 1.0);
			break;
		case EImageBlitMode::ONLY_BODY_HIDE_SELECTION:
		case EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR:
		case EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY:
		case EImageBlitMode::ONLY_FLAG_COLOR:
		case EImageBlitMode::ONLY_SELECTION:
		case EImageBlitMode::GRAYSCALE_BODY_HIDE_SELECTION:
			setShadowTransparency(originalPalette, 0.0);
			break;
	}

	switch(blitMode)
	{
		case EImageBlitMode::ONLY_FLAG_COLOR:
		case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
			setOverlayColor(originalPalette, Colors::WHITE_TRUE, false);
			break;
		case EImageBlitMode::ONLY_SELECTION:
		case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
			setOverlayColor(originalPalette, Colors::WHITE_TRUE, true);
			break;
		case EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR:
		case EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR:
			setOverlayColor(originalPalette, Colors::TRANSPARENCY, false);
			break;
		case EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION:
		case EImageBlitMode::ONLY_BODY_HIDE_SELECTION:
		case EImageBlitMode::GRAYSCALE_BODY_HIDE_SELECTION:
			setOverlayColor(originalPalette, Colors::TRANSPARENCY, true);
			break;
	}
}

void ScalableImageParameters::setOverlayColor(const SDL_Palette * originalPalette, const ColorRGBA & color, bool includeShadow)
{
	palette->colors[5] = CSDL_Ext::toSDL(addColors(targetPalette[5], color));

	if (includeShadow)
	{
		for (int i : {6,7})
			palette->colors[i] = CSDL_Ext::toSDL(addColors(targetPalette[i], color));
	}
}

void ScalableImageParameters::shiftPalette(const SDL_Palette * originalPalette, uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
	std::vector<SDL_Color> shifterColors(colorsToMove);

	for(uint32_t i=0; i<colorsToMove; ++i)
		shifterColors[(i+distanceToMove)%colorsToMove] = originalPalette->colors[firstColorID + i];

	SDL_SetPaletteColors(palette, shifterColors.data(), firstColorID, colorsToMove);
}

void ScalableImageParameters::setShadowTransparency(const SDL_Palette * originalPalette, float factor)
{
	ColorRGBA shadow50(0, 0, 0, 128 * factor);
	ColorRGBA shadow25(0, 0, 0,  64 * factor);

	std::array<SDL_Color, 5> colorsSDL = {
		originalPalette->colors[0],
		originalPalette->colors[1],
		originalPalette->colors[2],
		originalPalette->colors[3],
		originalPalette->colors[4]
	};

	// seems to be used unconditionally
	colorsSDL[0] = CSDL_Ext::toSDL(Colors::TRANSPARENCY);
	colorsSDL[1] = CSDL_Ext::toSDL(shadow25);
	colorsSDL[4] = CSDL_Ext::toSDL(shadow50);

	// seems to be used only if color matches
	if (colorsSimilar(originalPalette->colors[2], sourcePalette[2]))
		colorsSDL[2] = CSDL_Ext::toSDL(shadow25);

	if (colorsSimilar(originalPalette->colors[3], sourcePalette[3]))
		colorsSDL[3] = CSDL_Ext::toSDL(shadow50);

	SDL_SetPaletteColors(palette, colorsSDL.data(), 0, colorsSDL.size());
}

void ScalableImageParameters::adjustPalette(const SDL_Palette * originalPalette, EImageBlitMode blitMode, const ColorFilter & shifter, uint32_t colorsToSkipMask)
{
	// If shadow is enabled, following colors must be skipped unconditionally
	if (blitMode == EImageBlitMode::WITH_SHADOW || blitMode == EImageBlitMode::WITH_SHADOW_AND_SELECTION || blitMode == EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR)
		colorsToSkipMask |= (1 << 0) + (1 << 1) + (1 << 4);

	// Note: here we skip first colors in the palette that are predefined in H3 images
	for(int i = 0; i < palette->ncolors; i++)
	{
		if (i < std::size(sourcePalette) && colorsSimilar(sourcePalette[i], originalPalette->colors[i]))
			continue;

		if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
			continue;

		palette->colors[i] = CSDL_Ext::toSDL(shifter.shiftColor(CSDL_Ext::fromSDL(originalPalette->colors[i])));
	}
}

ScalableImageShared::ScalableImageShared(const SharedImageLocator & locator, const std::shared_ptr<const ISharedImage> & baseImage)
	:locator(locator)
{
	scaled[1].body[0] = baseImage;
	assert(scaled[1].body[0] != nullptr);

	loadScaledImages(ENGINE->screenHandler().getScalingFactor(), PlayerColor::CANNOT_DETERMINE);
}

Point ScalableImageShared::dimensions() const
{
	return scaled[1].body[0]->dimensions();
}

void ScalableImageShared::exportBitmap(const boost::filesystem::path & path, const ScalableImageParameters & parameters) const
{
	scaled[1].body[0]->exportBitmap(path, parameters.palette);
}

bool ScalableImageShared::isTransparent(const Point & coords) const
{
	return scaled[1].body[0]->isTransparent(coords);
}

Rect ScalableImageShared::contentRect() const
{
	return scaled[1].body[0]->contentRect();
}

void ScalableImageShared::draw(SDL_Surface * where, const Point & dest, const Rect * src, const ScalableImageParameters & parameters, int scalingFactor)
{
	const auto & getFlippedImage = [&](FlippedImages & images){
		int index = 0;
		if (parameters.flipVertical)
		{
			if (!images[index|1])
				images[index|1] = images[index]->verticalFlip();

			index |= 1;
		}

		if (parameters.flipHorizontal)
		{
			if (!images[index|2])
				images[index|2] = images[index]->horizontalFlip();

			index |= 2;
		}

		return images[index];
	};

	bool shadowLoading = scaled.at(scalingFactor).shadow.at(0) && scaled.at(scalingFactor).shadow.at(0)->isLoading();
	bool bodyLoading = scaled.at(scalingFactor).body.at(0) && scaled.at(scalingFactor).body.at(0)->isLoading();
	bool overlayLoading = scaled.at(scalingFactor).overlay.at(0) && scaled.at(scalingFactor).overlay.at(0)->isLoading();
	bool grayscaleLoading = scaled.at(scalingFactor).bodyGrayscale.at(0) && scaled.at(scalingFactor).bodyGrayscale.at(0)->isLoading();
	bool playerLoading = parameters.player != PlayerColor::CANNOT_DETERMINE && scaled.at(scalingFactor).playerColored.at(1+parameters.player.getNum()) && scaled.at(scalingFactor).playerColored.at(1+parameters.player.getNum())->isLoading();

	if (shadowLoading || bodyLoading || overlayLoading || playerLoading || grayscaleLoading)
	{
		getFlippedImage(scaled[1].body)->scaledDraw(where, parameters.palette, dimensions() * scalingFactor, dest, src, parameters.colorMultiplier, parameters.alphaValue, locator.layer);

		if (parameters.effectColorMultiplier.a != ColorRGBA::ALPHA_TRANSPARENT)
			getFlippedImage(scaled[1].body)->scaledDraw(where, parameters.palette, dimensions() * scalingFactor, dest, src, parameters.effectColorMultiplier, parameters.alphaValue, locator.layer);
		return;
	}

	if (scaled.at(scalingFactor).shadow.at(0))
		getFlippedImage(scaled.at(scalingFactor).shadow)->draw(where, parameters.palette, dest, src, Colors::WHITE_TRUE, parameters.alphaValue, locator.layer);

	if (parameters.player != PlayerColor::CANNOT_DETERMINE && scaled.at(scalingFactor).playerColored.at(1+parameters.player.getNum()))
	{
		scaled.at(scalingFactor).playerColored.at(1+parameters.player.getNum())->draw(where, parameters.palette, dest, src, Colors::WHITE_TRUE, parameters.alphaValue, locator.layer);
	}
	else
	{
		if (scaled.at(scalingFactor).body.at(0))
			getFlippedImage(scaled.at(scalingFactor).body)->draw(where, parameters.palette, dest, src, parameters.colorMultiplier, parameters.alphaValue, locator.layer);

		if (scaled.at(scalingFactor).bodyGrayscale.at(0) && parameters.effectColorMultiplier.a != ColorRGBA::ALPHA_TRANSPARENT)
			getFlippedImage(scaled.at(scalingFactor).bodyGrayscale)->draw(where, parameters.palette, dest, src, parameters.effectColorMultiplier, parameters.alphaValue, locator.layer);
	}

	if (scaled.at(scalingFactor).overlay.at(0))
		getFlippedImage(scaled.at(scalingFactor).overlay)->draw(where, parameters.palette, dest, src, parameters.ovelayColorMultiplier, static_cast<int>(parameters.alphaValue) * parameters.ovelayColorMultiplier.a / 255, locator.layer);
}

const SDL_Palette * ScalableImageShared::getPalette() const
{
	return scaled[1].body[0]->getPalette();
}

std::shared_ptr<ScalableImageInstance> ScalableImageShared::createImageReference()
{
	return std::make_shared<ScalableImageInstance>(shared_from_this(), locator.layer);
}

ScalableImageInstance::ScalableImageInstance(const std::shared_ptr<ScalableImageShared> & image, EImageBlitMode blitMode)
	:image(image)
	,parameters(image->getPalette(), blitMode)
	,blitMode(blitMode)
{
	assert(image);
}

void ScalableImageInstance::scaleTo(const Point & size, EScalingAlgorithm algorithm)
{
	scaledImage = nullptr;

	auto newScaledImage = ENGINE->renderHandler().createImage(dimensions(), CanvasScalingPolicy::AUTO);

	newScaledImage->getCanvas().draw(*this, Point(0, 0));
	newScaledImage->scaleTo(size, algorithm);
	scaledImage = newScaledImage;
}

void ScalableImageInstance::exportBitmap(const boost::filesystem::path & path) const
{
	image->exportBitmap(path, parameters);
}

bool ScalableImageInstance::isTransparent(const Point & coords) const
{
	return image->isTransparent(coords);
}

Rect ScalableImageInstance::contentRect() const
{
	return image->contentRect();
}

Point ScalableImageInstance::dimensions() const
{
	if (scaledImage)
		return scaledImage->dimensions();
	return image->dimensions();
}

void ScalableImageInstance::setAlpha(uint8_t value)
{
	parameters.alphaValue = value;
}

void ScalableImageInstance::draw(SDL_Surface * where, const Point & pos, const Rect * src, int scalingFactor) const
{
	if (scaledImage)
		scaledImage->draw(where, pos, src, scalingFactor);
	else
		image->draw(where, pos, src, parameters, scalingFactor);
}

void ScalableImageInstance::setOverlayColor(const ColorRGBA & color)
{
	parameters.ovelayColorMultiplier = color;

	if (parameters.palette)
		parameters.setOverlayColor(image->getPalette(), color, blitMode == EImageBlitMode::WITH_SHADOW_AND_SELECTION);
}

void ScalableImageInstance::setEffectColor(const ColorRGBA & color)
{
	parameters.effectColorMultiplier = color;

	if (parameters.palette)
	{
		const auto grayscaleFilter = ColorFilter::genMuxerShifter( { 0.299, 0.587, 0.114, 0}, { 0.299, 0.587, 0.114, 0}, { 0.299, 0.587, 0.114, 0}, 1);
		const auto effectStrengthFilter = ColorFilter::genRangeShifter( 0, 0, 0, color.r / 255.f, color.g / 255.f, color.b / 255.f);
		const auto effectFilter = ColorFilter::genCombined(grayscaleFilter, effectStrengthFilter);
		const auto effectiveFilter = ColorFilter::genInterpolated(ColorFilter::genEmptyShifter(), effectFilter, color.a / 255.f);

		parameters.adjustPalette(image->getPalette(), blitMode, effectiveFilter, 0);
	}

	if (color.a != ColorRGBA::ALPHA_TRANSPARENT)
		image->prepareEffectImage();
}

void ScalableImageInstance::playerColored(const PlayerColor & player)
{
	parameters.player = player;

	if (parameters.palette)
		parameters.playerColored(player);

	image->preparePlayerColoredImage(player);
}

void ScalableImageParameters::playerColored(PlayerColor playerColor)
{
	graphics->setPlayerPalette(palette, playerColor);
}

void ScalableImageInstance::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
	if (parameters.palette)
		parameters.shiftPalette(image->getPalette(),firstColorID, colorsToMove, distanceToMove);
}

void ScalableImageInstance::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{
	if (parameters.palette)
		parameters.adjustPalette(image->getPalette(), blitMode, shifter, colorsToSkipMask);
}

void ScalableImageInstance::horizontalFlip()
{
	parameters.flipHorizontal = !parameters.flipHorizontal;
}

void ScalableImageInstance::verticalFlip()
{
	parameters.flipVertical = !parameters.flipVertical;
}

std::shared_ptr<const ISharedImage> ScalableImageShared::loadOrGenerateImage(EImageBlitMode mode, int8_t scalingFactor, PlayerColor color, ImageType upscalingSource) const
{
	ImageLocator loadingLocator;

	loadingLocator.image = locator.image;
	loadingLocator.defFile = locator.defFile;
	loadingLocator.generateShadow = locator.generateShadow;
	loadingLocator.generateOverlay = locator.generateOverlay;
	loadingLocator.defFrame = locator.defFrame;
	loadingLocator.defGroup = locator.defGroup;
	loadingLocator.layer = mode;
	loadingLocator.scalingFactor = scalingFactor;
	loadingLocator.playerColored = color;

	// best case - requested image is already available in filesystem
	auto loadedFullMatchImage = ENGINE->renderHandler().loadScaledImage(loadingLocator);
	if (loadedFullMatchImage)
		return loadedFullMatchImage;

	// optional images for 1x resolution - only try load them, don't attempt to generate
	bool optionalImage =
		mode == EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR ||
		mode == EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION ||
		mode == EImageBlitMode::ONLY_FLAG_COLOR ||
		mode == EImageBlitMode::ONLY_SELECTION ||
		mode == EImageBlitMode::GRAYSCALE_BODY_HIDE_SELECTION ||
		color != PlayerColor::CANNOT_DETERMINE;

	if (scalingFactor == 1)
	{
		// this block should never be called for 'body' layer - that image is loaded unconditionally before construction
		assert(optionalImage);
		return nullptr;
	}

	// alternatively, find largest pre-scaled image, load it and rescale to desired scaling
	for (int8_t scaling = 4; scaling > 0; --scaling)
	{
		loadingLocator.scalingFactor = scaling;
		auto loadedImage = ENGINE->renderHandler().loadScaledImage(loadingLocator);
		if (loadedImage)
		{
			if (scaling == 1)
			{
				if (optionalImage)
				{
					ScalableImageParameters parameters(getPalette(), mode);
					return loadedImage->scaleInteger(scalingFactor, parameters.palette, mode);
				}
			}
			else
			{
				Point targetSize = scaled[1].body[0]->dimensions() * scalingFactor;
				return loadedImage->scaleTo(targetSize, nullptr);
			}
		}
	}

	ScalableImageParameters parameters(getPalette(), mode);
	// if all else fails - use base (presumably, indexed) image and convert it to desired form
	if (color != PlayerColor::CANNOT_DETERMINE && parameters.palette)
		parameters.playerColored(color);

	if (upscalingSource)
		return upscalingSource->scaleInteger(scalingFactor, parameters.palette, mode);
	else
		return scaled[1].body[0]->scaleInteger(scalingFactor, parameters.palette, mode);
}

void ScalableImageShared::loadScaledImages(int8_t scalingFactor, PlayerColor color)
{
	if (scaled[scalingFactor].body[0] == nullptr && scalingFactor != 1)
	{
		switch(locator.layer)
		{
			case EImageBlitMode::OPAQUE:
			case EImageBlitMode::COLORKEY:
			case EImageBlitMode::SIMPLE:
				scaled[scalingFactor].body[0] = loadOrGenerateImage(locator.layer, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].body[0]);
				break;

			case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
			case EImageBlitMode::ONLY_BODY_HIDE_SELECTION:
				scaled[scalingFactor].body[0] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_HIDE_SELECTION, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].body[0]);
				break;
			case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
			case EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR:
				scaled[scalingFactor].body[0] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].body[0]);
				break;

			case EImageBlitMode::WITH_SHADOW:
			case EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY:
				scaled[scalingFactor].body[0] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].body[0]);
				break;
		}
	}

	if (color != PlayerColor::CANNOT_DETERMINE && scaled[scalingFactor].playerColored[1+color.getNum()] == nullptr)
	{
		switch(locator.layer)
		{
			case EImageBlitMode::OPAQUE:
			case EImageBlitMode::COLORKEY:
			case EImageBlitMode::SIMPLE:
				scaled[scalingFactor].playerColored[1+color.getNum()] = loadOrGenerateImage(locator.layer, scalingFactor, color, scaled[1].playerColored[1+color.getNum()]);
				break;

			case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
			case EImageBlitMode::ONLY_BODY_HIDE_SELECTION:
				scaled[scalingFactor].playerColored[1+color.getNum()] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_HIDE_SELECTION, scalingFactor, color, scaled[1].playerColored[1+color.getNum()]);
				break;
			case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
			case EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR:
				scaled[scalingFactor].playerColored[1+color.getNum()] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_HIDE_FLAG_COLOR, scalingFactor, color, scaled[1].playerColored[1+color.getNum()]);
				break;

			case EImageBlitMode::WITH_SHADOW:
			case EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY:
				scaled[scalingFactor].playerColored[1+color.getNum()] = loadOrGenerateImage(EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY, scalingFactor, color, scaled[1].playerColored[1+color.getNum()]);
				break;
		}
	}

	if (scaled[scalingFactor].shadow[0] == nullptr)
	{
		switch(locator.layer)
		{
			case EImageBlitMode::WITH_SHADOW:
			case EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION:
			case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
				scaled[scalingFactor].shadow[0] = loadOrGenerateImage(EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].shadow[0]);
				break;
			case EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR:
			case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
				scaled[scalingFactor].shadow[0] = loadOrGenerateImage(EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].shadow[0]);
				break;
			default:
				break;
		}
	}

	if (scaled[scalingFactor].overlay[0] == nullptr)
	{
		switch(locator.layer)
		{
			case EImageBlitMode::ONLY_FLAG_COLOR:
			case EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR:
				scaled[scalingFactor].overlay[0] = loadOrGenerateImage(EImageBlitMode::ONLY_FLAG_COLOR, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].overlay[0]);
				break;
			case EImageBlitMode::ONLY_SELECTION:
			case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
				scaled[scalingFactor].overlay[0] = loadOrGenerateImage(EImageBlitMode::ONLY_SELECTION, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].overlay[0]);
				break;
			default:
				break;
		}
	}
}

void ScalableImageShared::preparePlayerColoredImage(PlayerColor color)
{
	loadScaledImages(ENGINE->screenHandler().getScalingFactor(), color);
}

void ScalableImageShared::prepareEffectImage()
{
	int scalingFactor = ENGINE->screenHandler().getScalingFactor();

	if (scaled[scalingFactor].bodyGrayscale[0] == nullptr)
	{
		switch(locator.layer)
		{
			case EImageBlitMode::WITH_SHADOW_AND_SELECTION:
				scaled[scalingFactor].bodyGrayscale[0] = loadOrGenerateImage(EImageBlitMode::GRAYSCALE_BODY_HIDE_SELECTION, scalingFactor, PlayerColor::CANNOT_DETERMINE, scaled[1].bodyGrayscale[0]);
				break;
			default:
				break;
		}
	}
}
