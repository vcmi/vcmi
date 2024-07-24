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

ImageSharedScaled::ImageSharedScaled(std::shared_ptr<SDLImageShared> sourceImage)
	:sourceImage(sourceImage)
{
	scaledImage = sourceImage->scaleFast(sourceImage->dimensions() * getScalingFactor());
}

int ImageSharedScaled::getScalingFactor() const
{
	return 2;
}

void ImageSharedScaled::draw(SDL_Surface *where, const Point &dest, const Rect *src, uint8_t alpha, EImageBlitMode mode) const
{
	scaledImage->draw(where, nullptr, dest, src, alpha, mode);
}

void ImageSharedScaled::exportBitmap(const boost::filesystem::path &path) const
{
	sourceImage->exportBitmap(path);
}

Point ImageSharedScaled::dimensions() const
{
	return sourceImage->dimensions();
}

bool ImageSharedScaled::isTransparent(const Point &coords) const
{
	return sourceImage->isTransparent(coords);
}

std::shared_ptr<IImage> ImageSharedScaled::createImageReference(EImageBlitMode mode)
{
	return std::make_shared<ImageScaled>(shared_from_this(), mode);
}

std::shared_ptr<ISharedImage> ImageSharedScaled::horizontalFlip() const
{
	return std::make_shared<ImageSharedScaled>(std::dynamic_pointer_cast<SDLImageShared>(sourceImage->horizontalFlip()));
}

std::shared_ptr<ISharedImage> ImageSharedScaled::verticalFlip() const
{
	return std::make_shared<ImageSharedScaled>(std::dynamic_pointer_cast<SDLImageShared>(sourceImage->verticalFlip()));
}

std::shared_ptr<ImageSharedScaled> ImageSharedScaled::scaleFast(const Point &size) const
{
	return std::make_shared<ImageSharedScaled>(sourceImage->scaleFast(size));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ImageScaled::ImageScaled(const std::shared_ptr<ImageSharedScaled> &image, EImageBlitMode mode)
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
