#include "stdafx.h"
#include "CConsoleHandler.h"
#include "CAdvmapInterface.h"
#include "CCastleInterface.h"
#include "CPlayerInterface.h"
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
#include "hch/CHeroHandler.h"
#include "hch/CLodHandler.h"
#include "boost/filesystem/operations.hpp"
#include <boost/algorithm/string.hpp>
#ifdef WIN32
#include <conio.h>
#else
#endif

bool continueReadingConsole = true;

int internalFunc(void * callback)
{
	CCallback * cb = (CCallback*)callback;
	char * usersMessage = new char[500];
	std::string readed;
	while(true)
	{
#ifdef WIN32
		if(continueReadingConsole && kbhit())
#else
#endif
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

			if(pom==std::string("die, fool"))
				exit(0);
			if(cn==std::string("activate"))
			{
				int what;
				readed >> what;
				switch (what)
				{
				case 0:
					LOCPLINT->curint->activate();
					break;
				case 1:
					LOCPLINT->adventureInt->activate();
					break;
				case 2:
					LOCPLINT->castleInt->activate();
					break;
				}
			}
			else if(pom==std::string("get txt"))
			{
				boost::filesystem::create_directory("Extracted_txts");
				std::cout<<"Command accepted. Opening .lod file...\t";
				CLodHandler * txth = new CLodHandler;
				txth->init(std::string(DATA_DIR "Data" PATHSEPARATOR "H3bitmap.lod"),"data");
				std::cout<<"done.\nScanning .lod file\n";
				int curp=0;
				std::string pattern = ".TXT";
				for(int i=0;i<txth->entries.size(); i++)
				{
					std::string pom = txth->entries[i].nameStr;
					if(boost::algorithm::find_last(pom,pattern))
					{
						txth->extractFile(std::string("Extracted_txts\\")+pom,pom);
					}
					int p2 = ((float)i/(float)txth->entries.size())*(float)100;
					if(p2!=curp)
					{
						curp = p2;
						std::cout<<"\r"<<curp<<"%";
					}
				}
				std::cout<<"\rExtracting done :)\n";
			}
			vector<Coordinate>* p;
			int heroX;
			int heroY;
			int heroZ;
			switch (*cn.c_str())
			{
			//case 'P':
			//	std::cout<<"Policzyc sciezke."<<std::endl;		
			//	readed>>src>>dst;
			//	
			//	p = CGI->pathf->GetPath(Coordinate(src),Coordinate(dst),CGI->heroh->heroInstances[0]);
			//	LOCPLINT->adventureInt->terrain.currentPath = CGI->pathf->ConvertToOldFormat(p);
			//	//LOCPLINT->adventureInt->terrain.currentPath = CGI->pathf->getPath(src,dst,CGI->heroh->heroInstances[0]);
			//	break;
			//case 'm': //number of heroes
			//	std::cout<<"Number of heroes: "<<CGI->heroh->heroInstances.size()<<std::endl;
			//	break;
			//case 'H': //position of hero
			//	readed>>heronum;
			//	std::cout<<"Position of hero "<<heronum<<": "<<CGI->heroh->heroInstances[heronum]->getPosition(false)<<std::endl;
			//	break;
			//case 'M': //move heroa
			//	{
			//		readed>>heronum>>dest;
			//		const CGHeroInstance * hero = cb->getHeroInfo(0,heronum,0);
			//		p = CGI->pathf->GetPath(Coordinate(hero->getPosition(false)),Coordinate(dest),hero);
			//		cb->moveHero(heronum, CGI->pathf->ConvertToOldFormat(p), 0, 0);
			//		//LOCPLINT->adventureInt->terrain.currentPath = CGI->pathf->getPath(src,dst,CGI->heroh->heroInstances[0]);
			//		break;
			//	}
			//case 'D': //pos description
			//	readed>>src;
			//	CGI->mh->getObjDescriptions(src);
			//	break;
			//case 'I': 
			//	{
			//		SDL_Surface * temp = LOCPLINT->infoWin(NULL);
			//		blitAtWR(temp,605,389);
			//		SDL_FreeSurface(temp);
			//		break;
			//	}
			//case 'T': //test rect
			//	readed>>src;
			//	for(int g=0; g<8; ++g)
			//	{
			//		for(int v=0; v<8; ++v)
			//		{
			//			int3 csrc = src;
			//			csrc.y+=g;
			//			csrc.x+=v;
			//			if(CGI->mh->getObjDescriptions(csrc).size())
			//				std::cout<<'x';
			//			else
			//				std::cout<<'o';
			//		}
			//		std::cout<<std::endl;
			//	}
			//	break;
			//case 'A':  //hide everything from map
			//	for(int c=0; c<CGI->objh->objInstances.size(); ++c)
			//	{
			//		CGI->mh->hideObject(CGI->objh->objInstances[c]);
			//	}
			//	break;
			//case 'R': //restora all objects after A has been pressed
			//	for(int c=0; c<CGI->objh->objInstances.size(); ++c)
			//	{
			//		CGI->mh->printObject(CGI->objh->objInstances[c]);
			//	}
			//	break;
			}
			//SDL_Delay(100);
			//delete p;
		}
		SDL_Delay(10);
	}
	return -1;
}

SDL_Thread * consoleReadingThread;

void CConsoleHandler::runConsole()
{
	consoleReadingThread = SDL_CreateThread(&internalFunc, cb);
}
