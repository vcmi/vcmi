/*
 * IImageLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Point;
VCMI_LIB_NAMESPACE_END

class SDLImage;

struct SDL_Color;

class IImageLoader
{
public:
	//load size raw pixels from data
	virtual void load(size_t size, const ui8 * data) = 0;
	//set size pixels to color
	virtual void load(size_t size, ui8 color=0) = 0;

	virtual void endLine() = 0;
	//init image with these sizes and palette
	virtual void init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal) = 0;

	virtual ~IImageLoader() = default;
};
