/*
 * CAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class Rect;
class Point;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;
class CDefFile;
class ColorFilter;

/*
 * Base class for images, can be used for non-animation pictures as well
 */
class IImage
{
public:
	using SpecialPalette = std::array<SDL_Color, 7>;

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
	virtual void shiftPalette(int from, int howMany) = 0;
	virtual void adjustPalette(const ColorFilter & shifter, size_t colorsToSkip) = 0;
	virtual void resetPalette(int colorID) = 0;
	virtual void resetPalette() = 0;

	virtual void setAlpha(uint8_t value) = 0;

	//only indexed bitmaps with 7 special colors
	virtual void setSpecialPallete(const SpecialPalette & SpecialPalette) = 0;

	virtual void horizontalFlip() = 0;
	virtual void verticalFlip() = 0;

	IImage();
	virtual ~IImage();

	/// loads image from specified file. Returns 0-sized images on failure
	static std::shared_ptr<IImage> createFromFile( const std::string & path );

	/// temporary compatibility method. Creates IImage from existing SDL_Surface
	/// Surface will be shared, called must still free it with SDL_FreeSurface
	static std::shared_ptr<IImage> createFromSurface( SDL_Surface * source );
};

