#include "CConsoleHandler.h"
#include "stdafx.h"
#include "SDL.h"
#include "SDL_thread.h"

int internalFunc(void * nothingUsed)
{
	char * usersMessage = new char[500];
	std::string readed;
	while(true)
	{
		std::cin.getline(usersMessage, 500);
		readed = std::string(usersMessage);
	}
	return -1;
}

void CConsoleHandler::runConsole()
{
	SDL_Thread * myth = SDL_CreateThread(&internalFunc, NULL);
}
