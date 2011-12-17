#pragma once

struct SDL_Surface;

/*
 * IShowable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Defines a show method
class IShowable
{
public:
	virtual void redraw()=0;
	virtual void show(SDL_Surface * to)=0;
	virtual void showAll(SDL_Surface * to)
	{
		show(to);
	}
	virtual ~IShowable(){}; //d-tor
};