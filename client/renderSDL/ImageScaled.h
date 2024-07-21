/*
 * ImageScaled.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IImage.h"

struct SDL_Palette;

class SDLImageConst;

class ImageConstScaled final : public IConstImage, public std::enable_shared_from_this<ImageConstScaled>, boost::noncopyable
{
	std::shared_ptr<SDLImageConst> sourceImage;
	std::shared_ptr<SDLImageConst> scaledImage;

	int getScalingFactor() const;
public:
	ImageConstScaled(std::shared_ptr<SDLImageConst> sourceImage);

	void draw(SDL_Surface * where, const Point & dest, const Rect * src, uint8_t alpha, EImageBlitMode mode) const;

	void exportBitmap(const boost::filesystem::path & path) const override;
	Point dimensions() const override;
	bool isTransparent(const Point & coords) const override;
	std::shared_ptr<IImage> createImageReference(EImageBlitMode mode) override;
	std::shared_ptr<IConstImage> horizontalFlip() const override;
	std::shared_ptr<IConstImage> verticalFlip() const override;
	std::shared_ptr<ImageConstScaled> scaleFast(const Point & size) const;
};

class ImageScaled final : public IImage
{
private:
	std::shared_ptr<ImageConstScaled> image;

	uint8_t alphaValue;
	EImageBlitMode blitMode;

public:
	ImageScaled(const std::shared_ptr<ImageConstScaled> & image, EImageBlitMode mode);

	void scaleFast(const Point & size) override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	bool isTransparent(const Point & coords) const override;
	Point dimensions() const override;
	void setAlpha(uint8_t value) override;
	void setBlitMode(EImageBlitMode mode) override;
	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setSpecialPalette(const SpecialPalette & SpecialPalette, uint32_t colorsToSkipMask) override;
	void playerColored(PlayerColor player) override;
	void setFlagColor(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;
};
