/*
 * CMusicHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/CConfigHandler.h"
#include "../lib/CSoundBase.h"

struct _Mix_Music;
struct SDL_RWops;
using Mix_Music = struct _Mix_Music;
struct Mix_Chunk;

class CAudioBase {
protected:
	boost::mutex mutex;
	bool initialized;
	int volume;					// from 0 (mute) to 100

	CAudioBase(): initialized(false), volume(0) {};
	~CAudioBase() = default;
public:
	virtual void init() = 0;
	virtual void release() = 0;

	virtual void setVolume(ui32 percent);
	ui32 getVolume() const { return volume; };
};

class CSoundHandler final : public CAudioBase
{
private:
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode &volumeNode);

	using CachedChunk = std::pair<Mix_Chunk *, std::unique_ptr<ui8[]>>;
	std::map<AudioPath, CachedChunk> soundChunks;
	std::map<std::vector<ui8>, CachedChunk> soundChunksRaw;

	Mix_Chunk *GetSoundChunk(const AudioPath & sound, bool cache);
	Mix_Chunk *GetSoundChunk(std::pair<std::unique_ptr<ui8 []>, si64> & data, bool cache);

	/// have entry for every currently active channel
	/// vector will be empty if callback was not set
	std::map<int, std::vector<std::function<void()>> > callbacks;

	/// Protects access to callbacks member to avoid data races:
	/// SDL calls sound finished callbacks from audio thread
	boost::mutex mutexCallbacks;

	int ambientDistToVolume(int distance) const;
	void ambientStopSound(const AudioPath & soundId);
	void updateChannelVolume(int channel);

	const JsonNode ambientConfig;

	std::map<AudioPath, int> ambientChannels;
	std::map<int, int> channelVolumes;

	void initCallback(int channel, const std::function<void()> & function);
	void initCallback(int channel);

public:
	CSoundHandler();

	void init() override;
	void release() override;

	void setVolume(ui32 percent) override;
	void setChannelVolume(int channel, ui32 percent);

	// Sounds
	int playSound(soundBase::soundID soundID, int repeats=0);
	int playSound(const AudioPath & sound, int repeats=0, bool cache=false);
	int playSound(std::pair<std::unique_ptr<ui8 []>, si64> & data, int repeats=0, bool cache=false);
	int playSoundFromSet(std::vector<soundBase::soundID> &sound_vec);
	void stopSound(int handler);

	void setCallback(int channel, std::function<void()> function);
	void soundFinishedCallback(int channel);

	int ambientGetRange() const;
	void ambientUpdateChannels(std::map<AudioPath, int> currentSounds);
	void ambientStopAllChannels();

	// Sets
	std::vector<soundBase::soundID> battleIntroSounds;
};

class CMusicHandler;

//Class for handling one music file
class MusicEntry
{
	CMusicHandler *owner;
	Mix_Music *music;

	int loop; // -1 = indefinite
	bool fromStart;
	bool playing;
	uint32_t startTime;
	uint32_t startPosition;
	//if not null - set from which music will be randomly selected
	std::string setName;
	AudioPath currentName;

	void load(const AudioPath & musicURI);

public:
	MusicEntry(CMusicHandler *owner, std::string setName, const AudioPath & musicURI, bool looped, bool fromStart);
	~MusicEntry();

	bool isSet(std::string setName);
	bool isTrack(const AudioPath & trackName);
	bool isPlaying();

	bool play();
	bool stop(int fade_ms=0);
};

class CMusicHandler final: public CAudioBase
{
private:
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode &volumeNode);

	std::unique_ptr<MusicEntry> current;
	std::unique_ptr<MusicEntry> next;

	void queueNext(CMusicHandler *owner, const std::string & setName, const AudioPath & musicURI, bool looped, bool fromStart);
	void queueNext(std::unique_ptr<MusicEntry> queued);
	void musicFinishedCallback();

	/// map <set name> -> <list of URI's to tracks belonging to the said set>
	std::map<std::string, std::vector<AudioPath>> musicsSet;
	/// stored position, in seconds at which music player should resume playing this track
	std::map<AudioPath, float> trackPositions;

public:
	CMusicHandler();

	/// add entry with URI musicURI in set. Track will have ID musicID
	void addEntryToSet(const std::string & set, const AudioPath & musicURI);

	void init() override;
	void loadTerrainMusicThemes();
	void release() override;
	void setVolume(ui32 percent) override;

	/// play track by URI, if loop = true music will be looped
	void playMusic(const AudioPath & musicURI, bool loop, bool fromStart);
	/// play random track from this set
	void playMusicFromSet(const std::string & musicSet, bool loop, bool fromStart);
	/// play random track from set (musicSet, entryID)
	void playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop, bool fromStart);
	/// stops currently playing music by fading out it over fade_ms and starts next scheduled track, if any
	void stopMusic(int fade_ms=1000);

	friend class MusicEntry;
};
