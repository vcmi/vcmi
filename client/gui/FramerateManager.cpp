/*
 * FramerateManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "FramerateManager.h"

#include <SDL_timer.h>

FramerateManager::FramerateManager(int newRate)
	: rate(0)
	, rateticks(0)
	, fps(0)
	, accumulatedFrames(0)
	, accumulatedTime(0)
	, lastticks(0)
	, timeElapsed(0)
{
	init(newRate);
}

void FramerateManager::init(int newRate)
{
	rate = newRate;
	rateticks = 1000.0 / rate;
	this->lastticks = SDL_GetTicks();
}

void FramerateManager::framerateDelay()
{
	ui32 currentTicks = SDL_GetTicks();

	timeElapsed = currentTicks - lastticks;
	accumulatedFrames++;

	// FPS is higher than it should be, then wait some time
	if(timeElapsed < rateticks)
	{
		int timeToSleep = (uint32_t)ceil(this->rateticks) - timeElapsed;
		boost::this_thread::sleep(boost::posix_time::milliseconds(timeToSleep));
	}

	currentTicks = SDL_GetTicks();
	// recalculate timeElapsed for external calls via getElapsed()
	// limit it to 100 ms to avoid breaking animation in case of huge lag (e.g. triggered breakpoint)
	timeElapsed = std::min<ui32>(currentTicks - lastticks, 100);

	lastticks = SDL_GetTicks();

	accumulatedTime += timeElapsed;

	if(accumulatedFrames >= 100)
	{
		//about 2 second should be passed
		fps = static_cast<int>(ceil(1000.0 / (accumulatedTime / accumulatedFrames)));
		accumulatedTime = 0;
		accumulatedFrames = 0;
	}
}
