#include "StdInc.h"
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
