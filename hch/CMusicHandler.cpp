#include "../stdafx.h"

#include <sstream>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/bimap.hpp>

#include <SDL_mixer.h>

#include "CSndHandler.h"
#include "CMusicHandler.h"
#include "CCreatureHandler.h"
#include "../CGameInfo.h"

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
	CGI->audioh->musicFinishedCallback();
}

void CSoundHandler::initSounds()
{
	// Map sound names
#define VCMI_SOUND_NAME(x) ( soundBase::x,
#define VCMI_SOUND_FILE(y) #y )
	sounds = boost::assign::list_of<boost::bimap<soundBase::soundID, std::string>::relation>
		VCMI_SOUND_LIST;
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

	// Vectors for helper(s)
	pickup_sounds += soundBase::pickup01, soundBase::pickup02, soundBase::pickup03,
		soundBase::pickup04, soundBase::pickup05, soundBase::pickup06, soundBase::pickup07;
	horseSounds +=  // must be the same order as terrains (see EtrrainType);
		soundBase::horseDirt, soundBase::horseSand, soundBase::horseGrass,
		soundBase::horseSnow, soundBase::horseSwamp, soundBase::horseRough,
		soundBase::horseSubterranean, soundBase::horseLava,
		soundBase::horseWater, soundBase::horseRock;

	// Load sounds
	sndh = new CSndHandler(std::string(DATA_DIR "Data" PATHSEPARATOR "Heroes3.snd"));
}

void CSoundHandler::freeSounds()
{
	Mix_HaltChannel(-1);
	delete sndh;

	std::map<soundBase::soundID, Mix_Chunk *>::iterator it;
	for (it=soundChunks.begin(); it != soundChunks.end(); it++) {
		if (it->second)
			Mix_FreeChunk(it->second);
	}
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
	const char *data = sndh->extract(it->second, size);
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

void CSoundHandler::initCreaturesSounds(std::vector<CCreature> &creatures)
{
	tlog5 << "\t\tReading config/cr_sounds.txt" << std::endl;
	std::ifstream ifs("config/cr_sounds.txt");
	std::string line;

	while(getline(ifs, line))
	{
		std::string cname="", attack="", defend="", killed="", move="", 
			shoot="", wince="", ext1="", ext2="";
		std::stringstream str(line);

		str >> cname >> attack >> defend >> killed >> move >> shoot >> wince >> ext1 >> ext2;

		if (cname[0] == '#')
			// That's a comment. Discard.
			continue;

		if (str.good() || (str.eof() && wince != ""))
		{
			int id = CGI->creh->nameToID[cname];
			CCreature &c = CGI->creh->creatures[id];

			if (c.sounds.killed != soundBase::invalid)
				tlog1 << "Creature << " << cname << " already has sounds" << std::endl;
			
			c.sounds.attack = getSoundID(attack);
			c.sounds.defend = getSoundID(defend);
			c.sounds.killed = getSoundID(killed);
			c.sounds.move = getSoundID(move);
			c.sounds.shoot = getSoundID(shoot);
			c.sounds.wince = getSoundID(wince);
			c.sounds.ext1 = getSoundID(ext1);
			c.sounds.ext2 = getSoundID(ext2);

			// Special creatures
			if (c.idNumber == 55 || // Archdevil
				c.idNumber == 62 || // Vampire
				c.idNumber == 62)	// Vampire Lord
			{
				c.sounds.startMoving = c.sounds.ext1;
				c.sounds.endMoving = c.sounds.ext2;
			}
		}
	}
	ifs.close();
	ifs.clear();

	// Find creatures without sounds
	for(unsigned int i=0;i<CGI->creh->creatures.size();i++)
	{
		// Note: this will exclude war machines, but it's better
		// than nothing.
		if (vstd::contains(CGI->creh->notUsedMonsters, i))
			continue;

		CCreature &c = CGI->creh->creatures[i];
		if (c.sounds.killed == soundBase::invalid)
			tlog1 << "creature " << c.idNumber << " doesn't have sounds" << std::endl;
	}
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	int channel;
	Mix_Chunk *chunk;
	std::map<soundBase::soundID, Mix_Chunk *>::iterator it;

	if (!sndh)
		return -1;

	chunk = GetSoundChunk(soundID);

	if (chunk)
	{
		channel = Mix_PlayChannel(-1, chunk, repeats);
		if (channel == -1)
			tlog1 << "Unable to play sound file " << soundID << std::endl;
		
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
	if (handler != -1)
		Mix_HaltChannel(handler);
}

void CMusicHandler::initMusics()
{
	// Map music IDs
#define VCMI_MUSIC_ID(x) ( musicBase::x ,
#define VCMI_MUSIC_FILE(y) y )
	musics = map_list_of
		VCMI_MUSIC_LIST;
#undef VCMI_MUSIC_NAME
#undef VCMI_MUSIC_FILE

	Mix_HookMusicFinished(musicFinishedCallbackC);

	// Vector for helper
	battleMusics += musicBase::combat1, musicBase::combat2, 
		musicBase::combat3, musicBase::combat4;
}

void CMusicHandler::freeMusics()
{
	Mix_HookMusicFinished(NULL);

	musicMutex.lock();

	if (currentMusic) {
		Mix_HaltMusic();
		Mix_FreeMusic(currentMusic);
	}

	if (nextMusic)
		Mix_FreeMusic(nextMusic);

	musicMutex.unlock();
}

// Plays a music
// loop: -1 always repeats, 0=do not play, 1+=number of loops
void CMusicHandler::playMusic(musicBase::musicID musicID, int loop)
{
	std::string filename = DATA_DIR "Mp3" PATHSEPARATOR;
	filename += musics[musicID];

	musicMutex.lock();

	if (nextMusic) {
		// There's already a music queued, so remove it
		Mix_FreeMusic(nextMusic);
		nextMusic = NULL;
	}

	if (currentMusic) {
		// A music is already playing. Stop it and the callback will
		// start the new one
		nextMusic = Mix_LoadMUS(filename.c_str());
		nextMusicLoop = loop;
		Mix_FadeOutMusic(1000);
	} else {
		currentMusic = Mix_LoadMUS(filename.c_str());
		if (Mix_PlayMusic(currentMusic, loop) == -1)
			tlog1 << "Unable to play sound file " << musicID << "(" << Mix_GetError() << ")" << std::endl;
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
	musicMutex.lock();

	if (currentMusic) {
		Mix_FadeOutMusic(fade_ms);
	}

	musicMutex.unlock();
}

// Called by SDL when a music finished.
void CMusicHandler::musicFinishedCallback(void)
{
	musicMutex.lock();

	if (currentMusic) {
		Mix_FreeMusic(currentMusic);
		currentMusic = NULL;
	}

	if (nextMusic) {
		currentMusic = nextMusic;
		nextMusic = NULL;
		if (Mix_PlayMusic(currentMusic, nextMusicLoop) == -1)
			tlog1 << "Unable to play music (" << Mix_GetError() << ")" << std::endl;
	}

	musicMutex.unlock();
}

void CAudioHandler::initAudio()
{
	if (audioInitialized)
		return;

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		tlog1 << "Mix_OpenAudio error: %s!!!" << Mix_GetError() << std::endl;
		return;
	}

	audioInitialized = true;

	initSounds();
	initMusics();
}

CAudioHandler::~CAudioHandler()
{
	if (!audioInitialized)
		return;

	freeSounds();
	freeMusics();

	Mix_CloseAudio();
}
