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
#include <SDL_timer.h>

#include "CMusicHandler.h"
#include "CGameInfo.h"
#include "renderSDL/SDLRWwrapper.h"
#include "eventsSDL/InputHandler.h"
#include "gui/CGuiHandler.h"

#include "../lib/JsonNode.h"
#include "../lib/GameConstants.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/constants/StringConstants.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/TerrainHandler.h"


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
	setVolume((ui32)volumeNode.Float());
}

CSoundHandler::CSoundHandler():
	listener(settings.listen["general"]["sound"]),
	ambientConfig(JsonPath::builtin("config/ambientSounds.json"))
{
	listener(std::bind(&CSoundHandler::onVolumeChange, this, _1));

	battleIntroSounds =
	{
		soundBase::battle00, soundBase::battle01,
		soundBase::battle02, soundBase::battle03, soundBase::battle04,
		soundBase::battle05, soundBase::battle06, soundBase::battle07
	};
}

void CSoundHandler::init()
{
	CAudioBase::init();
	if(ambientConfig["allocateChannels"].isNumber())
		Mix_AllocateChannels((int)ambientConfig["allocateChannels"].Integer());

	if (initialized)
	{
		Mix_ChannelFinished([](int channel)
		{
			CCS->soundh->soundFinishedCallback(channel);
		});
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
Mix_Chunk *CSoundHandler::GetSoundChunk(const AudioPath & sound, bool cache)
{
	try
	{
		if (cache && soundChunks.find(sound) != soundChunks.end())
			return soundChunks[sound].first;

		auto data = CResourceHandler::get()->load(sound.addPrefix("SOUNDS/"))->readAll();
		SDL_RWops *ops = SDL_RWFromMem(data.first.get(), (int)data.second);
		Mix_Chunk *chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

		if (cache)
			soundChunks.insert({sound, std::make_pair (chunk, std::move (data.first))});

		return chunk;
	}
	catch(std::exception &e)
	{
		logGlobal->warn("Cannot get sound %s chunk: %s", sound.getOriginalName(), e.what());
		return nullptr;
	}
}

Mix_Chunk *CSoundHandler::GetSoundChunk(std::pair<std::unique_ptr<ui8 []>, si64> & data, bool cache)
{
	try
	{
		std::vector<ui8> startBytes = std::vector<ui8>(data.first.get(), data.first.get() + std::min((si64)100, data.second));

		if (cache && soundChunksRaw.find(startBytes) != soundChunksRaw.end())
			return soundChunksRaw[startBytes].first;

		SDL_RWops *ops = SDL_RWFromMem(data.first.get(), (int)data.second);
		Mix_Chunk *chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

		if (cache)
			soundChunksRaw.insert({startBytes, std::make_pair (chunk, std::move (data.first))});

		return chunk;
	}
	catch(std::exception &e)
	{
		logGlobal->warn("Cannot get sound chunk: %s", e.what());
		return nullptr;
	}
}

int CSoundHandler::ambientDistToVolume(int distance) const
{
	const auto & distancesVector = ambientConfig["distances"].Vector();

	if(distance >= distancesVector.size())
		return 0;

	int volume = static_cast<int>(distancesVector[distance].Integer());
	return volume * (int)ambientConfig["volume"].Integer() / 100;
}

void CSoundHandler::ambientStopSound(const AudioPath & soundId)
{
	stopSound(ambientChannels[soundId]);
	setChannelVolume(ambientChannels[soundId], volume);
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	assert(soundID < soundBase::sound_after_last);
	auto sound = AudioPath::builtin(sounds[soundID]);
	logGlobal->trace("Attempt to play sound %d with file name %s with cache", soundID, sound.getOriginalName());

	return playSound(sound, repeats, true);
}

int CSoundHandler::playSound(const AudioPath & sound, int repeats, bool cache)
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
			logGlobal->error("Unable to play sound file %s , error %s", sound.getOriginalName(), Mix_GetError());
			if (!cache)
				Mix_FreeChunk(chunk);
		}
		else if (cache)
			initCallback(channel);
		else
			initCallback(channel, [chunk](){ Mix_FreeChunk(chunk);});
	}
	else
		channel = -1;

	return channel;
}

int CSoundHandler::playSound(std::pair<std::unique_ptr<ui8 []>, si64> & data, int repeats, bool cache)
{
	int channel = -1;
	if (Mix_Chunk *chunk = GetSoundChunk(data, cache))
	{
		channel = Mix_PlayChannel(-1, chunk, repeats);
		if (channel == -1)
		{
			logGlobal->error("Unable to play sound, error %s", Mix_GetError());
			if (!cache)
				Mix_FreeChunk(chunk);
		}
		else if (cache)
			initCallback(channel);
		else
			initCallback(channel, [chunk](){ Mix_FreeChunk(chunk);});
	}
	return channel;
}

// Helper. Randomly select a sound from an array and play it
int CSoundHandler::playSoundFromSet(std::vector<soundBase::soundID> &sound_vec)
{
	return playSound(*RandomGeneratorUtil::nextItem(sound_vec, CRandomGenerator::getDefault()));
}

void CSoundHandler::stopSound(int handler)
{
	if (initialized && handler != -1)
		Mix_HaltChannel(handler);
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setVolume(ui32 percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
	{
		setChannelVolume(-1, volume);

		for (auto const & channel : channelVolumes)
			updateChannelVolume(channel.first);
	}
}

void CSoundHandler::updateChannelVolume(int channel)
{
	if (channelVolumes.count(channel))
		setChannelVolume(channel, getVolume() * channelVolumes[channel] / 100);
	else
		setChannelVolume(channel, getVolume());
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setChannelVolume(int channel, ui32 percent)
{
	Mix_Volume(channel, (MIX_MAX_VOLUME * percent)/100);
}

void CSoundHandler::setCallback(int channel, std::function<void()> function)
{
	boost::mutex::scoped_lock lockGuard(mutexCallbacks);

	auto iter = callbacks.find(channel);

	//channel not found. It may have finished so fire callback now
	if(iter == callbacks.end())
		function();
	else
		iter->second.push_back(function);
}

void CSoundHandler::soundFinishedCallback(int channel)
{
	boost::mutex::scoped_lock lockGuard(mutexCallbacks);

	if (callbacks.count(channel) == 0)
		return;

	// store callbacks from container locally - SDL might reuse this channel for another sound
	// but do actualy execution in separate thread, to avoid potential deadlocks in case if callback requires locks of its own
	auto callback = callbacks.at(channel);
	callbacks.erase(channel);

	if (!callback.empty())
	{
		GH.dispatchMainThread([callback](){
			for (auto entry : callback)
				entry();
		});
	}
}

void CSoundHandler::initCallback(int channel)
{
	boost::mutex::scoped_lock lockGuard(mutexCallbacks);
	assert(callbacks.count(channel) == 0);
	callbacks[channel] = {};
}

void CSoundHandler::initCallback(int channel, const std::function<void()> & function)
{
	boost::mutex::scoped_lock lockGuard(mutexCallbacks);
	assert(callbacks.count(channel) == 0);
	callbacks[channel].push_back(function);
}

int CSoundHandler::ambientGetRange() const
{
	return static_cast<int>(ambientConfig["range"].Integer());
}

void CSoundHandler::ambientUpdateChannels(std::map<AudioPath, int> soundsArg)
{
	boost::mutex::scoped_lock guard(mutex);

	std::vector<AudioPath> stoppedSounds;
	for(auto & pair : ambientChannels)
	{
		const auto & soundId = pair.first;
		const int channel = pair.second;

		if(!vstd::contains(soundsArg, soundId))
		{
			ambientStopSound(soundId);
			stoppedSounds.push_back(soundId);
		}
		else
		{
			int volume = ambientDistToVolume(soundsArg[soundId]);
			channelVolumes[channel] = volume;
			updateChannelVolume(channel);
		}
	}
	for(auto soundId : stoppedSounds)
	{
		channelVolumes.erase(ambientChannels[soundId]);
		ambientChannels.erase(soundId);
	}

	for(auto & pair : soundsArg)
	{
		const auto & soundId = pair.first;
		const int distance = pair.second;

		if(!vstd::contains(ambientChannels, soundId))
		{
			int channel = playSound(soundId, -1);
			int volume = ambientDistToVolume(distance);
			channelVolumes[channel] = volume;

			updateChannelVolume(channel);
			ambientChannels[soundId] = channel;
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
	channelVolumes.clear();
	ambientChannels.clear();
}

void CMusicHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume((ui32)volumeNode.Float());
}

CMusicHandler::CMusicHandler():
	listener(settings.listen["general"]["music"])
{
	listener(std::bind(&CMusicHandler::onVolumeChange, this, _1));

	auto mp3files = CResourceHandler::get()->getFilteredFiles([](const ResourcePath & id) ->  bool
	{
		if(id.getType() != EResType::SOUND)
			return false;

		if(!boost::algorithm::istarts_with(id.getName(), "MUSIC/"))
			return false;

		logGlobal->trace("Found music file %s", id.getName());
		return true;
	});

	for(const ResourcePath & file : mp3files)
	{
		if(boost::algorithm::istarts_with(file.getName(), "MUSIC/Combat"))
			addEntryToSet("battle", AudioPath::fromResource(file));
		else if(boost::algorithm::istarts_with(file.getName(), "MUSIC/AITheme"))
			addEntryToSet("enemy-turn", AudioPath::fromResource(file));
	}

}

void CMusicHandler::loadTerrainMusicThemes()
{
	for (const auto & terrain : CGI->terrainTypeHandler->objects)
	{
		addEntryToSet("terrain_" + terrain->getJsonKey(), terrain->musicFilename);
	}
}

void CMusicHandler::addEntryToSet(const std::string & set, const AudioPath & musicURI)
{
	musicsSet[set].push_back(musicURI);
}

void CMusicHandler::init()
{
	CAudioBase::init();

	if (initialized)
	{
		Mix_HookMusicFinished([]()
		{
			CCS->musich->musicFinishedCallback();
		});
	}
}

void CMusicHandler::release()
{
	if (initialized)
	{
		boost::mutex::scoped_lock guard(mutex);

		Mix_HookMusicFinished(nullptr);
		current->stop();

		current.reset();
		next.reset();
	}

	CAudioBase::release();
}

void CMusicHandler::playMusic(const AudioPath & musicURI, bool loop, bool fromStart)
{
	boost::mutex::scoped_lock guard(mutex);

	if (current && current->isPlaying() && current->isTrack(musicURI))
		return;

	queueNext(this, "", musicURI, loop, fromStart);
}

void CMusicHandler::playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop, bool fromStart)
{
	playMusicFromSet(musicSet + "_" + entryID, loop, fromStart);
}

void CMusicHandler::playMusicFromSet(const std::string & whichSet, bool loop, bool fromStart)
{
	boost::mutex::scoped_lock guard(mutex);

	auto selectedSet = musicsSet.find(whichSet);
	if (selectedSet == musicsSet.end())
	{
		logGlobal->error("Error: playing music from non-existing set: %s", whichSet);
		return;
	}

	if (current && current->isPlaying() && current->isSet(whichSet))
		return;

	// in this mode - play random track from set
	queueNext(this, whichSet, AudioPath(), loop, fromStart);
}

void CMusicHandler::queueNext(std::unique_ptr<MusicEntry> queued)
{
	if (!initialized)
		return;

	next = std::move(queued);

	if (current.get() == nullptr || !current->stop(1000))
	{
		current.reset(next.release());
		current->play();
	}
}

void CMusicHandler::queueNext(CMusicHandler *owner, const std::string & setName, const AudioPath & musicURI, bool looped, bool fromStart)
{
	queueNext(std::make_unique<MusicEntry>(owner, setName, musicURI, looped, fromStart));
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
	// call music restart in separate thread to avoid deadlock in some cases
	// It is possible for:
	// 1) SDL thread to call this method on end of playback
	// 2) VCMI code to call queueNext() method to queue new file
	// this leads to:
	// 1) SDL thread waiting to acquire music lock in this method (while keeping internal SDL mutex locked)
	// 2) VCMI thread waiting to acquire internal SDL mutex (while keeping music mutex locked)

	GH.dispatchMainThread([this]()
	{
		boost::unique_lock lockGuard(mutex);
		if (current.get() != nullptr)
		{
			// if music is looped, play it again
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
	});
}

MusicEntry::MusicEntry(CMusicHandler *owner, std::string setName, const AudioPath & musicURI, bool looped, bool fromStart):
	owner(owner),
	music(nullptr),
	playing(false),
	startTime(uint32_t(-1)),
	startPosition(0),
	loop(looped ? -1 : 1),
	fromStart(fromStart),
	setName(std::move(setName))
{
	if (!musicURI.empty())
		load(std::move(musicURI));
}
MusicEntry::~MusicEntry()
{
	if (playing && loop > 0)
	{
		assert(0);
		logGlobal->error("Attempt to delete music while playing!");
		Mix_HaltMusic();
	}

	if (loop == 0 && Mix_FadingMusic() != MIX_NO_FADING)
	{
		assert(0);
		logGlobal->error("Attempt to delete music while fading out!");
		Mix_HaltMusic();
	}

	logGlobal->trace("Del-ing music file %s", currentName.getOriginalName());
	if (music)
		Mix_FreeMusic(music);
}

void MusicEntry::load(const AudioPath & musicURI)
{
	if (music)
	{
		logGlobal->trace("Del-ing music file %s", currentName.getOriginalName());
		Mix_FreeMusic(music);
		music = nullptr;
	}

	if (CResourceHandler::get()->existsResource(musicURI))
		currentName = musicURI;
	else
		currentName = musicURI.addPrefix("MUSIC/");

	music = nullptr;

	logGlobal->trace("Loading music file %s", currentName.getOriginalName());

	try
	{
		auto musicFile = MakeSDLRWops(CResourceHandler::get()->load(currentName));
		music = Mix_LoadMUS_RW(musicFile, SDL_TRUE);
	}
	catch(std::exception &e)
	{
		logGlobal->error("Failed to load music. setName=%s\tmusicURI=%s", setName, currentName.getOriginalName());
		logGlobal->error("Exception: %s", e.what());
	}

	if(!music)
	{
		logGlobal->warn("Warning: Cannot open %s: %s", currentName.getOriginalName(), Mix_GetError());
		return;
	}
}

bool MusicEntry::play()
{
	if (!(loop--) && music) //already played once - return
		return false;

	if (!setName.empty())
	{
		const auto & set = owner->musicsSet[setName];
		const auto & iter = RandomGeneratorUtil::nextItem(set, CRandomGenerator::getDefault());
		load(*iter);
	}

	logGlobal->trace("Playing music file %s", currentName.getOriginalName());

	if (!fromStart && owner->trackPositions.count(currentName) > 0 && owner->trackPositions[currentName] > 0)
	{
		float timeToStart = owner->trackPositions[currentName];
		startPosition = std::round(timeToStart * 1000);

		// erase stored position:
		// if music track will be interrupted again - new position will be written in stop() method
		// if music track is not interrupted and will finish by timeout/end of file - it will restart from begginning as it should
		owner->trackPositions.erase(owner->trackPositions.find(currentName));

		if (Mix_FadeInMusicPos(music, 1, 1000, timeToStart) == -1)
		{
			logGlobal->error("Unable to play music (%s)", Mix_GetError());
			return false;
		}
	}
	else
	{
		startPosition = 0;

		if(Mix_PlayMusic(music, 1) == -1)
		{
			logGlobal->error("Unable to play music (%s)", Mix_GetError());
			return false;
		}
	}

	startTime = GH.input().getTicks();
	
	playing = true;
	return true;
}

bool MusicEntry::stop(int fade_ms)
{
	if (Mix_PlayingMusic())
	{
		playing = false;
		loop = 0;
		uint32_t endTime = GH.input().getTicks();
		assert(startTime != uint32_t(-1));
		float playDuration = (endTime - startTime + startPosition) / 1000.f;
		owner->trackPositions[currentName] = playDuration;
		logGlobal->trace("Stopping music file %s at %f", currentName.getOriginalName(), playDuration);

		Mix_FadeOutMusic(fade_ms);
		return true;
	}
	return false;
}

bool MusicEntry::isPlaying()
{
	return playing;
}

bool MusicEntry::isSet(std::string set)
{
	return !setName.empty() && set == setName;
}

bool MusicEntry::isTrack(const AudioPath & track)
{
	return setName.empty() && track == currentName;
}
