#include "StdInc.h"
#include <SDL_mixer.h>

#include "CMusicHandler.h"
#include "CGameInfo.h"
#include "SDLRWwrapper.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/JsonNode.h"
#include "../lib/GameConstants.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/StringConstants.h"
#include "../lib/CRandomGenerator.h"

/*
 * CMusicHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

static void musicFinishedCallbackC(void)
{
	CCS->musich->musicFinishedCallback();
}

void CAudioBase::init()
{
	if (initialized)
		return;

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		logGlobal->errorStream() << "Mix_OpenAudio error: " << Mix_GetError();
		return;
	}

	initialized = true;
}

void CAudioBase::release()
{
	if (initialized)
	{
		Mix_CloseAudio();
		initialized = false;
	}
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
	listener(settings.listen["general"]["sound"])
{
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
			if (chunk.second)
				Mix_FreeChunk(chunk.second);
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
			return soundChunks[sound];

		auto data = CResourceHandler::get()->load(ResourceID(std::string("SOUNDS/") + sound, EResType::SOUND))->readAll();
		SDL_RWops *ops = SDL_RWFromMem(data.first.release(), data.second);
		Mix_Chunk *chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

		if (cache)
			soundChunks.insert(std::pair<std::string, Mix_Chunk *>(sound, chunk));

		return chunk;
	}
	catch(std::exception &e)
	{
		logGlobal->warnStream() << "Cannot get sound " << sound << " chunk: " << e.what();
		return nullptr;
	}
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	assert(soundID < soundBase::sound_after_last);
	auto sound = sounds[soundID];
	logGlobal->traceStream() << "Attempt to play sound " << soundID << " with file name " << sound << " with cache";

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
			logGlobal->errorStream() << "Unable to play sound file " << sound << " , error " << Mix_GetError();
			if (!cache)
				Mix_FreeChunk(chunk);
		}
		else if (cache)
			callbacks[channel];
		else
			callbacks[channel] = [chunk]{ Mix_FreeChunk(chunk);};
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
		Mix_Volume(-1, (MIX_MAX_VOLUME * volume)/100);
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

void CMusicHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume(volumeNode.Float());
}

CMusicHandler::CMusicHandler():
	listener(settings.listen["general"]["music"])
{
	listener(std::bind(&CMusicHandler::onVolumeChange, this, _1));
	// Map music IDs
	// Vectors for helper
	const std::string setEnemy[] = {"AITheme0", "AITheme1", "AITheme2"};
	const std::string setBattle[] = {"Combat01", "Combat02", "Combat03", "Combat04"};

	auto fillSet = [=](std::string setName, const std::string list[], size_t amount)
	{
		for (size_t i=0; i < amount; i++)
			addEntryToSet(setName, i, "music/" + list[i]);
	};
	fillSet("enemy-turn", setEnemy, ARRAY_COUNT(setEnemy));
	fillSet("battle", setBattle, ARRAY_COUNT(setBattle));

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
		boost::mutex::scoped_lock guard(musicMutex);

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
		logGlobal->errorStream() << "Error: playing music from non-existing set: " << whichSet;
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
		logGlobal->errorStream() << "Error: playing music from non-existing set: " << whichSet;
		return;
	}

	auto selectedEntry = selectedSet->second.find(entryID);
	if (selectedEntry == selectedSet->second.end())
	{
		logGlobal->errorStream() << "Error: playing non-existing entry " << entryID << " from set: " << whichSet;
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

	boost::mutex::scoped_lock guard(musicMutex);

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
		logGlobal->errorStream() << "Failed to queue music. setName=" << setName << "\tmusicURI=" << musicURI;
		logGlobal->errorStream() << "Exception: " << e.what();
	}
}

void CMusicHandler::stopMusic(int fade_ms)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(musicMutex);

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

void CMusicHandler::musicFinishedCallback(void)
{
	boost::mutex::scoped_lock guard(musicMutex);

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
	logGlobal->traceStream()<<"Del-ing music file "<<currentName;
	if (music)
		Mix_FreeMusic(music);
}

void MusicEntry::load(std::string musicURI)
{
	if (music)
	{
		logGlobal->traceStream()<<"Del-ing music file "<<currentName;
		Mix_FreeMusic(music);
		music = nullptr;
	}

	currentName = musicURI;

	logGlobal->traceStream()<<"Loading music file "<<musicURI;

	auto musicFile = MakeSDLRWops(CResourceHandler::get()->load(ResourceID(std::move(musicURI), EResType::MUSIC)));

	music = Mix_LoadMUS_RW(musicFile, SDL_TRUE);

	if(!music)
	{
		logGlobal->warnStream() << "Warning: Cannot open " << currentName << ": " << Mix_GetError();
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

	logGlobal->traceStream()<<"Playing music file "<<currentName;
	if(Mix_PlayMusic(music, 1) == -1)
	{
		logGlobal->errorStream() << "Unable to play music (" << Mix_GetError() << ")";
		return false;
	}
	return true;
}

bool MusicEntry::stop(int fade_ms)
{
	if (Mix_PlayingMusic())
	{
		logGlobal->traceStream()<<"Stoping music file "<<currentName;
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
