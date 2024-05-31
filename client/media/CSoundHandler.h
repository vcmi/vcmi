/*
 * CSoundHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CAudioBase.h"
#include "ISoundPlayer.h"

#include "../lib/CConfigHandler.h"

struct Mix_Chunk;

class CSoundHandler final : public CAudioBase, public ISoundPlayer
{
private:
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode & volumeNode);

	using CachedChunk = std::pair<Mix_Chunk *, std::unique_ptr<ui8[]>>;
	std::map<AudioPath, CachedChunk> soundChunks;
	std::map<std::vector<ui8>, CachedChunk> soundChunksRaw;

	Mix_Chunk * GetSoundChunk(const AudioPath & sound, bool cache);
	Mix_Chunk * GetSoundChunk(std::pair<std::unique_ptr<ui8[]>, si64> & data, bool cache);

	/// have entry for every currently active channel
	/// vector will be empty if callback was not set
	std::map<int, std::vector<std::function<void()>>> callbacks;

	/// Protects access to callbacks member to avoid data races:
	/// SDL calls sound finished callbacks from audio thread
	boost::mutex mutexCallbacks;

	int ambientDistToVolume(int distance) const;
	void ambientStopSound(const AudioPath & soundId);
	void updateChannelVolume(int channel);

	const JsonNode ambientConfig;

	boost::mutex mutex;
	std::map<AudioPath, int> ambientChannels;
	std::map<int, int> channelVolumes;
	int volume = 0;

	void initCallback(int channel, const std::function<void()> & function);
	void initCallback(int channel);

public:
	CSoundHandler();
	~CSoundHandler();

	ui32 getVolume() const final;
	void setVolume(ui32 percent) final;
	void setChannelVolume(int channel, ui32 percent);

	// Sounds
	uint32_t getSoundDurationMilliseconds(const AudioPath & sound) final;
	int playSound(soundBase::soundID soundID, int repeats = 0) final;
	int playSound(const AudioPath & sound, int repeats = 0, bool cache = false) final;
	int playSound(std::pair<std::unique_ptr<ui8[]>, si64> & data, int repeats = 0, bool cache = false) final;
	int playSoundFromSet(std::vector<soundBase::soundID> & sound_vec) final;
	void stopSound(int handler) final;

	void setCallback(int channel, std::function<void()> function) final;
	void resetCallback(int channel) final;
	void soundFinishedCallback(int channel) final;

	int ambientGetRange() const final;
	void ambientUpdateChannels(std::map<AudioPath, int> currentSounds) final;
	void ambientStopAllChannels() final;
};
