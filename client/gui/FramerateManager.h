/*
 * FramerateManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Framerate manager controls current game frame rate by constantly trying to reach targeted frame rate
class FramerateManager
{
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = Clock::time_point;
	using Duration = Clock::duration;

	/// cyclic buffer of durations of last frames
	std::array<Duration, 60> lastFrameTimes;

	Duration targetFrameTime;
	TimePoint lastTimePoint;

	/// index of last measured frome in lastFrameTimes array
	ui32 lastFrameIndex;

public:
	FramerateManager(int targetFramerate);

	/// must be called every frame
	/// updates framerate calculations and executes sleep to maintain target frame rate
	void framerateDelay();

	/// returns duration of last frame in seconds
	ui32 getElapsedMilliseconds() const;

	/// returns current estimation of frame rate
	ui32 getFramerate() const;
};
