#include "stdafx.h"
#include "CMusicHandler.h"

void CMusicHandler::initMusics()
{
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1)
	{
		printf("Mix_OpenAudio: %s\n", Mix_GetError());
		exit(2);
	}
	atexit(Mix_CloseAudio);

	AITheme0 = Mix_LoadMUS("MP3\\AITheme0.mp3");
	AITheme1 = Mix_LoadMUS("MP3\\AITHEME1.mp3");
	AITheme2 = Mix_LoadMUS("MP3\\AITHEME2.mp3");
	buildTown = Mix_LoadWAV("MP3\\BUILDTWN.wav");
	combat1 = Mix_LoadMUS("MP3\\COMBAT01.mp3");
	combat2 = Mix_LoadMUS("MP3\\COMBAT02.mp3");
	combat3 = Mix_LoadMUS("MP3\\COMBAT03.mp3");
	combat4 = Mix_LoadMUS("MP3\\COMBAT04.mp3");
	castleTown = Mix_LoadMUS("MP3\\CstleTown.mp3");
	defendCastle = Mix_LoadMUS("MP3\\Defend Castle.mp3");
	dirt = Mix_LoadMUS("MP3\\DIRT.mp3");
	dungeon = Mix_LoadMUS("MP3\\DUNGEON.mp3");
	elemTown = Mix_LoadMUS("MP3\\ElemTown.mp3");
	evilTheme = Mix_LoadMUS("MP3\\EvilTheme.mp3");
	fortressTown = Mix_LoadMUS("MP3\\FortressTown.mp3");
	goodTheme = Mix_LoadMUS("MP3\\GoodTheme.mp3");
	grass = Mix_LoadMUS("MP3\\GRASS.mp3");
	infernoTown = Mix_LoadMUS("MP3\\InfernoTown.mp3");
	lava = Mix_LoadMUS("MP3\\LAVA.mp3");
	loopLepr = Mix_LoadMUS("MP3\\LoopLepr.mp3");
	loseCampain = Mix_LoadMUS("MP3\\Lose Campain.mp3");
	loseCastle = Mix_LoadMUS("MP3\\LoseCastle.mp3");
	loseCombat = Mix_LoadMUS("MP3\\LoseCombat.mp3");
	mainMenu = Mix_LoadMUS("MP3\\MAINMENU.mp3");
	mainMenuWoG = Mix_LoadMUS("MP3\\MainMenuWoG.mp3");
	necroTown = Mix_LoadMUS("MP3\\necroTown.mp3");
	neutralTheme = Mix_LoadMUS("MP3\\NeutralTheme.mp3");
	rampart = Mix_LoadMUS("MP3\\RAMPART.mp3");
	retreatBattle = Mix_LoadMUS("MP3\\Retreat Battle.mp3");
	rough = Mix_LoadMUS("MP3\\ROUGH.mp3");
	sand = Mix_LoadMUS("MP3\\SAND.mp3");
	secretTheme = Mix_LoadMUS("MP3\\SecretTheme.mp3");
	snow = Mix_LoadMUS("MP3\\SNOW.mp3");
	stronghold = Mix_LoadMUS("MP3\\StrongHold.mp3");
	surrenderBattle = Mix_LoadMUS("MP3\\Surrender Battle.mp3");
	swamp = Mix_LoadMUS("MP3\\SWAMP.mp3");
	towerTown = Mix_LoadMUS("MP3\\TowerTown.mp3");
	ultimateLose = Mix_LoadMUS("MP3\\UltimateLose.mp3");
	underground = Mix_LoadMUS("MP3\\Underground.mp3");
	water = Mix_LoadMUS("MP3\\WATER.mp3");
	winBattle = Mix_LoadMUS("MP3\\Win Battle.mp3");
	winScenario = Mix_LoadMUS("MP3\\Win Scenario.mp3");
	
	click = Mix_LoadWAV("MP3\\snd1.wav");
	click->volume = 30;
}

void CMusicHandler::playClick()
{
	int channel;
	channel = Mix_PlayChannel(-1, click, 0);
	if(channel == -1)
	{
		fprintf(stderr, "Unable to play WAV file: %s\n", Mix_GetError());
	}
}