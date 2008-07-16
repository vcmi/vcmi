#include "../stdafx.h"
#include "CMusicHandler.h"

void CMusicHandler::initMusics()
{
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1)
	{
		printf("Mix_OpenAudio error: %s!!!\n", Mix_GetError());
		//exit(2);
		sndh = NULL;
		return;
	}
	atexit(Mix_CloseAudio);

	AITheme0 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "AITheme0.mp3");
	AITheme1 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "AITHEME1.mp3");
	AITheme2 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "AITHEME2.mp3");
	buildTown = Mix_LoadWAV("MP3" PATHSEPARATOR "BUILDTWN.wav");
	combat1 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "COMBAT01.mp3");
	combat2 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "COMBAT02.mp3");
	combat3 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "COMBAT03.mp3");
	combat4 = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "COMBAT04.mp3");
	castleTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "CstleTown.mp3");
	defendCastle = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Defend Castle.mp3");
	dirt = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "DIRT.mp3");
	dungeon = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "DUNGEON.mp3");
	elemTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "ElemTown.mp3");
	evilTheme = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "EvilTheme.mp3");
	fortressTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "FortressTown.mp3");
	goodTheme = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "GoodTheme.mp3");
	grass = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "GRASS.mp3");
	infernoTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "InfernoTown.mp3");
	lava = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "LAVA.mp3");
	loopLepr = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "LoopLepr.mp3");
	loseCampain = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Lose Campain.mp3");
	loseCastle = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "LoseCastle.mp3");
	loseCombat = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "LoseCombat.mp3");
	mainMenu = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "MAINMENU.mp3");
	mainMenuWoG = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "MainMenuWoG.mp3");
	necroTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "necroTown.mp3");
	neutralTheme = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "NeutralTheme.mp3");
	rampart = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "RAMPART.mp3");
	retreatBattle = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Retreat Battle.mp3");
	rough = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "ROUGH.mp3");
	sand = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "SAND.mp3");
	secretTheme = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "SecretTheme.mp3");
	snow = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "SNOW.mp3");
	stronghold = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "StrongHold.mp3");
	surrenderBattle = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Surrender Battle.mp3");
	swamp = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "SWAMP.mp3");
	towerTown = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "TowerTown.mp3");
	ultimateLose = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "UltimateLose.mp3");
	underground = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Underground.mp3");
	water = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "WATER.mp3");
	winBattle = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Win Battle.mp3");
	winScenario = Mix_LoadMUS(DATADIR "MP3" PATHSEPARATOR "Win Scenario.mp3");

	click = Mix_LoadWAV(DATADIR "MP3" PATHSEPARATOR "snd1.wav");
	click->volume = 30;

	this->sndh = new CSndHandler(std::string(DATADIR "Data" PATHSEPARATOR "Heroes3.snd"));
}

void CMusicHandler::playClick()
{
	if(!sndh) return;
	int channel;
	channel = Mix_PlayChannel(-1, click, 0);
	if(channel == -1)
	{
		fprintf(stderr, "Unable to play WAV file: %s\n", Mix_GetError());
	}
}

void CMusicHandler::playLodSnd(std::string sndname)
{
	if(!sndh) return;
	int size;
	unsigned char *data;
	SDL_RWops *ops;
	Mix_Chunk *chunk;
	int channel;

	if ((data = sndh->extract(sndname, size)) == NULL)
		return;

	ops = SDL_RWFromConstMem(data, size);
	chunk = Mix_LoadWAV_RW(ops, 1);

	channel = Mix_PlayChannel(-1, chunk, 0);
	if(channel == -1)
	{
		fprintf(stderr, "Unable to play WAV file("DATADIR "Data" PATHSEPARATOR "Heroes3.wav::%s): %s\n",
			sndname.c_str(),Mix_GetError());
	}
	ops->close(ops);
}
