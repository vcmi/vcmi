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
#include "CMusicHandler.h"

#include "../events/InputHandler.h"
#include "GameEngine.h"
#include "../render/SDLRWwrapper.h"

#include "lib/entities/faction/CFaction.h"
#include "lib/entities/faction/CTown.h"
#include "lib/entities/faction/CTownHandler.h"
#include "lib/CRandomGenerator.h"
#include "lib/TerrainHandler.h"
#include "lib/filesystem/Filesystem.h"
#include "lib/GameLibrary.h"

void CMusicHandler::onVolumeChange(const JsonNode & volumeNode)
{
	setVolume(volumeNode.Integer());
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

	if (isInitialized())
	{
		musicTrack = MIX_CreateTrack(getMixer());

		if(musicTrack == nullptr)
			logGlobal->error("Unable to create music track: %s", SDL_GetError());
		else
		{
			MIX_SetTrackStoppedCallback(musicTrack, [](void *, MIX_Track *)
			{
				if (ENGINE)
					ENGINE->music().musicFinishedCallback();
			}, nullptr);
		}
	}
}

MIX_Track * CMusicHandler::getMusicTrack() const
{
	return musicTrack;
}

Sint64 CMusicHandler::fadeFrames(int fade_ms) const
{
	if(fade_ms <= 0 || musicTrack == nullptr)
		return 0;

	// fade lengths are counted in sample frames of whatever is assigned to the track
	return MIX_TrackMSToFrames(musicTrack, fade_ms);
}

void CMusicHandler::loadTerrainMusicThemes()
{
	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
	{
		for(const auto & filename : terrain->musicFilename)
			addEntryToSet("terrain_" + terrain->getJsonKey(), filename);
	}

	for(const auto & faction : LIBRARY->townh->objects)
	{
		if (!faction || !faction->hasTown())
			continue;

		for(const auto & filename : faction->town->clientInfo.musicTheme)
			addEntryToSet("faction_" + faction->getJsonKey(), filename);
	}
}

void CMusicHandler::addEntryToSet(const std::string & set, const AudioPath & musicURI)
{
	musicsSet[set].push_back(musicURI);
}

CMusicHandler::~CMusicHandler()
{
	if(isInitialized())
	{
		std::scoped_lock guard(mutex);

		if(musicTrack != nullptr)
			MIX_SetTrackStoppedCallback(musicTrack, nullptr, nullptr);

		current.reset();
		next.reset();

		if(musicTrack != nullptr)
		{
			MIX_StopTrack(musicTrack, 0);
			MIX_DestroyTrack(musicTrack);
			musicTrack = nullptr;
		}
	}
}

void CMusicHandler::playMusic(const AudioPath & musicURI, bool loop, bool fromStart)
{
	std::scoped_lock guard(mutex);

	if(current && current->isPlaying() && current->isTrack(musicURI))
		return;

	queueNext(this, "", musicURI, loop, fromStart);
}

void CMusicHandler::playMusicFromSet(const std::string & musicSet, const std::string & entryID, bool loop, bool fromStart)
{
	playMusicFromSet(musicSet + "_" + entryID, loop, fromStart);
}

void CMusicHandler::playMusicFromSet(const std::string & whichSet, bool loop, bool fromStart)
{
	std::scoped_lock guard(mutex);

	auto selectedSet = musicsSet.find(whichSet);
	if(selectedSet == musicsSet.end())
	{
		logGlobal->error("Error: playing music from non-existing set: %s", whichSet);
		return;
	}

	if(current && current->isPlaying() && current->isSet(whichSet))
		return;

	// in this mode - play random track from set
	queueNext(this, whichSet, AudioPath(), loop, fromStart);
}

void CMusicHandler::queueNext(std::unique_ptr<MusicEntry> queued)
{
	if(!isInitialized())
		return;

	next = std::move(queued);

	if(current == nullptr || !current->stop(1000))
	{
		current.reset(next.release());
		current->play();
	}
}

void CMusicHandler::queueNext(CMusicHandler * owner, const std::string & setName, const AudioPath & musicURI, bool looped, bool fromStart)
{
	queueNext(std::make_unique<MusicEntry>(owner, setName, musicURI, looped, fromStart));
}

void CMusicHandler::stopMusic(int fade_ms)
{
	if(!isInitialized())
		return;

	std::scoped_lock guard(mutex);

	if(current != nullptr)
		current->stop(fade_ms);
	next.reset();
}

ui32 CMusicHandler::getVolume() const
{
	return volume;
}

void CMusicHandler::setVolume(ui32 percent)
{
	volume = std::min(100u, percent);

	if(isInitialized() && musicTrack != nullptr)
		MIX_SetTrackGain(musicTrack, static_cast<float>(volume) / 100.f);
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

	ENGINE->dispatchMainThread(
		[this]()
		{
			std::unique_lock lockGuard(mutex);
			if(current != nullptr)
			{
				// if music is looped, play it again
				if(current->play())
					return;
				else
					current.reset();
			}

			if(current == nullptr && next != nullptr)
			{
				current.reset(next.release());
				current->play();
			}
		}
		);
}

MusicEntry::MusicEntry(CMusicHandler * owner, std::string setName, const AudioPath & musicURI, bool looped, bool fromStart)
	: owner(owner)
	, music(nullptr)
	, setName(std::move(setName))
	, currentName()
	, startTime(static_cast<uint32_t>(-1))
	, startPosition(0)
	, loop(looped ? -1 : 1)
	, fromStart(fromStart)
	, playing(false)
{
	if(!musicURI.empty())
		load(musicURI);
}

MusicEntry::~MusicEntry()
{
	if(playing && loop > 0)
	{
		assert(0);
		logGlobal->error("Attempt to delete music while playing!");
		MIX_StopTrack(owner->getMusicTrack(), 0);
	}

	if(loop == 0 && MIX_TrackPlaying(owner->getMusicTrack()))
	{
		logGlobal->trace("Halting playback of music file %s", currentName.getOriginalName());
		MIX_StopTrack(owner->getMusicTrack(), 0);
	}

	logGlobal->trace("Del-ing music file %s", currentName.getOriginalName());
	if(music)
		MIX_DestroyAudio(music);
}

void MusicEntry::load(const AudioPath & musicURI)
{
	if(music)
	{
		logGlobal->trace("Del-ing music file %s", currentName.getOriginalName());
		MIX_DestroyAudio(music);
		music = nullptr;
	}

	if(CResourceHandler::get()->existsResource(musicURI))
		currentName = musicURI;
	else
		currentName = musicURI.addPrefix("MUSIC/");

	music = nullptr;

	logGlobal->trace("Loading music file %s", currentName.getOriginalName());

	try
	{
		std::unique_ptr<CInputStream> stream = CResourceHandler::get()->load(currentName);

		if(musicURI.getName() == "BLADEFWCAMPAIGN") // handle defect MP3 file - ffprobe says: Skipping 52 bytes of junk at 0.
			stream->seek(52);

		auto * musicFile = MakeSDLIOStream(std::move(stream));
		// music is streamed from the file rather than predecoded, same as SDL2_mixer did
		music = MIX_LoadAudio_IO(owner->getMixer(), musicFile, false, true);
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Failed to load music. setName=%s\tmusicURI=%s", setName, currentName.getOriginalName());
		logGlobal->error("Exception: %s", e.what());
	}

	if(!music)
	{
		logGlobal->warn("Warning: Cannot open %s: %s", currentName.getOriginalName(), SDL_GetError());
		return;
	}
}

/// Starts the loaded track on the shared music track, optionally fading in and seeking
bool MusicEntry::playTrack(int fadeInMs, int startPositionMs)
{
	MIX_Track * track = owner->getMusicTrack();

	if(track == nullptr || music == nullptr)
		return false;

	if(!MIX_SetTrackAudio(track, music))
		return false;

	SDL_PropertiesID options = SDL_CreateProperties();
	SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, 0);

	if(fadeInMs > 0)
		SDL_SetNumberProperty(options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fadeInMs);

	if(startPositionMs > 0)
		SDL_SetNumberProperty(options, MIX_PROP_PLAY_START_MILLISECOND_NUMBER, startPositionMs);

	bool result = MIX_PlayTrack(track, options);
	SDL_DestroyProperties(options);

	return result;
}

bool MusicEntry::play()
{
	if(!(loop--) && music) //already played once - return
		return false;

	if(!setName.empty())
	{
		const auto & set = owner->musicsSet[setName];
		const auto & iter = RandomGeneratorUtil::nextItem(set, CRandomGenerator::getDefault());
		load(*iter);
	}

	logGlobal->trace("Playing music file %s", currentName.getOriginalName());

	if(!fromStart && owner->trackPositions.count(currentName) > 0 && owner->trackPositions[currentName] > 0)
	{
		float timeToStart = owner->trackPositions[currentName];
		startPosition = std::round(timeToStart * 1000);

		// erase stored position:
		// if music track will be interrupted again - new position will be written in stop() method
		// if music track is not interrupted and will finish by timeout/end of file - it will restart from beginning as it should
		owner->trackPositions.erase(owner->trackPositions.find(currentName));

		if(!playTrack(1000, std::round(timeToStart * 1000)))
		{
			logGlobal->error("Unable to play music (%s)", SDL_GetError());
			return false;
		}
	}
	else
	{
		startPosition = 0;

		if(!playTrack(0, 0))
		{
			logGlobal->error("Unable to play music (%s)", SDL_GetError());
			return false;
		}
	}

	startTime = ENGINE->input().getTicks();

	playing = true;
	return true;
}

bool MusicEntry::stop(int fade_ms)
{
	if(MIX_TrackPlaying(owner->getMusicTrack()))
	{
		playing = false;
		loop = 0;
		uint32_t endTime = ENGINE->input().getTicks();
		assert(startTime != uint32_t(-1));
		float playDuration = (endTime - startTime + startPosition) / 1000.f;
		owner->trackPositions[currentName] = playDuration;
		logGlobal->trace("Stopping music file %s at %f", currentName.getOriginalName(), playDuration);

		MIX_StopTrack(owner->getMusicTrack(), owner->fadeFrames(fade_ms));
		return true;
	}
	return false;
}

bool MusicEntry::isPlaying() const
{
	return playing;
}

bool MusicEntry::isSet(const std::string & set)
{
	return !setName.empty() && set == setName;
}

bool MusicEntry::isTrack(const AudioPath & track)
{
	return setName.empty() && track == currentName;
}
