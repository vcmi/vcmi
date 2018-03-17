/*
 * CMusicHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include <SDL_mixer.h>

#include "CMusicHandler.h"
#include "CGameInfo.h"
#include "SDLRWwrapper.h"
#include "../lib/JsonNode.h"
#include "../lib/GameConstants.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/StringConstants.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/VCMIDirs.h"

#define VCMI_SOUND_NAME(x)
#define VCMI_SOUND_FILE(y) #y,

// sounds mapped to soundBase enum
static std::string sounds[] = {
    "", // invalid
    "", // todo
	VCMI_SOUND_LIST
};
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

// Not pretty, but there's only one music handler object in the game.
static void soundFinishedCallbackC(int channel)
{
	CCS->soundh->soundFinishedCallback(channel);
}

static void musicFinishedCallbackC()
{
	CCS->musich->musicFinishedCallback();
}

void CAudioBase::init()
{
	if (initialized)
		return;

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		logGlobal->error("Mix_OpenAudio error: %s", Mix_GetError());
		return;
	}

	initialized = true;
}

void CAudioBase::release()
{
	if(!(CCS->soundh->initialized && CCS->musich->initialized))
		Mix_CloseAudio();

	initialized = false;
}

void CAudioBase::setVolume(ui32 percent)
{
	if (percent > 100)
		percent = 100;

	volume = percent;
}

void CSoundHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume(volumeNode.Float());
}

CSoundHandler::CSoundHandler():
	listener(settings.listen["general"]["sound"]),
	ambientConfig(JsonNode(ResourceID("config/ambientSounds.json")))
{
	allTilesSource = ambientConfig["allTilesSource"].Bool();
	listener(std::bind(&CSoundHandler::onVolumeChange, this, _1));

	// Vectors for helper(s)
	pickupSounds =
	{
		soundBase::pickup01, soundBase::pickup02, soundBase::pickup03,
		soundBase::pickup04, soundBase::pickup05, soundBase::pickup06, soundBase::pickup07
	};

    horseSounds =  // must be the same order as terrains (see ETerrainType);
    {
		soundBase::horseDirt, soundBase::horseSand, soundBase::horseGrass,
		soundBase::horseSnow, soundBase::horseSwamp, soundBase::horseRough,
		soundBase::horseSubterranean, soundBase::horseLava,
		soundBase::horseWater, soundBase::horseRock
    };

	battleIntroSounds =
	{
		soundBase::battle00, soundBase::battle01,
	    soundBase::battle02, soundBase::battle03, soundBase::battle04,
	    soundBase::battle05, soundBase::battle06, soundBase::battle07
	};
};

void CSoundHandler::init()
{
	CAudioBase::init();
	if(ambientConfig["allocateChannels"].isNumber())
		Mix_AllocateChannels(ambientConfig["allocateChannels"].Integer());

	if (initialized)
	{
		// Load sounds
		Mix_ChannelFinished(soundFinishedCallbackC);
	}
}

void CSoundHandler::release()
{
	if (initialized)
	{
		Mix_HaltChannel(-1);

		for (auto &chunk : soundChunks)
		{
			if (chunk.second.first)
				Mix_FreeChunk(chunk.second.first);
		}
	}

	CAudioBase::release();
}

// Allocate an SDL chunk and cache it.
Mix_Chunk *CSoundHandler::GetSoundChunk(std::string &sound, bool cache)
{
	try
	{
		if (cache && soundChunks.find(sound) != soundChunks.end())
			return soundChunks[sound].first;

		auto data = CResourceHandler::get()->load(ResourceID(std::string("SOUNDS/") + sound, EResType::SOUND))->readAll();
		SDL_RWops *ops = SDL_RWFromMem(data.first.get(), data.second);
		Mix_Chunk *chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

		if (cache)
			soundChunks.insert(std::pair<std::string, CachedChunk>(sound, std::make_pair (chunk, std::move (data.first))));

		return chunk;
	}
	catch(std::exception &e)
	{
		logGlobal->warn("Cannot get sound %s chunk: %s", sound, e.what());
		return nullptr;
	}
}

int CSoundHandler::ambientDistToVolume(int distance) const
{
	if(distance >= ambientConfig["distances"].Vector().size())
		return 0;

	int volume = ambientConfig["distances"].Vector()[distance].Integer();
	return volume * ambientConfig["volume"].Integer() * getVolume() / 10000;
}

void CSoundHandler::ambientStopSound(std::string soundId)
{
	stopSound(ambientChannels[soundId]);
	setChannelVolume(ambientChannels[soundId], volume);
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	assert(soundID < soundBase::sound_after_last);
	auto sound = sounds[soundID];
	logGlobal->trace("Attempt to play sound %d with file name %s with cache", soundID, sound);

	return playSound(sound, repeats, true);
}

int CSoundHandler::playSound(std::string sound, int repeats, bool cache)
{
	if (!initialized || sound.empty())
		return -1;

	int channel;
	Mix_Chunk *chunk = GetSoundChunk(sound, cache);

	if (chunk)
	{
		channel = Mix_PlayChannel(-1, chunk, repeats);
		if (channel == -1)
		{
			logGlobal->error("Unable to play sound file %s , error %s", sound, Mix_GetError());
			if (!cache)
				Mix_FreeChunk(chunk);
		}
		else if (cache)
			callbacks[channel];
		else
			callbacks[channel] = [chunk](){ Mix_FreeChunk(chunk);};
	}
	else
		channel = -1;

	return channel;
}

// Helper. Randomly select a sound from an array and play it
int CSoundHandler::playSoundFromSet(std::vector<soundBase::soundID> &sound_vec)
{
	return playSound(*RandomGeneratorUtil::nextItem(sound_vec, CRandomGenerator::getDefault()));
}

void CSoundHandler::stopSound( int handler )
{
	if (initialized && handler != -1)
		Mix_HaltChannel(handler);
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setVolume(ui32 percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		setChannelVolume(-1, volume);
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setChannelVolume(int channel, ui32 percent)
{
	Mix_Volume(channel, (MIX_MAX_VOLUME * percent)/100);
}

void CSoundHandler::setCallback(int channel, std::function<void()> function)
{
	std::map<int, std::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	//channel not found. It may have finished so fire callback now
	if(iter == callbacks.end())
		function();
	else
		iter->second = function;
}

void CSoundHandler::soundFinishedCallback(int channel)
{
	std::map<int, std::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	assert(iter != callbacks.end());

	if (iter->second)
		iter->second();

	callbacks.erase(iter);
}

int CSoundHandler::ambientGetRange() const
{
	return ambientConfig["range"].Integer();
}

bool CSoundHandler::ambientCheckVisitable() const
{
	return !allTilesSource;
}

void CSoundHandler::ambientUpdateChannels(std::map<std::string, int> soundsArg)
{
	boost::mutex::scoped_lock guard(mutex);

	std::vector<std::string> stoppedSounds;
	for(auto & pair : ambientChannels)
	{
		if(!vstd::contains(soundsArg, pair.first))
		{
			ambientStopSound(pair.first);
			stoppedSounds.push_back(pair.first);
		}
		else
		{
			CCS->soundh->setChannelVolume(pair.second, ambientDistToVolume(soundsArg[pair.first]));
		}
	}
	for(auto soundId : stoppedSounds)
		ambientChannels.erase(soundId);

	for(auto & pair : soundsArg)
	{
		if(!vstd::contains(ambientChannels, pair.first))
		{
			int channel = CCS->soundh->playSound(pair.first, -1);
			CCS->soundh->setChannelVolume(channel, ambientDistToVolume(pair.second));
			CCS->soundh->ambientChannels.insert(std::make_pair(pair.first, channel));
		}
	}
}

void CSoundHandler::ambientStopAllChannels()
{
	boost::mutex::scoped_lock guard(mutex);

	for(auto ch : ambientChannels)
	{
		ambientStopSound(ch.first);
	}
	ambientChannels.clear();
}

void CMusicHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume(volumeNode.Float());
}

CMusicHandler::CMusicHandler():
	listener(settings.listen["general"]["music"])
{
	listener(std::bind(&CMusicHandler::onVolumeChange, this, _1));

	auto mp3files = CResourceHandler::get()->getFilteredFiles([](const ResourceID & id) ->  bool
	{
		if(id.getType() != EResType::MUSIC)
			return false;

		if(!boost::algorithm::istarts_with(id.getName(), "MUSIC/"))
			return false;

		logGlobal->trace("Found music file %s", id.getName());
		return true;
	});

	int battleMusicID = 0;
	int AIThemeID = 0;

	for(const ResourceID & file : mp3files)
	{
		if(boost::algorithm::istarts_with(file.getName(), "MUSIC/Combat"))
			addEntryToSet("battle", battleMusicID++, file.getName());
		else if(boost::algorithm::istarts_with(file.getName(), "MUSIC/AITheme"))
			addEntryToSet("enemy-turn", AIThemeID++, file.getName());
	}

	JsonNode terrains(ResourceID("config/terrains.json"));
	for (auto entry : terrains.Struct())
	{
		int terrIndex = vstd::find_pos(GameConstants::TERRAIN_NAMES, entry.first);
		addEntryToSet("terrain", terrIndex, "Music/" + entry.second["music"].String());
	}
}

void CMusicHandler::addEntryToSet(std::string set, int musicID, std::string musicURI)
{
	musicsSet[set][musicID] = musicURI;
}

void CMusicHandler::init()
{
	CAudioBase::init();

	if (initialized)
		Mix_HookMusicFinished(musicFinishedCallbackC);
}

void CMusicHandler::release()
{
	if (initialized)
	{
		boost::mutex::scoped_lock guard(mutex);

		Mix_HookMusicFinished(nullptr);

		current.reset();
		next.reset();
	}

	CAudioBase::release();
}

void CMusicHandler::playMusic(std::string musicURI, bool loop)
{
	if (current && current->isTrack(musicURI))
		return;

	queueNext(this, "", musicURI, loop);
}

void CMusicHandler::playMusicFromSet(std::string whichSet, bool loop)
{
	auto selectedSet = musicsSet.find(whichSet);
	if (selectedSet == musicsSet.end())
	{
		logGlobal->error("Error: playing music from non-existing set: %s", whichSet);
		return;
	}

	if (current && current->isSet(whichSet))
		return;

	// in this mode - play random track from set
	queueNext(this, whichSet, "", loop);
}


void CMusicHandler::playMusicFromSet(std::string whichSet, int entryID, bool loop)
{
	auto selectedSet = musicsSet.find(whichSet);
	if (selectedSet == musicsSet.end())
	{
		logGlobal->error("Error: playing music from non-existing set: %s", whichSet);
		return;
	}

	auto selectedEntry = selectedSet->second.find(entryID);
	if (selectedEntry == selectedSet->second.end())
	{
		logGlobal->error("Error: playing non-existing entry %d from set: %s", entryID, whichSet);
		return;
	}

	if (current && current->isTrack(selectedEntry->second))
		return;

	// in this mode - play specific track from set
	queueNext(this, "", selectedEntry->second, loop);
}

void CMusicHandler::queueNext(std::unique_ptr<MusicEntry> queued)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(mutex);

	next = std::move(queued);

	if (current.get() == nullptr || !current->stop(1000))
	{
		current.reset(next.release());
		current->play();
	}
}

void CMusicHandler::queueNext(CMusicHandler *owner, std::string setName, std::string musicURI, bool looped)
{
	try
	{
		queueNext(make_unique<MusicEntry>(owner, setName, musicURI, looped));
	}
	catch(std::exception &e)
	{
		logGlobal->error("Failed to queue music. setName=%s\tmusicURI=%s", setName, musicURI);
		logGlobal->error("Exception: %s", e.what());
	}
}

void CMusicHandler::stopMusic(int fade_ms)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(mutex);

	if (current.get() != nullptr)
		current->stop(fade_ms);
	next.reset();
}

void CMusicHandler::setVolume(ui32 percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		Mix_VolumeMusic((MIX_MAX_VOLUME * volume)/100);
}

void CMusicHandler::musicFinishedCallback()
{
	boost::mutex::scoped_lock guard(mutex);

	if (current.get() != nullptr)
	{
		//return if current music still not finished
		if (current->play())
			return;
		else
			current.reset();
	}

	if (current.get() == nullptr && next.get() != nullptr)
	{
		current.reset(next.release());
		current->play();
	}
}

MusicEntry::MusicEntry(CMusicHandler *owner, std::string setName, std::string musicURI, bool looped):
	owner(owner),
	music(nullptr),
	loop(looped ? -1 : 1),
    setName(std::move(setName))
{
	if (!musicURI.empty())
		load(std::move(musicURI));
}
MusicEntry::~MusicEntry()
{
	logGlobal->trace("Del-ing music file %s", currentName);
	if (music)
		Mix_FreeMusic(music);
}

void MusicEntry::load(std::string musicURI)
{
	if (music)
	{
		logGlobal->trace("Del-ing music file %s", currentName);
		Mix_FreeMusic(music);
		music = nullptr;
	}

	currentName = musicURI;

	logGlobal->trace("Loading music file %s", musicURI);

	auto musicFile = MakeSDLRWops(CResourceHandler::get()->load(ResourceID(std::move(musicURI), EResType::MUSIC)));

	music = Mix_LoadMUS_RW(musicFile, SDL_TRUE);

	if(!music)
	{
		logGlobal->warn("Warning: Cannot open %s: %s", currentName, Mix_GetError());
		return;
	}
}

bool MusicEntry::play()
{
	if (!(loop--) && music) //already played once - return
		return false;

	if (!setName.empty())
	{
		auto set = owner->musicsSet[setName];
		load(RandomGeneratorUtil::nextItem(set, CRandomGenerator::getDefault())->second);
	}

	logGlobal->trace("Playing music file %s", currentName);
	if(Mix_PlayMusic(music, 1) == -1)
	{
		logGlobal->error("Unable to play music (%s)", Mix_GetError());
		return false;
	}
	return true;
}

bool MusicEntry::stop(int fade_ms)
{
	if (Mix_PlayingMusic())
	{
		logGlobal->trace("Stopping music file %s", currentName);
		loop = 0;
		Mix_FadeOutMusic(fade_ms);
		return true;
	}
	return false;
}

bool MusicEntry::isSet(std::string set)
{
	return !setName.empty() && set == setName;
}

bool MusicEntry::isTrack(std::string track)
{
	return setName.empty() && track == currentName;
}
