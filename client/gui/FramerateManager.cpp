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

#include "../../lib/CConfigHandler.h"
#include <SDL_video.h>

FramerateManager::FramerateManager(int targetFrameRate)
	: lastFrameIndex(0)
	, lastFrameTimes({})
	, lastTimePoint(Clock::now())
	, vsyncEnabled(settings["video"]["vsync"].Bool())
{
	if(vsyncEnabled)
	{
		static int display_in_use = 0;
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(display_in_use, &mode);
		int displayRefreshRate = mode.refresh_rate;
		logGlobal->info("Display refresh rate is %d", displayRefreshRate);
		targetFrameTime = Duration(boost::chrono::seconds(1)) / displayRefreshRate;
	} else
	{
		targetFrameTime = Duration(boost::chrono::seconds(1)) / targetFrameRate;
	}
	boost::range::fill(lastFrameTimes, targetFrameTime);
}

void FramerateManager::framerateDelay()
{
	Duration timeSpentBusy = Clock::now() - lastTimePoint;

	// FPS is higher than it should be, then wait some time
	if(timeSpentBusy < targetFrameTime)
		boost::this_thread::sleep_for(targetFrameTime - timeSpentBusy);

	// compute actual timeElapsed taking into account actual sleep interval
	// limit it to 100 ms to avoid breaking animation in case of huge lag (e.g. triggered breakpoint)
	TimePoint currentTicks = Clock::now();
	Duration timeElapsed = currentTicks - lastTimePoint;
	if(timeElapsed > boost::chrono::milliseconds(100))
		timeElapsed = boost::chrono::milliseconds(100);

	lastTimePoint = currentTicks;
	lastFrameIndex = (lastFrameIndex + 1) % lastFrameTimes.size();
	lastFrameTimes[lastFrameIndex] = timeElapsed;
}

ui32 FramerateManager::getElapsedMilliseconds() const
{
	return lastFrameTimes[lastFrameIndex] / boost::chrono::milliseconds(1);
}

ui32 FramerateManager::getFramerate() const
{
	Duration accumulatedTime = std::accumulate(lastFrameTimes.begin(), lastFrameTimes.end(), Duration());

	auto actualFrameTime = accumulatedTime / lastFrameTimes.size();
	if(actualFrameTime == actualFrameTime.zero())
		return 0;

	return std::round(boost::chrono::duration<double>(1) / actualFrameTime);
};
