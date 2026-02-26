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

#include "CAudioBase.h"
#include "IMusicPlayer.h"

#include "../lib/CConfigHandler.h"

#include <SDL_mixer.h>

class CMusicHandler;

//Class for handling one music file
class MusicEntry : boost::noncopyable
{
	CMusicHandler * owner;
	Mix_Music * music;

	//if not null - set from which music will be randomly selected
	std::string setName;
	AudioPath currentName;

	uint32_t startTime;
	uint32_t startPosition;
	int loop; // -1 = indefinite
	bool fromStart;
	bool playing;

	void load(const AudioPath & musicURI);

public:
	MusicEntry(CMusicHandler * owner, std::string setName, const AudioPath & musicURI, bool looped, bool fromStart);
	~MusicEntry();

	bool isSet(const std::string & setName);
	bool isTrack(const AudioPath & trackName);
	bool isPlaying() const;

	bool play();
	bool stop(int fade_ms = 0);
};

class CMusicHandler final : public CAudioBase, public IMusicPlayer
{
private:
	//update volume on configuration change
	SettingsListener listener;
	void onVolumeChange(const JsonNode & volumeNode);

	std::unique_ptr<MusicEntry> current;
	std::unique_ptr<MusicEntry> next;

	std::mutex mutex;
	int volume = 0; // from 0 (mute) to 100

	void queueNext(CMusicHandler * owner, const std::string & setName, const AudioPath & musicURI, bool looped, bool fromStart);
	void queueNext(std::unique_ptr<MusicEntry> queued);
	void musicFinishedCallback() final;

	/// map <set name> -> <list of URI's to tracks belonging to the said set>
	std::map<std::string, std::vector<AudioPath>> musicsSet;
	/// stored position, in seconds at which music player should resume playing this track
	std::map<AudioPath, float> trackPositions;

public:
	CMusicHandler();
	~CMusicHandler();

	/// add entry with URI musicURI in set. Track will have ID musicID
	void addEntryToSet(const std::string & set, const AudioPath & musicURI);

	void loadTerrainMusicThemes() final;
	void setVolume(ui32 percent) final;
	ui32 getVolume() const final;

	/// play track by URI, if loop = true music will be looped
	void playMusic(const AudioPath & musicURI, bool loop, bool fromStart) final;
	/// play random track from this set
	void playMusicFromSet(const std::string & musicSet, bool loop, bool fromStart) final;
	/// play random track from set (musicSet, entryID)
	void playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop, bool fromStart) final;
	/// stops currently playing music by fading out it over fade_ms and starts next scheduled track, if any
	void stopMusic(int fade_ms) final;

	friend class MusicEntry;
};
