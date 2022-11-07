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
#include "../lib/Terrain.h"

struct _Mix_Music;
struct SDL_RWops;
typedef struct _Mix_Music Mix_Music;
struct Mix_Chunk;

class CAudioBase {
protected:
	boost::mutex mutex;
	bool initialized;
	int volume;					// from 0 (mute) to 100

public:
	CAudioBase(): initialized(false), volume(0) {};
	virtual void init() = 0;
	virtual void release() = 0;

	virtual void setVolume(ui32 percent);
	ui32 getVolume() const { return volume; };
};

class CSoundHandler: public CAudioBase
{
private:
	//soundBase::soundID getSoundID(const std::string &fileName);
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode &volumeNode);

	using CachedChunk = std::pair<Mix_Chunk *, std::unique_ptr<ui8[]>>;
	std::map<std::string, CachedChunk> soundChunks;

	Mix_Chunk *GetSoundChunk(std::string &sound, bool cache);

	//have entry for every currently active channel
	//std::function will be nullptr if callback was not set
	std::map<int, std::function<void()> > callbacks;

	int ambientDistToVolume(int distance) const;
	void ambientStopSound(std::string soundId);

	const JsonNode ambientConfig;
	bool allTilesSource;
	std::map<std::string, int> ambientChannels;

public:
	CSoundHandler();

	void init() override;
	void loadHorseSounds();
	void release() override;

	void setVolume(ui32 percent) override;
	void setChannelVolume(int channel, ui32 percent);

	// Sounds
	int playSound(soundBase::soundID soundID, int repeats=0);
	int playSound(std::string sound, int repeats=0, bool cache=false);
	int playSoundFromSet(std::vector<soundBase::soundID> &sound_vec);
	void stopSound(int handler);

	void setCallback(int channel, std::function<void()> function);
	void soundFinishedCallback(int channel);

	int ambientGetRange() const;
	bool ambientCheckVisitable() const;
	void ambientUpdateChannels(std::map<std::string, int> currentSounds);
	void ambientStopAllChannels();

	// Sets
	std::vector<soundBase::soundID> pickupSounds;
	std::vector<soundBase::soundID> battleIntroSounds;
	std::map<TerrainId, soundBase::soundID> horseSounds;
};

// Helper //now it looks somewhat useless
#define battle_sound(creature,what_sound) creature->sounds.what_sound

class CMusicHandler;

//Class for handling one music file
class MusicEntry
{
	CMusicHandler *owner;
	Mix_Music *music;

	int loop; // -1 = indefinite
	//if not null - set from which music will be randomly selected
	std::string setName;
	std::string currentName;


	void load(std::string musicURI);

public:
	bool isSet(std::string setName);
	bool isTrack(std::string trackName);

	MusicEntry(CMusicHandler *owner, std::string setName, std::string musicURI, bool looped);
	~MusicEntry();

	bool play();
	bool stop(int fade_ms=0);
};

class CMusicHandler: public CAudioBase
{
private:
	
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode &volumeNode);

	std::unique_ptr<MusicEntry> current;
	std::unique_ptr<MusicEntry> next;

	void queueNext(CMusicHandler *owner, const std::string & setName, const std::string & musicURI, bool looped);
	void queueNext(std::unique_ptr<MusicEntry> queued);

	std::map<std::string, std::map<std::string, std::string>> musicsSet;
public:
	
	CMusicHandler();

	/// add entry with URI musicURI in set. Track will have ID musicID
	void addEntryToSet(const std::string & set, const std::string & entryID, const std::string & musicURI);

	void init() override;
	void loadTerrainSounds();
	void release() override;
	void setVolume(ui32 percent) override;

	/// play track by URI, if loop = true music will be looped
	void playMusic(const std::string & musicURI, bool loop);
	/// play random track from this set
	void playMusicFromSet(const std::string & musicSet, bool loop);
	/// play specific track from set
	void playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop);
	void stopMusic(int fade_ms=1000);
	void musicFinishedCallback();

	friend class MusicEntry;
};
