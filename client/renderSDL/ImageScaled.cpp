/*
 * ImageScaled.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ImageScaled.h"

#include "SDLImage.h"
#include "SDL_Extensions.h"

#include "../gui/CGuiHandler.h"
#include "../render/IScreenHandler.h"
#include "../render/Colors.h"

#include "../../lib/constants/EntityIdentifiers.h"

#include <SDL_surface.h>

ImageScaled::ImageScaled(const ImageLocator & inputLocator, const std::shared_ptr<ISharedImage> & source, EImageBlitMode mode)
	: source(source)
	, locator(inputLocator)
	, colorMultiplier(Colors::WHITE_TRUE)
	, alphaValue(SDL_ALPHA_OPAQUE)
	, blitMode(mode)
{
	locator.scalingFactor = GH.screenHandler().getScalingFactor();
	setBodyEnabled(true);
}

std::shared_ptr<ISharedImage> ImageScaled::getSharedImage() const
{
	return body;
}

void ImageScaled::scaleInteger(int factor)
{
	assert(0);
}

void ImageScaled::scaleTo(const Point & size)
{
	if (body)
		body = body->scaleTo(size, nullptr); // FIXME: adjust for scaling
}

void ImageScaled::exportBitmap(const boost::filesystem::path &path) const
{
	source->exportBitmap(path);
}

bool ImageScaled::isTransparent(const Point &coords) const
{
	return source->isTransparent(coords);
}

Point ImageScaled::dimensions() const
{
	return source->dimensions();
}

void ImageScaled::setAlpha(uint8_t value)
{
	alphaValue = value;
}

void ImageScaled::setBlitMode(EImageBlitMode mode)
{
	blitMode = mode;
}

void ImageScaled::draw(SDL_Surface *where, const Point &pos, const Rect *src) const
{
	if (shadow)
		shadow->draw(where, nullptr, pos, src, Colors::WHITE_TRUE, alphaValue, blitMode);
	if (body)
		body->draw(where, nullptr, pos, src, Colors::WHITE_TRUE, alphaValue, blitMode);
	if (overlay)
		overlay->draw(where, nullptr, pos, src, colorMultiplier, colorMultiplier.a * alphaValue / 255, blitMode);
}

void ImageScaled::setOverlayColor(const ColorRGBA & color)
{
	colorMultiplier = color;
}

void ImageScaled::playerColored(PlayerColor player)
{
	playerColor = player;
	if (body)
		setBodyEnabled(true); // regenerate
}

void ImageScaled::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
	// TODO: implement
}

void ImageScaled::adjustPalette(const ColorFilter &shifter, uint32_t colorsToSkipMask)
{
	// TODO: implement
}

void ImageScaled::setShadowEnabled(bool on)
{
	if (on)
	{
		locator.layerBody = false;
		locator.layerShadow = true;
		locator.layerOverlay = false;
		locator.playerColored = PlayerColor::CANNOT_DETERMINE;
		shadow = GH.renderHandler().loadImage(locator, blitMode)->getSharedImage();
	}
	else
		shadow = nullptr;
}

void ImageScaled::setBodyEnabled(bool on)
{
	if (on)
	{
		locator.layerBody = true;
		locator.layerShadow = false;
		locator.layerOverlay = false;
		locator.playerColored = playerColor;
		body = GH.renderHandler().loadImage(locator, blitMode)->getSharedImage();
	}
	else
		body = nullptr;
}


void ImageScaled::setOverlayEnabled(bool on)
{
	if (on)
	{
		locator.layerBody = false;
		locator.layerShadow = false;
		locator.layerOverlay = true;
		locator.playerColored = PlayerColor::CANNOT_DETERMINE;
		overlay = GH.renderHandler().loadImage(locator, blitMode)->getSharedImage();
	}
	else
		overlay = nullptr;
}
