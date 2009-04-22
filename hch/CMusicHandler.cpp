#include "../stdafx.h"

#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>

#include <SDL_mixer.h>

#include "CSndHandler.h"
#include "CMusicHandler.h"

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

void CMusicHandler::initMusics()
{
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		printf("Mix_OpenAudio error: %s!!!\n", Mix_GetError());
		return;
	}
	atexit(Mix_CloseAudio);

	// Map sound names
#define VCMI_SOUND_NAME(x) ( soundBase::x,
#define VCMI_SOUND_FILE(y) cachedSounds(#y, 0) )
	sounds = boost::assign::map_list_of
		VCMI_SOUND_LIST;
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

	//AITheme0 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "AITheme0.mp3");
	//AITheme1 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "AITHEME1.mp3");
	//AITheme2 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "AITHEME2.mp3");
	//buildTown = Mix_LoadWAV("MP3" PATHSEPARATOR "BUILDTWN.wav");
	//combat1 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "COMBAT01.mp3");
	//combat2 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "COMBAT02.mp3");
	//combat3 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "COMBAT03.mp3");
	//combat4 = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "COMBAT04.mp3");
	//castleTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "CstleTown.mp3");
	//defendCastle = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Defend Castle.mp3");
	//dirt = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "DIRT.mp3");
	//dungeon = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "DUNGEON.mp3");
	//elemTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "ElemTown.mp3");
	//evilTheme = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "EvilTheme.mp3");
	//fortressTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "FortressTown.mp3");
	//goodTheme = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "GoodTheme.mp3");
	//grass = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "GRASS.mp3");
	//infernoTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "InfernoTown.mp3");
	//lava = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "LAVA.mp3");
	//loopLepr = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "LoopLepr.mp3");
	//loseCampain = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Lose Campain.mp3");
	//loseCastle = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "LoseCastle.mp3");
	//loseCombat = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "LoseCombat.mp3");
	//mainMenu = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "MAINMENU.mp3");
	//mainMenuWoG = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "MainMenuWoG.mp3");
	//necroTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "necroTown.mp3");
	//neutralTheme = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "NeutralTheme.mp3");
	//rampart = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "RAMPART.mp3");
	//retreatBattle = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Retreat Battle.mp3");
	//rough = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "ROUGH.mp3");
	//sand = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "SAND.mp3");
	//secretTheme = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "SecretTheme.mp3");
	//snow = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "SNOW.mp3");
	//stronghold = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "StrongHold.mp3");
	//surrenderBattle = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Surrender Battle.mp3");
	//swamp = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "SWAMP.mp3");
	//towerTown = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "TowerTown.mp3");
	//ultimateLose = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "UltimateLose.mp3");
	//underground = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Underground.mp3");
	//water = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "WATER.mp3");
	//winBattle = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Win Battle.mp3");
	//winScenario = Mix_LoadMUS(DATA_DIR "MP3" PATHSEPARATOR "Win Scenario.mp3");

	// Map sounds
	sndh = new CSndHandler(std::string(DATA_DIR "Data" PATHSEPARATOR "Heroes3.snd"));
}

// Return an SDL chunk. Allocate if it is not cached yet.
Mix_Chunk *CMusicHandler::GetSoundChunk(std::string srcName)
{
	int size;
	const char *data = sndh->extract(srcName, size);
	SDL_RWops *ops;
	Mix_Chunk *chunk;

	if (!data)
		return NULL;

	ops = SDL_RWFromConstMem(data, size);
	chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops

	if (!chunk)
		fprintf(stderr, "Unable to mix: %s\n",
				Mix_GetError());

	return chunk;
}
  
// Plays a sound, and return its channel so we can fade it out later
int CMusicHandler::playSound(soundBase::soundNames soundID)
{
	int channel;

	if (!sndh)
		return -1;

	std::map<soundBase::soundNames, cachedSounds>::iterator it;

	it = sounds.find(soundID);
	if (it == sounds.end())
		return -1;

	class cachedSounds sound = it->second;

	if (!sound.chunk) {
		sound.chunk = GetSoundChunk(sound.filename);
	}

	if (sound.chunk)
	{
		channel = Mix_PlayChannel(-1, sound.chunk, 0);
		if(channel == -1)
		{
			fprintf(stderr, "Unable to play WAV file("DATA_DIR "Data" PATHSEPARATOR "Heroes3.wav::%s): %s\n",
					sound.filename.c_str(),Mix_GetError());
		}
	}

	return channel;
}
