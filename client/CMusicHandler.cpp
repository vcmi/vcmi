#include "../stdafx.h"

#include <sstream>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/bimap.hpp>
#include <boost/foreach.hpp>

#include <SDL_mixer.h>

#include "CSndHandler.h"
#include "CMusicHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CSpellHandler.h"
#include "../client/CGameInfo.h"
#include "../lib/JsonNode.h"

/*
 * CMusicHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;

static boost::bimap<soundBase::soundID, std::string> sounds;

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
		tlog1 << "Mix_OpenAudio error: " << Mix_GetError() << std::endl;
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

void CAudioBase::setVolume(unsigned int percent)
{
	if (percent > 100)
		percent = 100;
	
	volume = percent;
}

CSoundHandler::CSoundHandler()
{
	// Map sound names
#define VCMI_SOUND_NAME(x) ( soundBase::x,
#define VCMI_SOUND_FILE(y) #y )
	sounds = boost::assign::list_of<boost::bimap<soundBase::soundID, std::string>::relation>
		VCMI_SOUND_LIST;
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

	// Vectors for helper(s)
	pickupSounds += soundBase::pickup01, soundBase::pickup02, soundBase::pickup03,
		soundBase::pickup04, soundBase::pickup05, soundBase::pickup06, soundBase::pickup07;

	horseSounds +=  // must be the same order as terrains (see EterrainType);
		soundBase::horseDirt, soundBase::horseSand, soundBase::horseGrass,
		soundBase::horseSnow, soundBase::horseSwamp, soundBase::horseRough,
		soundBase::horseSubterranean, soundBase::horseLava,
		soundBase::horseWater, soundBase::horseRock;

	battleIntroSounds +=     soundBase::battle00, soundBase::battle01,
	    soundBase::battle02, soundBase::battle03, soundBase::battle04,
	    soundBase::battle05, soundBase::battle06, soundBase::battle07;
};

void CSoundHandler::init()
{
	CAudioBase::init();

	if (initialized)
	{
		// Load sounds
		sndh.add_file(std::string(DATA_DIR "/Data/Heroes3.snd"));
		sndh.add_file(std::string(DATA_DIR "/Data/Heroes3-cd2.snd"), false);
		sndh.add_file(std::string(DATA_DIR "/Data/H3ab_ahd.snd"));
		Mix_ChannelFinished(soundFinishedCallbackC);
	}
}

void CSoundHandler::release()
{
	if (initialized)
	{
		Mix_HaltChannel(-1);

		std::map<soundBase::soundID, Mix_Chunk *>::iterator it;
		for (it=soundChunks.begin(); it != soundChunks.end(); it++)
		{
			if (it->second)
				Mix_FreeChunk(it->second);
		}
	}

	CAudioBase::release();
}

// Allocate an SDL chunk and cache it.
Mix_Chunk *CSoundHandler::GetSoundChunk(soundBase::soundID soundID)
{
	// Find its name
	boost::bimap<soundBase::soundID, std::string>::left_iterator it;
	it = sounds.left.find(soundID);
	if (it == sounds.left.end())
		return NULL;

	// Load and insert
	int size;
	const char *data = sndh.extract(it->second, size);
	if (!data)
		return NULL;

	SDL_RWops *ops = SDL_RWFromConstMem(data, size);
	Mix_Chunk *chunk;
	chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

	if (!chunk)
	{
		tlog1 << "Unable to mix sound" << it->second << "(" << Mix_GetError() << ")" << std::endl;
		return NULL;
	}

	soundChunks.insert(std::pair<soundBase::soundID, Mix_Chunk *>(soundID, chunk));

	return chunk;
}

// Get a soundID given a filename
soundBase::soundID CSoundHandler::getSoundID(const std::string &fileName)
{
	boost::bimap<soundBase::soundID, std::string>::right_iterator it;

	it = sounds.right.find(fileName);
	if (it == sounds.right.end())
		return soundBase::invalid;
	else
		return it->second;
}

void CSoundHandler::initCreaturesSounds(const std::vector<ConstTransitivePtr< CCreature> > &creatures)
{
	tlog5 << "\t\tReading config/cr_sounds.json" << std::endl;
	const JsonNode config(DATA_DIR "/config/cr_sounds.json");

	CBattleSounds.resize(creatures.size());

	if (!config["creature_sounds"].isNull()) {
		
		BOOST_FOREACH(const JsonNode &node, config["creature_sounds"].Vector()) {
			const JsonNode *value;
			int id;

			value = &node["name"];

			bmap<std::string,int>::const_iterator i = CGI->creh->nameToID.find(value->String());
			if (i != CGI->creh->nameToID.end())
				id = i->second;
			else
			{
				tlog1 << "Sound info for an unknown creature: " << value->String() << std::endl;
				continue;
			}

			/* This is a bit ugly. Maybe we should use an array for
			 * sound ids instead of separate variables and define
			 * attack/defend/killed/... as indexes. */
#define GET_SOUND_VALUE(value_name) do { value = &node[#value_name]; if (!value->isNull()) CBattleSounds[id].value_name = getSoundID(value->String()); } while(0)

			GET_SOUND_VALUE(attack);
			GET_SOUND_VALUE(defend);
			GET_SOUND_VALUE(killed);
			GET_SOUND_VALUE(move);
			GET_SOUND_VALUE(shoot);
			GET_SOUND_VALUE(wince);
			GET_SOUND_VALUE(ext1);
			GET_SOUND_VALUE(ext2);
			GET_SOUND_VALUE(startMoving);
			GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE
		}
	}
	
	//commented to avoid spurious warnings
	/*
	// Find creatures without sounds
	for(unsigned int i=0;i<creatures.size();i++)
	{
		// Note: this will exclude war machines, but it's better
		// than nothing.
		if (vstd::contains(CGI->creh->notUsedMonsters, i))
			continue;

		CCreature &c = creatures[i];
		if (c.sounds.killed == soundBase::invalid)
			tlog1 << "creature " << c.idNumber << " doesn't have sounds" << std::endl;
	}*/
}

void CSoundHandler::initSpellsSounds(const std::vector< ConstTransitivePtr<CSpell> > &spells)
{
	const JsonNode config(DATA_DIR "/config/sp_sounds.json");

	if (!config["spell_sounds"].isNull()) {
		BOOST_FOREACH(const JsonNode &node, config["spell_sounds"].Vector()) {
			int spellid = node["id"].Float();
			const CSpell *s = CGI->spellh->spells[spellid];

			if (vstd::contains(spellSounds, s))
				tlog1 << "Spell << " << spellid << " already has a sound" << std::endl;

			spellSounds[s] = getSoundID(node["soundfile"].String());
		}
	}
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	if (!initialized)
		return -1;

	int channel;
	Mix_Chunk *chunk = GetSoundChunk(soundID);

	if (chunk)
	{
		channel = Mix_PlayChannel(-1, chunk, repeats);
		if (channel == -1)
			tlog1 << "Unable to play sound file " << soundID << " , error " << Mix_GetError() << std::endl;
		else
			callbacks[channel];//insert empty callback
	}
	else
	{
		channel = -1;
	}

	return channel;
}

// Helper. Randomly select a sound from an array and play it
int CSoundHandler::playSoundFromSet(std::vector<soundBase::soundID> &sound_vec)
{
	return playSound(sound_vec[rand() % sound_vec.size()]);
}

void CSoundHandler::stopSound( int handler )
{
	if (initialized && handler != -1)
		Mix_HaltChannel(handler);
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setVolume(unsigned int percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		Mix_Volume(-1, (MIX_MAX_VOLUME * volume)/100);
}

void CSoundHandler::setCallback(int channel, boost::function<void()> function)
{
	std::map<int, boost::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	//channel not found. It may have finished so fire callback now
	if(iter == callbacks.end())
		function();
	else
		iter->second = function;
}

void CSoundHandler::soundFinishedCallback(int channel)
{
	std::map<int, boost::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	assert(iter != callbacks.end());

	if (iter->second)
		iter->second();

	callbacks.erase(iter);
}

CMusicHandler::CMusicHandler()
{
	// Map music IDs
#define VCMI_MUSIC_ID(x) ( musicBase::x ,
#define VCMI_MUSIC_FILE(y) y )
	musics = map_list_of
		VCMI_MUSIC_LIST;
#undef VCMI_MUSIC_NAME
#undef VCMI_MUSIC_FILE

	// Vectors for helper
	aiMusics += musicBase::AITheme0, musicBase::AITheme1, musicBase::AITheme2;
	
	battleMusics += musicBase::combat1, musicBase::combat2, 
		musicBase::combat3, musicBase::combat4;
	
	townMusics += musicBase::castleTown,     musicBase::rampartTown,
	              musicBase::towerTown,      musicBase::infernoTown,
	              musicBase::necroTown,      musicBase::dungeonTown,
	              musicBase::strongHoldTown, musicBase::fortressTown,
	              musicBase::elemTown;

	terrainMusics += musicBase::dirt, musicBase::sand, musicBase::grass,
		musicBase::snow, musicBase::swamp, musicBase::rough,
		musicBase::underground, musicBase::lava,musicBase::water;
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

		Mix_HookMusicFinished(NULL);

		current.reset();
		next.reset();
	}

	CAudioBase::release();
}

// Plays a music
// loop: -1 always repeats, 0=do not play, 1+=number of loops
void CMusicHandler::playMusic(musicBase::musicID musicID, int loop)
{
	if (current.get() != NULL && *current == musicID)
		return;

	queueNext(new MusicEntry(this, musicID, loop));
}

// Helper. Randomly plays tracks from music_vec
void CMusicHandler::playMusicFromSet(std::vector<musicBase::musicID> &music_vec, int loop)
{
	if (current.get() != NULL && *current == music_vec)
		return;

	queueNext(new MusicEntry(this, music_vec, loop));
}

void CMusicHandler::queueNext(MusicEntry *queued)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(musicMutex);

	next.reset(queued);

	if (current.get() != NULL)
	{
		current->stop(1000);
	}
	else
	{
		current = next;
		current->play();
	}
}

// Stop and free the current music
void CMusicHandler::stopMusic(int fade_ms)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(musicMutex);

	if (current.get() != NULL)
		current->stop(fade_ms);
	next.reset();

}

// Sets the music volume, from 0 (mute) to 100
void CMusicHandler::setVolume(unsigned int percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		Mix_VolumeMusic((MIX_MAX_VOLUME * volume)/100);
}

// Called by SDL when a music finished.
void CMusicHandler::musicFinishedCallback(void)
{
	boost::mutex::scoped_lock guard(musicMutex);

	if (current.get() != NULL)
	{
		//return if current music still not finished
		if (current->play())
			return;
		else
			current.reset();
	}

	if (current.get() == NULL && next.get() != NULL)
	{
		current = next;
		current->play();
	}
}

MusicEntry::MusicEntry(CMusicHandler *_owner, musicBase::musicID _musicID, int _loopCount):
	owner(_owner),
	music(NULL),
	loopCount(_loopCount)
{
	load(_musicID);
}

MusicEntry::MusicEntry(CMusicHandler *_owner, std::vector<musicBase::musicID> &_musicVec, int _loopCount):
	currentID(musicBase::music_todo),
	owner(_owner),
	music(NULL),
	loopCount(_loopCount),
	musicVec(_musicVec)
{
	//In this case music will be loaded only on playing - no need to call load() here
}

MusicEntry::~MusicEntry()
{
	tlog5<<"Del-ing music file "<<filename<<"\n";
	if (music)
		Mix_FreeMusic(music);
}

void MusicEntry::load(musicBase::musicID ID)
{
	currentID = ID;
	filename = DATA_DIR "/Mp3/";
	filename += owner->musics[ID];

	tlog5<<"Loading music file "<<filename<<"\n";
	if (music)
		Mix_FreeMusic(music);

	music = Mix_LoadMUS(filename.c_str());

	if(!music)
	{
		tlog3 << "Warning: Cannot open " << filename << ": " << Mix_GetError() << std::endl;
		return;
	}

#ifdef _WIN32
	//The assertion will fail if old MSVC libraries pack .dll is used
	assert(Mix_GetMusicType(music) == MUS_MP3_MAD);
#endif
}

bool MusicEntry::play()
{
	tlog5<<"Playing music file "<<filename<<"\n";
	if (loopCount == 0)
		return false;

	if (loopCount > 0)
		loopCount--;

	if (!musicVec.empty())
		load(musicVec.at(rand() % musicVec.size()));

	if(Mix_PlayMusic(music, 1) == -1)
	{
		tlog1 << "Unable to play music (" << Mix_GetError() << ")" << std::endl;
		return false;
	}
	return true;
}

void MusicEntry::stop(int fade_ms)
{
	tlog5<<"Stoping music file "<<filename<<"\n";
	loopCount = 0;
	Mix_FadeOutMusic(fade_ms);
}

bool MusicEntry::operator == (musicBase::musicID _musicID) const
{
	return musicVec.empty() && currentID == _musicID;
}

bool MusicEntry::operator == (std::vector<musicBase::musicID> &_musicVec) const
{
	return musicVec == _musicVec;
}
