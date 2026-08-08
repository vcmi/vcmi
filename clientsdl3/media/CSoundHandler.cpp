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

#include "GameEngine.h"

#include "lib/filesystem/Filesystem.h"
#include "lib/CRandomGenerator.h"

#include <SDL3_mixer/SDL_mixer.h>

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

/// SDL2_mixer defaulted to 8 mixing channels, keep that as fallback
static constexpr int DEFAULT_CHANNELS_COUNT = 8;

CSoundHandler::CSoundHandler():
	listener(settings.listen["general"]["sound"]),
	ambientConfig(JsonPath::builtin("config/ambientSounds.json"))
{
	listener(std::bind(&CSoundHandler::onVolumeChange, this, _1));

	if(isInitialized())
	{
		if(ambientConfig["allocateChannels"].isNumber())
			allocateChannels(ambientConfig["allocateChannels"].Integer());
		else
			allocateChannels(DEFAULT_CHANNELS_COUNT);
	}
}

void CSoundHandler::allocateChannels(int count)
{
	for(int channel = 0; channel < count; ++channel)
	{
		MIX_Track * track = MIX_CreateTrack(getMixer());

		if(track == nullptr)
		{
			logGlobal->error("Unable to create mixer track: %s", SDL_GetError());
			break;
		}

		// track index is passed as userdata so that the callback knows the channel it belongs to
		MIX_SetTrackStoppedCallback(track, [](void * userdata, MIX_Track *)
		{
			// It is possible for this code to be executed during ENGINE destruction.
			// In this scenario, ENGINE is already nullptr, but ~CSoundHandler is still running
			if (ENGINE)
				ENGINE->sound().soundFinishedCallback(static_cast<int>(reinterpret_cast<intptr_t>(userdata)));
		}, reinterpret_cast<void *>(static_cast<intptr_t>(channel)));

		channels.push_back(track);
	}
}

MIX_Track * CSoundHandler::getChannel(int channel) const
{
	if(channel < 0 || channel >= static_cast<int>(channels.size()))
		return nullptr;

	return channels[channel];
}

int CSoundHandler::findFreeChannel() const
{
	for(int channel = 0; channel < static_cast<int>(channels.size()); ++channel)
		if(!MIX_TrackPlaying(channels[channel]) && !MIX_TrackPaused(channels[channel]))
			return channel;

	return -1;
}

void CSoundHandler::MixChunkDeleter::operator()(MIX_Audio * ptr)
{
	MIX_DestroyAudio(ptr);
}

CSoundHandler::~CSoundHandler()
{
	if(isInitialized())
	{
		for(auto * track : channels)
		{
			MIX_SetTrackStoppedCallback(track, nullptr, nullptr);
			MIX_StopTrack(track, 0);
			MIX_DestroyTrack(track);
		}
		channels.clear();

		soundChunks.clear();
		uncachedPlayingChunks.clear();
	}
}

MIX_Audio * CSoundHandler::getSoundChunkCached(const AudioPath & sound)
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
		SDL_IOStream * ops = SDL_IOFromMem(data.first.get(), data.second);
		// predecode, since the backing memory is released as soon as this method returns
		MIX_Audio * chunk = MIX_LoadAudio_IO(getMixer(), ops, true, true); // will free ops
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

		SDL_IOStream * ops = SDL_IOFromMem(data.first.get(), data.second);
		MIX_Audio * chunk = MIX_LoadAudio_IO(getMixer(), ops, true, true); // will free ops
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

	MIX_Audio * chunk = MIX_LoadAudio_IO(getMixer(), SDL_IOFromMem(data.first.get(), data.second), false, true);

	if(chunk != nullptr)
	{
		SDL_AudioSpec spec;
		Sint64 frames = MIX_GetAudioDuration(chunk);

		if(frames >= 0 && MIX_GetAudioFormat(chunk, &spec))
			milliseconds = MIX_FramesToMS(spec.freq, frames);

		MIX_DestroyAudio(chunk);
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
	MIX_Audio * chunk = nullptr;
	if (!useCache)
	{
		chunkPtr = getSoundChunk(sound);
		chunk = chunkPtr.get();
	}
	else
		chunk = getSoundChunkCached(sound);

	if(chunk)
	{
		channel = playChunk(chunk, repeats);
		if(channel == -1)
		{
			logGlobal->error("Unable to play sound file %s , error %s", sound.getOriginalName(), SDL_GetError());
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

/// Assigns the chunk to a free track and starts it, emulating Mix_PlayChannel(-1, ...)
int CSoundHandler::playChunk(MIX_Audio * chunk, int repeats)
{
	int channel = findFreeChannel();

	if(channel == -1)
	{
		SDL_SetError("No free mixer track available");
		return -1;
	}

	MIX_Track * track = channels[channel];

	if(!MIX_SetTrackAudio(track, chunk))
		return -1;

	SDL_PropertiesID options = SDL_CreateProperties();
	SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, repeats);
	bool started = MIX_PlayTrack(track, options);
	SDL_DestroyProperties(options);

	if(!started)
		return -1;

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
		channel = playChunk(chunk.get(), 0);
		if(channel == -1)
		{
			logGlobal->error("Unable to play sound, error %s", SDL_GetError());
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
	if(isInitialized() && getChannel(handler))
		MIX_StopTrack(getChannel(handler), 0);
}

void CSoundHandler::pauseSound(int handler)
{
	if(isInitialized() && getChannel(handler))
		MIX_PauseTrack(getChannel(handler));
}

void CSoundHandler::resumeSound(int handler)
{
	if(isInitialized() && getChannel(handler))
		MIX_ResumeTrack(getChannel(handler));
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
	float gain = static_cast<float>(percent) / 100.f;

	// channel of -1 means "every channel", same as in SDL2_mixer
	if(channel == -1)
	{
		for(auto * track : channels)
			MIX_SetTrackGain(track, gain);
	}
	else if(getChannel(channel))
		MIX_SetTrackGain(getChannel(channel), gain);
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
