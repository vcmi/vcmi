/*
 * ISoundPlayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/CSoundBase.h"
#include "../lib/filesystem/ResourcePath.h"

class ISoundPlayer
{
public:
	virtual ~ISoundPlayer() = default;

	virtual int playSound(soundBase::soundID soundID) = 0;
	virtual int playSound(const AudioPath & sound) = 0;
	virtual int playSoundLooped(const AudioPath & sound) = 0;
	virtual int playSound(std::pair<std::unique_ptr<ui8[]>, si64> & data) = 0;
	virtual int playSoundFromSet(std::vector<soundBase::soundID> & sound_vec) = 0;
	virtual void stopSound(int handler) = 0;
	virtual void pauseSound(int handler) = 0;
	virtual void resumeSound(int handler) = 0;

	virtual ui32 getVolume() const = 0;
	virtual void setVolume(ui32 percent) = 0;
	virtual uint32_t getSoundDurationMilliseconds(const AudioPath & sound) = 0;
	virtual void setCallback(int channel, std::function<void()> function) = 0;
	virtual void resetCallback(int channel) = 0;
	virtual void soundFinishedCallback(int channel) = 0;
	virtual void ambientUpdateChannels(std::map<AudioPath, int> currentSounds) = 0;
	virtual void ambientStopAllChannels() = 0;
	virtual int ambientGetRange() const = 0;
};
