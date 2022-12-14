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

#include "Geometries.h"

struct SDL_Color;
struct SDL_Surface;
class IImage;
enum EFonts : int;

/// Class that represents surface for drawing on
class Canvas
{
	SDL_Surface * surface;

	Canvas & operator = (Canvas & other) = delete;
public:

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	Canvas(SDL_Surface * surface);

	/// copy contructor
	Canvas(Canvas & other);

	/// constructs canvas of specified size
	Canvas(const Point & size);

	~Canvas();

	/// renders image onto this canvas at specified position
	void draw(std::shared_ptr<IImage> image, const Point & pos);

	/// renders section of image bounded by sourceRect at specified position
	void draw(std::shared_ptr<IImage> image, const Point & pos, const Rect & sourceRect);

	/// renders section of image bounded by sourceRect at specified position at specific transparency value
	void draw(std::shared_ptr<IImage> image, const Point & pos, const Rect & sourceRect, uint8_t alpha);


	/// renders another canvas onto this canvas
	void draw(Canvas & image, const Point & pos);

	/// renders continuous, 1-pixel wide line with color gradient
	void drawLine(const Point & from, const Point & dest, const SDL_Color & colorFrom, const SDL_Color & colorDest);

	/// renders single line of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::string & text );

	/// renders multiple lines of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::vector<std::string> & text );
};
