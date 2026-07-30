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
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using Duration = Clock::duration;

	/// cyclic buffer of durations of last frames
	std::array<Duration, 60> lastFrameTimes;

	Duration targetFrameTime;
	TimePoint lastTimePoint;
	TimePoint lastRenderedTimePoint;

	/// index of last measured from in lastFrameTimes array
	ui32 lastFrameIndex;

	/// time not yet handed out by consumeElapsedMilliseconds, including skipped frames
	Duration elapsedSinceConsumed = Duration::zero();

	bool vsyncEnabled;

	/// Typical interval between two drawn frames, which is one display refresh while vsync paces
	/// them - the target frame rate says nothing about how fast the display actually runs
	Duration measuredRefreshInterval() const;

public:
	FramerateManager(int targetFramerate);

	/// must be called every iteration of the main loop, whether or not a frame was drawn
	/// updates framerate calculations and executes sleep to maintain target frame rate
	void framerateDelay(bool frameRendered);

	/// returns duration of last frame in seconds
	ui32 getElapsedMilliseconds() const;

	/// whole milliseconds elapsed since the last call, carrying the remainder over. Covers frames
	/// that were not rendered, so animations advance by real time rather than by frames drawn
	ui32 consumeElapsedMilliseconds();

	/// returns current estimation of frame rate
	ui32 getFramerate() const;
};
