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
#include "../render/IRenderHandler.h"

#include "../../lib/Color.h"
#include "../../lib/constants/EntityIdentifiers.h"

struct SDL_Palette;

class SDLImageShared;

// Upscaled image with several mechanisms to emulate H3 palette effects
class ImageScaled final : public IImage
{
private:

	/// Original unscaled image
	std::shared_ptr<ISharedImage> source;

	/// Upscaled shadow of our image, may be null
	std::shared_ptr<ISharedImage> shadow;

	/// Upscaled main part of our image, may be null
	std::shared_ptr<ISharedImage> body;

	/// Upscaled overlay (player color, selection highlight) of our image, may be null
	std::shared_ptr<ISharedImage> overlay;

	ImageLocator locator;

	ColorRGBA colorMultiplier;
	PlayerColor playerColor = PlayerColor::CANNOT_DETERMINE;

	uint8_t alphaValue;
	EImageBlitMode blitMode;

public:
	ImageScaled(const ImageLocator & locator, const std::shared_ptr<ISharedImage> & source, EImageBlitMode mode);

	void scaleInteger(int factor) override;
	void scaleTo(const Point & size) override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	bool isTransparent(const Point & coords) const override;
	Point dimensions() const override;
	void setAlpha(uint8_t value) override;
	void setBlitMode(EImageBlitMode mode) override;
	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setOverlayColor(const ColorRGBA & color) override;
	void playerColored(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;

	void setShadowEnabled(bool on) override;
	void setBodyEnabled(bool on) override;
	void setOverlayEnabled(bool on) override;
	std::shared_ptr<ISharedImage> getSharedImage() const override;
};
