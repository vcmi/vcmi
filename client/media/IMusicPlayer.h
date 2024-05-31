/*
 * IMusicPlayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/filesystem/ResourcePath.h"

class IMusicPlayer
{
public:
	virtual ~IMusicPlayer() = default;

	virtual void loadTerrainMusicThemes() = 0;
	virtual void setVolume(ui32 percent) = 0;
	virtual ui32 getVolume() const = 0;

	virtual void musicFinishedCallback() = 0;

	/// play track by URI, if loop = true music will be looped
	virtual void playMusic(const AudioPath & musicURI, bool loop, bool fromStart) = 0;
	/// play random track from this set
	virtual void playMusicFromSet(const std::string & musicSet, bool loop, bool fromStart) = 0;
	/// play random track from set (musicSet, entryID)
	virtual void playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop, bool fromStart) = 0;
	/// stops currently playing music by fading out it over fade_ms and starts next scheduled track, if any
	virtual void stopMusic(int fade_ms = 1000) = 0;
};
