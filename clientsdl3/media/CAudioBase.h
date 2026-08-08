/*
 * CAudioBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct MIX_Mixer;

class CAudioBase : boost::noncopyable
{
	static int initializationCounter;
	static bool initializeSuccess;
	static MIX_Mixer * mixer;

protected:
	bool isInitialized() const;

	/// SDL3_mixer routes all playback through an explicit mixer object shared by music and sounds
	static MIX_Mixer * getMixer();

	CAudioBase();
	~CAudioBase();
};
