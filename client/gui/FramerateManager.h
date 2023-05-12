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

// A fps manager which holds game updates at a constant rate
class FramerateManager
{
private:
	double rateticks;
	ui32 lastticks;
	ui32 timeElapsed;
	int rate;
	int fps; // the actual fps value
	ui32 accumulatedTime;
	ui32 accumulatedFrames;

public:
	FramerateManager(int newRate); // initializes the manager with a given fps rate
	void init(int newRate); // needs to be called directly before the main game loop to reset the internal timer
	void framerateDelay(); // needs to be called every game update cycle
	ui32 getElapsedMilliseconds() const {return this->timeElapsed;}
	ui32 getFrameNumber() const { return accumulatedFrames; }
	ui32 getFramerate() const { return fps; };
};
