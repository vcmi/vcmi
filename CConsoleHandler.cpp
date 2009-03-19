#define VCMI_DLL
#include "stdafx.h"
#include "CConsoleHandler.h"
#include "boost/function.hpp"

#ifdef _WIN32
#include <windows.h>
HANDLE handleIn;
HANDLE handleOut;
#endif
WORD defColor;

void CConsoleHandler::setColor(int level)
{
	WORD color;
	switch(level)
	{
	case -1:
		color = defColor;
		break;
	case 0:
#ifdef _WIN32
		color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
#else
		color = "\x1b[1;40;32m";
#endif
		break;
	case 1:
#ifdef _WIN32
		color = FOREGROUND_RED | FOREGROUND_INTENSITY;
#else
		color = "\x1b[1;40;31m";
#endif
		break;
	case 2:
#ifdef _WIN32
		color = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#else
		color = "\x1b[1;40;35m";
#endif
		break;
	case 3:
#ifdef _WIN32
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
#else
		color = "\x1b[1;40;32m";
#endif
		break;
	case 4:
#ifdef _WIN32
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#else
		color = "\x1b[1;40;39m";
#endif
		break;
	case 5:
#ifdef _WIN32
		color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
#else
		color = "\x1b[0;40;39m";
#endif
		break;
	default:
		color = defColor;
		break;
	}
#ifdef _WIN32
	SetConsoleTextAttribute(handleOut,color);
#else
	std::cout << color;
#endif
}

int CConsoleHandler::run()
{
	char buffer[5000];
	while(true)
	{
		std::cin.getline(buffer, 5000);
		if(cb && *cb)
			(*cb)(buffer);
	}
	return -1;
}
CConsoleHandler::CConsoleHandler()
{
#ifdef _WIN32
	handleIn = GetStdHandle(STD_INPUT_HANDLE);
	handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handleOut,&csbi);
	defColor = csbi.wAttributes;
#else
	defColor = "\x1b[0m";
#endif
	cb = new boost::function<void(const std::string &)>;
}
CConsoleHandler::~CConsoleHandler()
{
	delete cb;
}
#ifndef _WIN32
void CConsoleHandler::killConsole(pthread_t hThread)
#else
void CConsoleHandler::killConsole(void *hThread)
#endif
{
	tlog3 << "Killing console... ";
	_kill_thread(hThread,0);
	tlog3 << "done!\n";
}
