/*
 * SDLImageLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IImageLoader.h"

class SDLImageLoader : public IImageLoader
{
	SDLImage * image;
	ui8 * lineStart;
	ui8 * position;
public:
	//load size raw pixels from data
	void load(size_t size, const ui8 * data);
	//set size pixels to color
	void load(size_t size, ui8 color=0);
	void endLine();
	//init image with these sizes and palette
	void init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal);

	SDLImageLoader(SDLImage * Img);
	~SDLImageLoader();
};


