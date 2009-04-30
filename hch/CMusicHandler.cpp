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

boost::bimap<soundBase::soundID, std::string> sounds;

CMusicHandler::~CMusicHandler()
{
	if (!audioInit)
		return;

	if (sndh) {
		Mix_HaltChannel(-1);
		delete sndh;
	}

	std::map<soundBase::soundID, Mix_Chunk *>::iterator it;
	for (it=soundChunks.begin(); it != soundChunks.end(); it++) {
		if (it->second)
			Mix_FreeChunk(it->second);
	}

	Mix_CloseAudio();
}

void CMusicHandler::initMusics()
{
	if (audioInit)
		return;

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		printf("Mix_OpenAudio error: %s!!!\n", Mix_GetError());
		return;
	}

	audioInit = true;

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

	// Load sounds
	sndh = new CSndHandler(std::string(DATA_DIR "Data" PATHSEPARATOR "Heroes3.snd"));
}

// Allocate an SDL chunk and cache it.
Mix_Chunk *CMusicHandler::GetSoundChunk(soundBase::soundID soundID)
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
soundBase::soundID CMusicHandler::getSoundID(std::string &fileName)
{
	boost::bimap<soundBase::soundID, std::string>::right_iterator it;

	it = sounds.right.find(fileName);
	if (it == sounds.right.end())
		return soundBase::invalid;
	else
		return it->second;
}

void CMusicHandler::initCreaturesSounds(std::vector<CCreature> &creatures)
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
int CMusicHandler::playSound(soundBase::soundID soundID, int repeats)
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
int CMusicHandler::playSoundFromSet(std::vector<soundBase::soundID> &sound_vec)
{
	return playSound(sound_vec[rand() % sound_vec.size()]);
}

void CMusicHandler::stopSound( int handler )
{
	if (handler != -1)
		Mix_HaltChannel(handler);
}