
/*
 * SDL_framerate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../stdafx.h"
#include "SDL_framerate.h"
#include <SDL.h>


FPSManager::FPSManager(int rate)
{
	this->rate = rate;
	this->rateticks = (1000.0 / (double) rate);
	this->fps = 0;
}

void FPSManager::init()
{
	this->lastticks = SDL_GetTicks();
}

void FPSManager::framerateDelay()
{
    Uint32 currentTicks = SDL_GetTicks();
	double diff = currentTicks - this->lastticks;

	if (diff < this->rateticks) // FPS is higher than it should be, then wait some time
	{
		SDL_Delay(ceil(this->rateticks) - diff);
	}

	this->fps = ceil(1000. / (SDL_GetTicks() - this->lastticks));
	this->lastticks = SDL_GetTicks();
}
