#include "stdafx.h"
#include "CConsoleHandler.h"
#include "CAdvmapInterface.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "CGameInfo.h"
#include "global.h"
#include <sstream>

int internalFunc(void * nothingUsed)
{
	char * usersMessage = new char[500];
	std::string readed;
	while(true)
	{
		std::cin.getline(usersMessage, 500);
		std::istringstream readed;
		std::string pom(usersMessage);
		readed.str(pom);
		std::string cn; //command name
		readed >> cn;
		switch (*cn.c_str())
		{
		case 'P':
			std::cout<<"Policzyc sciezke."<<std::endl;
			int3 src, dst;
			readed>>src>>dst;
			LOCPLINT->adventureInt->terrain.currentPath = CGI->pathf->getPath(src,dst,CGI->heroh->heroInstances[0]);
			break;
		}
		//SDL_Delay(100);
	}
	return -1;
}

void CConsoleHandler::runConsole()
{
	SDL_Thread * myth = SDL_CreateThread(&internalFunc, NULL);
}
