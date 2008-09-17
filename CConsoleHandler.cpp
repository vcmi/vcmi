#define VCMI_DLL
#include "stdafx.h"
#include "CConsoleHandler.h"
#include "CAdvmapInterface.h"
#include "CCastleInterface.h"
#include "CPlayerInterface.h"
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
#include <boost/algorithm/string.hpp>
#include "boost/function.hpp"
#include <boost/thread.hpp>
#ifdef _MSC_VER
#include <windows.h>
HANDLE handleIn;
HANDLE handleOut;
WORD defColor;
#endif

void CConsoleHandler::setColor(int level)
{
#ifdef _MSC_VER
	WORD color;
	switch(level)
	{
	case -1:
		color = defColor;
		break;
	case 0:
		color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
	case 1:
		color = FOREGROUND_RED | FOREGROUND_INTENSITY;
		break;
	case 2:
		color = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
	case 3:
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
	case 4:
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
	case 5:
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
	default:
		color = defColor;
		break;
	}
	SetConsoleTextAttribute(handleOut,color);
#endif
}

int CConsoleHandler::run()
{
	char buffer[500];
	std::string readed;
	while(true)
	{
		std::cin.getline(buffer, 500);
		if(cb && *cb)
			(*cb)(buffer);
	}
	return -1;
}
CConsoleHandler::CConsoleHandler()
{
#ifdef _MSC_VER
	handleIn = GetStdHandle(STD_INPUT_HANDLE);
	handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handleOut,&csbi);
	defColor = csbi.wAttributes;
#endif
	cb = new boost::function<void(const std::string &)>;
}
CConsoleHandler::~CConsoleHandler()
{
	delete cb;
}
void CConsoleHandler::killConsole(void *hThread)
{
#ifdef _MSC_VER
	log3 << "Killing console... ";
	TerminateThread(hThread,0);
	log3 << "done!\n";
#endif
}