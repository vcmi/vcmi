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

#include "../../lib/constants/EntityIdentifiers.h"

#include <SDL_surface.h>

ImageConstScaled::ImageConstScaled(std::shared_ptr<SDLImageConst> sourceImage)
	:sourceImage(sourceImage)
{
	scaledImage = sourceImage->scaleFast(sourceImage->dimensions() * getScalingFactor());
}

int ImageConstScaled::getScalingFactor() const
{
	return 2;
}

void ImageConstScaled::draw(SDL_Surface *where, const Point &dest, const Rect *src, uint8_t alpha, EImageBlitMode mode) const
{
	scaledImage->draw(where, nullptr, dest, src, alpha, mode);
}

void ImageConstScaled::exportBitmap(const boost::filesystem::path &path) const
{
	sourceImage->exportBitmap(path);
}

Point ImageConstScaled::dimensions() const
{
	return sourceImage->dimensions();
}

bool ImageConstScaled::isTransparent(const Point &coords) const
{
	return sourceImage->isTransparent(coords);
}

std::shared_ptr<IImage> ImageConstScaled::createImageReference(EImageBlitMode mode)
{
	return std::make_shared<ImageScaled>(shared_from_this(), mode);
}

std::shared_ptr<IConstImage> ImageConstScaled::horizontalFlip() const
{
	return std::make_shared<ImageConstScaled>(std::dynamic_pointer_cast<SDLImageConst>(sourceImage->horizontalFlip()));
}

std::shared_ptr<IConstImage> ImageConstScaled::verticalFlip() const
{
	return std::make_shared<ImageConstScaled>(std::dynamic_pointer_cast<SDLImageConst>(sourceImage->verticalFlip()));
}

std::shared_ptr<ImageConstScaled> ImageConstScaled::scaleFast(const Point &size) const
{
	return std::make_shared<ImageConstScaled>(sourceImage->scaleFast(size));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ImageScaled::ImageScaled(const std::shared_ptr<ImageConstScaled> &image, EImageBlitMode mode)
	:image(image)
	, alphaValue(SDL_ALPHA_OPAQUE)
	, blitMode(mode)
{
}

void ImageScaled::scaleFast(const Point &size)
{
	image = image->scaleFast(size);
}

void ImageScaled::exportBitmap(const boost::filesystem::path &path) const
{
	image->exportBitmap(path);
}

bool ImageScaled::isTransparent(const Point &coords) const
{
	return image->isTransparent(coords);
}

Point ImageScaled::dimensions() const
{
	return image->dimensions();
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
	image->draw(where, pos, src, alphaValue, blitMode);
}

void ImageScaled::setSpecialPalette(const SpecialPalette &SpecialPalette, uint32_t colorsToSkipMask)
{
}

void ImageScaled::playerColored(PlayerColor player)
{
}

void ImageScaled::setFlagColor(PlayerColor player)
{
}

void ImageScaled::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
}

void ImageScaled::adjustPalette(const ColorFilter &shifter, uint32_t colorsToSkipMask)
{
}
