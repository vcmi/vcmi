#include "../stdafx.h"

#include <sstream>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/bimap.hpp>

#include <SDL_mixer.h>

#include "CSndHandler.h"
#include "CMusicHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CSpellHandler.h"
#include "../client/CGameInfo.h"

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
static void musicFinishedCallbackC(void) {
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
	if (initialized) {
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
	horseSounds +=  // must be the same order as terrains (see EtrrainType);
		soundBase::horseDirt, soundBase::horseSand, soundBase::horseGrass,
		soundBase::horseSnow, soundBase::horseSwamp, soundBase::horseRough,
		soundBase::horseSubterranean, soundBase::horseLava,
		soundBase::horseWater, soundBase::horseRock;
};

void CSoundHandler::init()
{
	CAudioBase::init();

	if (initialized) {
		// Load sounds
		sndh.add_file(std::string(DATA_DIR "/Data/Heroes3.snd"));
		sndh.add_file(std::string(DATA_DIR "/Data/H3ab_ahd.snd"));
	}
}

void CSoundHandler::release()
{
	if (initialized) {
		Mix_HaltChannel(-1);

		std::map<soundBase::soundID, Mix_Chunk *>::iterator it;
		for (it=soundChunks.begin(); it != soundChunks.end(); it++) {
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

	if (!chunk) {
		tlog1 << "Unable to mix sound" << it->second << "(" << Mix_GetError() << ")" << std::endl;
		return NULL;
	}

	soundChunks.insert(std::pair<soundBase::soundID, Mix_Chunk *>(soundID, chunk));

	return chunk;
}

// Get a soundID given a filename
soundBase::soundID CSoundHandler::getSoundID(std::string &fileName)
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
	tlog5 << "\t\tReading config/cr_sounds.txt" << std::endl;
	std::ifstream ifs(DATA_DIR "/config/cr_sounds.txt");
	std::string line;

	CBattleSounds.resize(creatures.size());

	while(getline(ifs, line))
	{
		std::string cname="", attack="", defend="", killed="", move="", 
			shoot="", wince="", ext1="", ext2="";
		std::istringstream str(line);

		str >> cname >> attack >> defend >> killed >> move >> shoot >> wince >> ext1 >> ext2;

		if (!line.size() || cname[0] == '#')
			// That's a comment. Discard.
			continue;

		if (str.good() || (str.eof() && wince != ""))
		{
			int id = -1;

			bmap<std::string,int>::const_iterator i = CGI->creh->nameToID.find(cname);
			if(i != CGI->creh->nameToID.end())
				id = i->second;
			else
			{
				tlog1 << "Sound info for an unknown creature: " << cname << std::endl;
				continue;
			}

			if (CBattleSounds[id].killed != soundBase::invalid)
				tlog1 << "Creature << " << cname << " already has sounds" << std::endl;
			
			CBattleSounds[id].attack = getSoundID(attack);
			CBattleSounds[id].defend = getSoundID(defend);
			CBattleSounds[id].killed = getSoundID(killed);
			CBattleSounds[id].move = getSoundID(move);
			CBattleSounds[id].shoot = getSoundID(shoot);
			CBattleSounds[id].wince = getSoundID(wince);
			CBattleSounds[id].ext1 = getSoundID(ext1);
			CBattleSounds[id].ext2 = getSoundID(ext2);

			// Special creatures
			if (id == 55 || // Archdevil
				id == 62 || // Vampire
				id == 62)	// Vampire Lord
			{
				CBattleSounds[id].startMoving = CBattleSounds[id].ext1;
				CBattleSounds[id].endMoving = CBattleSounds[id].ext2;
			}
		}
	}
	ifs.close();
	ifs.clear();

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
	tlog5 << "\t\tReading config/sp_sounds.txt" << std::endl;
	std::ifstream ifs(DATA_DIR "/config/sp_sounds.txt");
	std::string line;

	while(getline(ifs, line))
	{
		int spellid;
		std::string soundfile="";
		std::istringstream str(line);

		str >> spellid >> soundfile;

		if (str.good() || (str.eof() && soundfile != ""))
		{
			const CSpell *s = CGI->spellh->spells[spellid];

			if (vstd::contains(spellSounds, s))
			{
				tlog1 << "Spell << " << spellid << " already has a sound" << std::endl;
			}
			
			spellSounds[s] = getSoundID(soundfile);
		}
	}
	ifs.close();
	ifs.clear();
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
		
	} else {
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

CMusicHandler::CMusicHandler(): currentMusic(NULL), nextMusic(NULL)
{
	// Map music IDs
#define VCMI_MUSIC_ID(x) ( musicBase::x ,
#define VCMI_MUSIC_FILE(y) y )
	musics = map_list_of
		VCMI_MUSIC_LIST;
#undef VCMI_MUSIC_NAME
#undef VCMI_MUSIC_FILE

	// Vectors for helper
	battleMusics += musicBase::combat1, musicBase::combat2, 
		musicBase::combat3, musicBase::combat4;
	
	townMusics += musicBase::castleTown,     musicBase::rampartTown,
	              musicBase::towerTown,      musicBase::infernoTown,
	              musicBase::necroTown,      musicBase::dungeonTown,
				  musicBase::strongHoldTown, musicBase::fortressTown,
	              musicBase::elemTown;
}

void CMusicHandler::init()
{
	CAudioBase::init();

	if (initialized)
		Mix_HookMusicFinished(musicFinishedCallbackC);
}

void CMusicHandler::release()
{
	if (initialized) {
		Mix_HookMusicFinished(NULL);

		musicMutex.lock();

		if (currentMusic)
		{
			Mix_HaltMusic();
			Mix_FreeMusic(currentMusic);
		}

		if (nextMusic)
			Mix_FreeMusic(nextMusic);

		musicMutex.unlock();
	}

	CAudioBase::release();
}

// Plays a music
// loop: -1 always repeats, 0=do not play, 1+=number of loops
void CMusicHandler::playMusic(musicBase::musicID musicID, int loop)
{
	if (!initialized)
		return;

	std::string filename = DATA_DIR "/Mp3/";
	filename += musics[musicID];

	musicMutex.lock();

	if (nextMusic)
	{
		// There's already a music queued, so remove it
		Mix_FreeMusic(nextMusic);
		nextMusic = NULL;
	}

	if (currentMusic)
	{
		// A music is already playing. Stop it and the callback will
		// start the new one
		nextMusic = LoadMUS(filename.c_str());
		nextMusicLoop = loop;
		Mix_FadeOutMusic(1000);
	}
	else
	{
		currentMusic = LoadMUS(filename.c_str());
		PlayMusic(currentMusic,loop);
	}

	musicMutex.unlock();
}

// Helper. Randomly select a music from an array and play it
void CMusicHandler::playMusicFromSet(std::vector<musicBase::musicID> &music_vec, int loop)
{
	playMusic(music_vec[rand() % music_vec.size()], loop);
}

// Stop and free the current music
void CMusicHandler::stopMusic(int fade_ms)
{
	if (!initialized)
		return;

	musicMutex.lock();

	if (currentMusic)
	{
		Mix_FadeOutMusic(fade_ms);
	}

	musicMutex.unlock();
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
	musicMutex.lock();

	if (currentMusic)
	{
		Mix_FreeMusic(currentMusic);
		currentMusic = NULL;
	}

	if (nextMusic)
	{
		currentMusic = nextMusic;
		nextMusic = NULL;
		PlayMusic(currentMusic,nextMusicLoop);
	}

	musicMutex.unlock();
}

Mix_Music * CMusicHandler::LoadMUS(const char *file)
{
	Mix_Music *ret = Mix_LoadMUS(file);
	if(!ret) //load music and check for error
		tlog1 << "Unable to load music file (" << file <<"). Error: " << Mix_GetError() << std::endl;
	
	return ret;
}

int CMusicHandler::PlayMusic(Mix_Music *music, int loops)
{
	int ret = Mix_PlayMusic(music, loops);
	if(ret == -1)
		tlog1 << "Unable to play music (" << Mix_GetError() << ")" << std::endl;

	return ret;
}
