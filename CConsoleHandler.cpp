#include "stdafx.h"
#include "CConsoleHandler.h"
#include "CAdvmapInterface.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "CGameInfo.h"
#include "global.h"
#include "CGameState.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "mapHandler.h"
#include <sstream>
#include "SDL_Extensions.h"
int internalFunc(void * callback)
{
	CCallback * cb = (CCallback*)callback;
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
		int3 src, dst;

		int heronum;
		int3 dest;

		switch (*cn.c_str())
		{
		case 'P':
			std::cout<<"Policzyc sciezke."<<std::endl;		
			readed>>src>>dst;
			LOCPLINT->adventureInt->terrain.currentPath = CGI->pathf->getPath(src,dst,CGI->heroh->heroInstances[0]);
			break;
		case 'm': //number of heroes
			std::cout<<"Number of heroes: "<<CGI->heroh->heroInstances.size()<<std::endl;
			break;
		case 'H': //position of hero
			readed>>heronum;
			std::cout<<"Position of hero "<<heronum<<": "<<CGI->heroh->heroInstances[heronum]->getPosition(false)<<std::endl;
			break;
		case 'M': //move heroa
			{
				readed>>heronum>>dest;
				const CHeroInstance * hero = cb->getHeroInfo(0,heronum,0);
				CPath * path = CGI->pathf->getPath(hero->getPosition(false),dest,hero);
				cb->moveHero(heronum, path, 0, 0);
				delete path;
				break;
			}
		case 'D': //pos description
			readed>>src;
			CGI->mh->getObjDescriptions(src);
			break;
		case 'I': 
			{
				SDL_Surface * temp = LOCPLINT->infoWin(NULL);
				blitAtWR(temp,605,389);
				break;
			}
		case 'T': //test rect
			readed>>src;
			for(int g=0; g<8; ++g)
			{
				for(int v=0; v<8; ++v)
				{
					int3 csrc = src;
					csrc.y+=g;
					csrc.x+=v;
					if(CGI->mh->getObjDescriptions(csrc).size())
						std::cout<<'x';
					else
						std::cout<<'o';
				}
				std::cout<<std::endl;
			}
			break;
		}
		//SDL_Delay(100);
	}
	return -1;
}

void CConsoleHandler::runConsole()
{
	SDL_Thread * myth = SDL_CreateThread(&internalFunc, cb);
}
