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
	struct MixChunkDeleter
	{
		void operator ()(Mix_Chunk *);
	};
	using MixChunkPtr = std::unique_ptr<Mix_Chunk, MixChunkDeleter>;

	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode & volumeNode);

	using CachedChunk = std::pair<MixChunkPtr, std::unique_ptr<ui8[]>>;

	/// List of all permanently cached sound chunks
	std::map<AudioPath, CachedChunk> soundChunks;

	/// List of all currently playing chunks that are currently playing
	/// and should be deallocated once channel playback is over
	/// indexed by channel ID
	std::map<int, MixChunkPtr> uncachedPlayingChunks;

	MixChunkPtr getSoundChunk(const AudioPath & sound);
	Mix_Chunk * getSoundChunkCached(const AudioPath & sound);
	MixChunkPtr getSoundChunk(std::pair<std::unique_ptr<ui8[]>, si64> & data);

	/// have entry for every currently active channel
	/// vector will be empty if callback was not set
	std::map<int, std::vector<std::function<void()>>> callbacks;

	/// Protects access to callbacks member to avoid data races:
	/// SDL calls sound finished callbacks from audio thread
	std::mutex mutexCallbacks;

	int ambientDistToVolume(int distance) const;
	void ambientStopSound(const AudioPath & soundId);
	void updateChannelVolume(int channel);

	const JsonNode ambientConfig;

	std::mutex mutex;
	std::map<AudioPath, int> ambientChannels;
	std::map<int, int> channelVolumes;
	int volume = 0;

	void initCallback(int channel, const std::function<void()> & function);
	void initCallback(int channel);
	void storeChunk(int channel, MixChunkPtr chunk);
	int playSoundImpl(const AudioPath & sound, int repeats, bool useCache);

public:
	CSoundHandler();
	~CSoundHandler();

	ui32 getVolume() const final;
	void setVolume(ui32 percent) final;
	void setChannelVolume(int channel, ui32 percent);

	// Sounds
	uint32_t getSoundDurationMilliseconds(const AudioPath & sound) final;
	int playSound(soundBase::soundID soundID) final;
	int playSound(const AudioPath & sound) final;
	int playSoundLooped(const AudioPath & sound) final;
	int playSound(std::pair<std::unique_ptr<ui8[]>, si64> & data) final;
	int playSoundFromSet(std::vector<soundBase::soundID> & sound_vec) final;
	void stopSound(int handler) final;
	void pauseSound(int handler) final;
	void resumeSound(int handler) final;

	void setCallback(int channel, std::function<void()> function) final;
	void resetCallback(int channel) final;
	void soundFinishedCallback(int channel) final;

	int ambientGetRange() const final;
	void ambientUpdateChannels(std::map<AudioPath, int> currentSounds) final;
	void ambientStopAllChannels() final;
};
