/*
 * FramerateManager.cpp, part of VCMI engine
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

FramerateManager::FramerateManager(int targetFrameRate)
	: targetFrameTime(Duration(std::chrono::seconds(1)) / targetFrameRate)
	, lastFrameIndex(0)
	, lastFrameTimes({})
	, lastTimePoint(Clock::now())
	, lastRenderedTimePoint(Clock::now())
	, vsyncEnabled(settings["video"]["vsync"].Bool())
{
	std::ranges::fill(lastFrameTimes, targetFrameTime);
}

FramerateManager::Duration FramerateManager::measuredRefreshInterval() const
{
	// median, not mean or minimum - a stalled frame drags the mean up, and one that never had to
	// wait for the display drags the minimum down
	auto sorted = lastFrameTimes;
	const auto middle = sorted.begin() + sorted.size() / 2;
	std::ranges::nth_element(sorted, middle);

	return *middle;
}

void FramerateManager::framerateDelay(bool frameRendered)
{
	Duration timeSpentBusy = Clock::now() - lastTimePoint;

	// vsync paces rendered frames inside SDL_RenderPresent, which a skipped frame never reaches
	const bool pacedByVsync = vsyncEnabled && frameRendered;

	Duration pacingTarget = targetFrameTime;

	// A skipped frame has to pace itself, but pacing it to targetfps holds the loop back for a
	// whole 60Hz period on a 120Hz display - and it wakes up out of step with the refresh, so the
	// frame after it misses its deadline too. One refresh is enough to keep the loop from spinning.
	if(vsyncEnabled && !frameRendered)
		pacingTarget = std::min(pacingTarget, measuredRefreshInterval());

	if(!pacedByVsync && timeSpentBusy < pacingTarget)
	{
		// if FPS is higher than it should be, then wait some time
		std::this_thread::sleep_for(pacingTarget - timeSpentBusy);
	}

	// compute actual timeElapsed taking into account actual sleep interval
	// limit it to 100 ms to avoid breaking animation in case of huge lag (e.g. triggered breakpoint)
	TimePoint currentTicks = Clock::now();
	Duration timeElapsed = currentTicks - lastTimePoint;
	if(timeElapsed > std::chrono::milliseconds(100))
		timeElapsed = std::chrono::milliseconds(100);

	lastTimePoint = currentTicks;
	elapsedSinceConsumed += timeElapsed;

	if(!frameRendered)
		return;

	// framerate is measured between drawn frames, so that skipped ones do not inflate it
	Duration sinceRendered = currentTicks - lastRenderedTimePoint;
	if(sinceRendered > std::chrono::milliseconds(100))
		sinceRendered = std::chrono::milliseconds(100);

	lastRenderedTimePoint = currentTicks;
	lastFrameIndex = (lastFrameIndex + 1) % lastFrameTimes.size();
	lastFrameTimes[lastFrameIndex] = sinceRendered;
}

ui32 FramerateManager::getElapsedMilliseconds() const
{
	return lastFrameTimes[lastFrameIndex] / std::chrono::milliseconds(1);
}

ui32 FramerateManager::consumeElapsedMilliseconds()
{
	ui32 result = elapsedSinceConsumed / std::chrono::milliseconds(1);
	elapsedSinceConsumed -= std::chrono::milliseconds(result);
	return result;
}

ui32 FramerateManager::getFramerate() const
{
	Duration accumulatedTime = std::accumulate(lastFrameTimes.begin(), lastFrameTimes.end(), Duration());

	auto actualFrameTime = accumulatedTime / lastFrameTimes.size();
	if(actualFrameTime == actualFrameTime.zero())
		return 0;

	return std::round(std::chrono::duration<double>(1) / actualFrameTime);
};
