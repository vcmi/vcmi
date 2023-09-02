/*
 * Canvas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/TextAlignment.h"
#include "../../lib/Rect.h"
#include "../../lib/Color.h"

struct SDL_Surface;
class IImage;
enum EFonts : int;

/// Class that represents surface for drawing on
class Canvas
{
	/// Target surface
	SDL_Surface * surface;

	/// Current rendering area, all rendering operations will be moved into selected area
	Rect renderArea;

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	explicit Canvas(SDL_Surface * surface);

	/// copy contructor
	Canvas(const Canvas & other);

public:
	Canvas & operator = (const Canvas & other) = delete;
	Canvas & operator = (Canvas && other) = delete;

	/// move contructor
	Canvas(Canvas && other);

	/// creates canvas that only covers specified subsection of a surface
	Canvas(const Canvas & other, const Rect & clipRect);

	/// constructs canvas of specified size
	explicit Canvas(const Point & size);

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	/// Compatibility method. AVOID USAGE. To be removed once SDL abstraction layer is finished.
	static Canvas createFromSurface(SDL_Surface * surface);

	~Canvas();

	/// if set to true, drawing this canvas onto another canvas will use alpha channel information
	void applyTransparency(bool on);

	/// applies grayscale filter onto current image
	void applyGrayscale();

	/// renders image onto this canvas at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos);

	/// renders section of image bounded by sourceRect at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect);

	/// renders another canvas onto this canvas
	void draw(const Canvas &image, const Point & pos);

	/// renders another canvas onto this canvas with transparency
	void drawTransparent(const Canvas & image, const Point & pos, double transparency);

	/// renders another canvas onto this canvas with scaling
	void drawScaled(const Canvas &image, const Point & pos, const Point & targetSize);

	/// renders single pixels with specified color
	void drawPoint(const Point & dest, const ColorRGBA & color);

	/// renders continuous, 1-pixel wide line with color gradient
	void drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest);

	/// renders dashed, 1-pixel wide line with specified color
	void drawLineDashed(const Point & from, const Point & dest, const ColorRGBA & color);

	/// renders rectangular, solid-color border in specified location
	void drawBorder(const Rect & target, const ColorRGBA & color, int width = 1);

	/// renders rectangular, dashed border in specified location
	void drawBorderDashed(const Rect & target, const ColorRGBA & color);

	/// renders single line of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text );

	/// renders multiple lines of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text );

	/// fills selected area with solid color, allows replacing or overdrawing
	void drawColor(const Rect & target, const ColorRGBA & color, const bool replace = true);

	/// Compatibility method. AVOID USAGE. To be removed once SDL abstraction layer is finished.
	SDL_Surface * getInternalSurface();
};
