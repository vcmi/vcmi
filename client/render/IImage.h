/*
 * IImage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class PlayerColor;
class Rect;
class Point;
class ColorRGBA;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class ColorFilter;

/// Defines which blit method will be selected when image is used for rendering
enum class EImageBlitMode
{
	/// Image can have no transparency and can be only used as background
	OPAQUE,

	/// Image can have only a single color as transparency and has no semi-transparent areas
	COLORKEY,

	/// Image might have full alpha transparency range, e.g. shadows
	ALPHA
};

/*
 * Base class for images, can be used for non-animation pictures as well
 */
class IImage
{
public:
	using SpecialPalette = std::vector<ColorRGBA>;
	static constexpr int32_t SPECIAL_PALETTE_MASK_CREATURES = 0b11110011;

	//draws image on surface "where" at position
	virtual void draw(SDL_Surface * where, int posX = 0, int posY = 0, const Rect * src = nullptr) const = 0;
	virtual void draw(SDL_Surface * where, const Rect * dest, const Rect * src) const = 0;

	virtual std::shared_ptr<IImage> scaleFast(const Point & size) const = 0;

	virtual void exportBitmap(const boost::filesystem::path & path) const = 0;

	//Change palette to specific player
	virtual void playerColored(PlayerColor player)=0;

	//set special color for flag
	virtual void setFlagColor(PlayerColor player)=0;

	//test transparency of specific pixel
	virtual bool isTransparent(const Point & coords) const = 0;

	virtual Point dimensions() const = 0;
	int width() const;
	int height() const;

	//only indexed bitmaps, 16 colors maximum
	virtual void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) = 0;
	virtual void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) = 0;
	virtual void resetPalette(int colorID) = 0;
	virtual void resetPalette() = 0;

	virtual void setAlpha(uint8_t value) = 0;
	virtual void setBlitMode(EImageBlitMode mode) = 0;

	//only indexed bitmaps with 7 special colors
	virtual void setSpecialPallete(const SpecialPalette & SpecialPalette, uint32_t colorsToSkipMask) = 0;

	virtual void horizontalFlip() = 0;
	virtual void verticalFlip() = 0;
	virtual void doubleFlip() = 0;

	IImage() = default;
	virtual ~IImage() = default;
};

