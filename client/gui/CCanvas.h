/*
 * CCanvas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_Surface;
struct Point;
class IImage;

/// Class that represents surface for drawing on
class CCanvas
{
	SDL_Surface * surface;
public:
	// constructs canvas using existing surface. Caller maintains ownership on the surface
	CCanvas(SDL_Surface * surface);

	CCanvas(const Point & size);
	~CCanvas();

	// renders image onto this canvas
	void draw(std::shared_ptr<IImage> image, const Point & pos);

	// renders another canvas onto this canvas
	void draw(std::shared_ptr<CCanvas> image, const Point & pos);

	// for compatibility, copies content of this canvas onto SDL_Surface
	void copyTo(SDL_Surface * to, const Point & pos);
};
