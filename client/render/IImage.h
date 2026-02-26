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
struct SDL_Palette;
class ColorFilter;
class ISharedImage;

/// Defines which blit method will be selected when image is used for rendering
enum class EImageBlitMode : uint8_t
{
	/// Preferred for images that don't need any background
	/// Indexed or RGBA: Image can have no transparency and can be only used as background
	OPAQUE,

	/// Preferred for images that may need transparency
	/// Indexed: Image can have only a single color as transparency and has no semi-transparent areas
	/// RGBA: full alpha transparency range, e.g. shadows
	COLORKEY,

	/// Full transparency including shadow, but treated as a single image
	/// Indexed: Image can have alpha transparency, e.g. shadow
	/// RGBA: full alpha transparency range, e.g. shadows
	/// Upscaled form: single image, no option to display shadow separately
	SIMPLE,

	/// RGBA, may consist from 2 separate parts: base and shadow, overlay not preset or treated as part of body
	WITH_SHADOW,

	/// RGBA, may consist from 3 separate parts: base, shadow, and overlay
	WITH_SHADOW_AND_SELECTION,
	WITH_SHADOW_AND_FLAG_COLOR,

	/// RGBA, contains only body, with shadow and overlay disabled
	GRAYSCALE_BODY_HIDE_SELECTION,
	ONLY_BODY_HIDE_SELECTION,
	ONLY_BODY_HIDE_FLAG_COLOR,

	/// RGBA, contains only body, with shadow disabled and overlay treated as part of body
	ONLY_BODY_IGNORE_OVERLAY,

	/// RGBA, contains only shadow
	ONLY_SHADOW_HIDE_SELECTION,
	ONLY_SHADOW_HIDE_FLAG_COLOR,

	/// RGBA, contains only overlay
	ONLY_SELECTION,
	ONLY_FLAG_COLOR,
};

enum class EScalingAlgorithm : int8_t
{
	NEAREST,
	BILINEAR,
	XBRZ_OPAQUE, // xbrz, image edges are considered to have same color as pixel inside image. Only for integer scaling
	XBRZ_ALPHA // xbrz, image edges are considered to be transparent. Only for integer scaling
};

/// Base class for images for use in client code.
/// This class represents current state of image, with potential transformations applied, such as player coloring
class IImage
{
public:
	//draws image on surface "where" at position
	virtual void draw(SDL_Surface * where, const Point & pos, const Rect * src, int scalingFactor) const = 0;

	virtual void scaleTo(const Point & size, EScalingAlgorithm algorithm) = 0;

	virtual void exportBitmap(const boost::filesystem::path & path) const = 0;

	//Change palette to specific player
	virtual void playerColored(const PlayerColor & player) = 0;

	//test transparency of specific pixel
	virtual bool isTransparent(const Point & coords) const = 0;

	virtual Rect contentRect() const = 0;

	virtual Point dimensions() const = 0;
	int width() const;
	int height() const;

	//only indexed bitmaps, 16 colors maximum
	virtual void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) = 0;
	virtual void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) = 0;

	virtual void setAlpha(uint8_t value) = 0;

	virtual void setOverlayColor(const ColorRGBA & color) = 0;
	virtual void setEffectColor(const ColorRGBA & color) = 0;

	virtual ~IImage() = default;
};

/// Base class for image data, mostly for internal use
/// Represents unmodified pixel data, usually loaded from file
/// This image can be shared between multiple image handlers (IImage instances)
class ISharedImage
{
public:
	virtual Point dimensions() const = 0;
	virtual void exportBitmap(const boost::filesystem::path & path, SDL_Palette * palette) const = 0;
	virtual bool isTransparent(const Point & coords) const = 0;
	virtual Rect contentRect() const = 0;

	virtual void scaledDraw(SDL_Surface * where, SDL_Palette * palette, const Point & scaling, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const = 0;
	virtual void draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const = 0;

	/// Returns true if this image is still loading and can't be used
	virtual bool isLoading() const = 0;

	/// When disabled upscaling needs to be done in sync (e.g. because there is no 1x base image)
	virtual void setAsyncUpscale(bool on) = 0;
	virtual bool getAsyncUpscale() const = 0;

	virtual ~ISharedImage() = default;

	virtual const SDL_Palette * getPalette() const = 0;

	[[nodiscard]] virtual std::shared_ptr<const ISharedImage> horizontalFlip() const = 0;
	[[nodiscard]] virtual std::shared_ptr<const ISharedImage> verticalFlip() const = 0;
	[[nodiscard]] virtual std::shared_ptr<const ISharedImage> scaleInteger(int factor, SDL_Palette * palette, EImageBlitMode blitMode) const = 0;
	[[nodiscard]] virtual std::shared_ptr<const ISharedImage> scaleTo(const Point & size, SDL_Palette * palette) const = 0;

};
