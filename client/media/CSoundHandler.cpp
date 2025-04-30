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
#include "CSoundHandler.h"

#include "../GameEngine.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/CRandomGenerator.h"

#include <SDL_mixer.h>

#define VCMI_SOUND_NAME(x)
#define VCMI_SOUND_FILE(y) #y,

// sounds mapped to soundBase enum
static const std::string soundsList[] = {
	"", // invalid
	"", // todo
	VCMI_SOUND_LIST
};
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

void CSoundHandler::onVolumeChange(const JsonNode & volumeNode)
{
	setVolume(volumeNode.Integer());
}

CSoundHandler::CSoundHandler():
	listener(settings.listen["general"]["sound"]),
	ambientConfig(JsonPath::builtin("config/ambientSounds.json"))
{
	listener(std::bind(&CSoundHandler::onVolumeChange, this, _1));

	if(ambientConfig["allocateChannels"].isNumber())
		Mix_AllocateChannels(ambientConfig["allocateChannels"].Integer());

	if(isInitialized())
	{
		Mix_ChannelFinished([](int channel)
		{
			ENGINE->sound().soundFinishedCallback(channel);
		});
	}
}

void CSoundHandler::MixChunkDeleter::operator()(Mix_Chunk * ptr)
{
	Mix_FreeChunk(ptr);
}

CSoundHandler::~CSoundHandler()
{
	if(isInitialized())
	{
		Mix_ChannelFinished(nullptr);
		Mix_HaltChannel(-1);

		soundChunks.clear();
		uncachedPlayingChunks.clear();
	}
}

Mix_Chunk * CSoundHandler::getSoundChunkCached(const AudioPath & sound)
{
	if (soundChunks.find(sound) == soundChunks.end())
		soundChunks[sound].first = getSoundChunk(sound);

	return soundChunks[sound].first.get();
}

// Allocate an SDL chunk and cache it.
CSoundHandler::MixChunkPtr CSoundHandler::getSoundChunk(const AudioPath & sound)
{
	try
	{
		auto data = CResourceHandler::get()->load(sound.addPrefix("SOUNDS/"))->readAll();
		SDL_RWops * ops = SDL_RWFromMem(data.first.get(), data.second);
		Mix_Chunk * chunk = Mix_LoadWAV_RW(ops, 1); // will free ops
		return MixChunkPtr(chunk);
	}
	catch(std::exception & e)
	{
		logGlobal->warn("Cannot get sound %s chunk: %s", sound.getOriginalName(), e.what());
		return nullptr;
	}
}

CSoundHandler::MixChunkPtr CSoundHandler::getSoundChunk(std::pair<std::unique_ptr<ui8[]>, si64> & data)
{
	try
	{
		std::vector<ui8> startBytes = std::vector<ui8>(data.first.get(), data.first.get() + std::min(static_cast<si64>(100), data.second));

		SDL_RWops * ops = SDL_RWFromMem(data.first.get(), data.second);
		Mix_Chunk * chunk = Mix_LoadWAV_RW(ops, 1); // will free ops
		return MixChunkPtr(chunk);
	}
	catch(std::exception & e)
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

	int volumeByDistance = static_cast<int>(distancesVector[distance].Integer());
	return volumeByDistance * ambientConfig["volume"].Integer() / 100;
}

void CSoundHandler::ambientStopSound(const AudioPath & soundId)
{
	stopSound(ambientChannels[soundId]);
	setChannelVolume(ambientChannels[soundId], volume);
}

uint32_t CSoundHandler::getSoundDurationMilliseconds(const AudioPath & sound)
{
	if(!isInitialized() || sound.empty())
		return 0;

	auto resourcePath = sound.addPrefix("SOUNDS/");

	if(!CResourceHandler::get()->existsResource(resourcePath))
		return 0;

	auto data = CResourceHandler::get()->load(resourcePath)->readAll();

	uint32_t milliseconds = 0;

	Mix_Chunk * chunk = Mix_LoadWAV_RW(SDL_RWFromMem(data.first.get(), data.second), 1);

	int freq = 0;
	Uint16 fmt = 0;
	int channels = 0;
	if(!Mix_QuerySpec(&freq, &fmt, &channels))
		return 0;

	if(chunk != nullptr)
	{
		Uint32 sampleSizeBytes = (fmt & 0xFF) / 8;
		Uint32 samples = (chunk->alen / sampleSizeBytes);
		Uint32 frames = (samples / channels);
		milliseconds = ((frames * 1000) / freq);

		Mix_FreeChunk(chunk);
	}

	return milliseconds;
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID)
{
	assert(soundID < soundBase::sound_after_last);
	auto sound = AudioPath::builtin(soundsList[soundID]);
	logGlobal->trace("Attempt to play sound %d with file name %s with cache", soundID, sound.getOriginalName());

	return playSoundImpl(sound, 0, true);
}

int CSoundHandler::playSoundLooped(const AudioPath & sound)
{
	return playSoundImpl(sound, -1, true);
}

int CSoundHandler::playSound(const AudioPath & sound)
{
	return playSoundImpl(sound, 0, false);
}

int CSoundHandler::playSoundImpl(const AudioPath & sound, int repeats, bool useCache)
{
	if(!isInitialized() || sound.empty())
		return -1;

	int channel;
	MixChunkPtr chunkPtr = getSoundChunk(sound);
	Mix_Chunk * chunk = nullptr;
	if (!useCache)
	{
		chunkPtr = getSoundChunk(sound);
		chunk = chunkPtr.get();
	}
	else
		chunk = getSoundChunkCached(sound);

	if(chunk)
	{
		channel = Mix_PlayChannel(-1, chunk, 0);
		if(channel == -1)
		{
			logGlobal->error("Unable to play sound file %s , error %s", sound.getOriginalName(), Mix_GetError());
		}
		else
		{
			storeChunk(channel, std::move(chunkPtr));
			initCallback(channel);
		}
	}
	else
		channel = -1;

	return channel;
}

void CSoundHandler::storeChunk(int channel, MixChunkPtr chunk)
{
	std::scoped_lock lockGuard(mutexCallbacks);
	uncachedPlayingChunks[channel] = std::move(chunk);
}

int CSoundHandler::playSound(std::pair<std::unique_ptr<ui8[]>, si64> & data)
{
	int channel = -1;
	auto chunk = getSoundChunk(data);
	if(chunk)
	{
		channel = Mix_PlayChannel(-1, chunk.get(), 0);
		if(channel == -1)
		{
			logGlobal->error("Unable to play sound, error %s", Mix_GetError());
		}
		else
		{
			storeChunk(channel, std::move(chunk));
			initCallback(channel);
		}
	}
	return channel;
}

// Helper. Randomly select a sound from an array and play it
int CSoundHandler::playSoundFromSet(std::vector<soundBase::soundID> & sound_vec)
{
	return playSound(*RandomGeneratorUtil::nextItem(sound_vec, CRandomGenerator::getDefault()));
}

void CSoundHandler::stopSound(int handler)
{
	if(isInitialized() && handler != -1)
		Mix_HaltChannel(handler);
}

void CSoundHandler::pauseSound(int handler)
{
	if(isInitialized() && handler != -1)
		Mix_Pause(handler);
}

void CSoundHandler::resumeSound(int handler)
{
	if(isInitialized() && handler != -1)
		Mix_Resume(handler);
}

ui32 CSoundHandler::getVolume() const
{
	return volume;
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setVolume(ui32 percent)
{
	volume = std::min(100u, percent);

	if(isInitialized())
	{
		setChannelVolume(-1, volume);

		for(const auto & channel : channelVolumes)
			updateChannelVolume(channel.first);
	}
}

void CSoundHandler::updateChannelVolume(int channel)
{
	if(channelVolumes.count(channel))
		setChannelVolume(channel, getVolume() * channelVolumes[channel] / 100);
	else
		setChannelVolume(channel, getVolume());
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setChannelVolume(int channel, ui32 percent)
{
	Mix_Volume(channel, (MIX_MAX_VOLUME * percent) / 100);
}

void CSoundHandler::setCallback(int channel, std::function<void()> function)
{
	std::scoped_lock lockGuard(mutexCallbacks);

	auto iter = callbacks.find(channel);

	//channel not found. It may have finished so fire callback now
	if(iter == callbacks.end())
		function();
	else
		iter->second.push_back(function);
}

void CSoundHandler::resetCallback(int channel)
{
	std::scoped_lock lockGuard(mutexCallbacks);

	callbacks.erase(channel);
}

void CSoundHandler::soundFinishedCallback(int channel)
{
	std::scoped_lock lockGuard(mutexCallbacks);

	uncachedPlayingChunks.erase(channel);

	if(callbacks.count(channel) == 0)
		return;

	// store callbacks from container locally - SDL might reuse this channel for another sound
	// but do actually execution in separate thread, to avoid potential deadlocks in case if callback requires locks of its own
	auto callback = callbacks.at(channel);
	callbacks.erase(channel);

	if(!callback.empty())
	{
		ENGINE->dispatchMainThread(
			[callback]()
			{
				for(const auto & entry : callback)
					entry();
			}
			);
	}
}

void CSoundHandler::initCallback(int channel)
{
	std::scoped_lock lockGuard(mutexCallbacks);
	assert(callbacks.count(channel) == 0);
	callbacks[channel] = {};
}

void CSoundHandler::initCallback(int channel, const std::function<void()> & function)
{
	std::scoped_lock lockGuard(mutexCallbacks);
	assert(callbacks.count(channel) == 0);
	callbacks[channel].push_back(function);
}

int CSoundHandler::ambientGetRange() const
{
	return ambientConfig["range"].Integer();
}

void CSoundHandler::ambientUpdateChannels(std::map<AudioPath, int> soundsArg)
{
	std::scoped_lock guard(mutex);

	std::vector<AudioPath> stoppedSounds;
	for(const auto & pair : ambientChannels)
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
			int channelVolume = ambientDistToVolume(soundsArg[soundId]);
			channelVolumes[channel] = channelVolume;
			updateChannelVolume(channel);
		}
	}
	for(const auto & soundId : stoppedSounds)
	{
		channelVolumes.erase(ambientChannels[soundId]);
		ambientChannels.erase(soundId);
	}

	for(const auto & pair : soundsArg)
	{
		const auto & soundId = pair.first;
		const int distance = pair.second;

		if(!vstd::contains(ambientChannels, soundId))
		{
			int channel = playSoundLooped(soundId);
			int channelVolume = ambientDistToVolume(distance);
			channelVolumes[channel] = channelVolume;

			updateChannelVolume(channel);
			ambientChannels[soundId] = channel;
		}
	}
}

void CSoundHandler::ambientStopAllChannels()
{
	std::scoped_lock guard(mutex);

	for(const auto & ch : ambientChannels)
	{
		ambientStopSound(ch.first);
	}
	channelVolumes.clear();
	ambientChannels.clear();
}
