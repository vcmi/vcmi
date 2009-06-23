#define VCMI_DLL
#include "stdafx.h"
#include "CConsoleHandler.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>


#ifndef _WIN32
	typedef std::string TColor;
	#define	_kill_thread(a) pthread_cancel(a)
	typedef pthread_t ThreadHandle;
	#define CONSOLE_GREEN "\x1b[1;40;32m"
	#define CONSOLE_RED "\x1b[1;40;32m"
	#define CONSOLE_MAGENTA "\x1b[1;40;35m"
	#define CONSOLE_YELLOW "\x1b[1;40;32m"
	#define CONSOLE_WHITE "\x1b[1;40;39m"
	#define CONSOLE_GRAY "\x1b[0;40;39m"
#else
	typedef WORD TColor;
	#define _kill_thread(a) TerminateThread(a,0)
	#include <windows.h>
	HANDLE handleIn;
	HANDLE handleOut;
	typedef void* ThreadHandle;
	#define CONSOLE_GREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_RED FOREGROUND_RED | FOREGROUND_INTENSITY
	#define CONSOLE_MAGENTA FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_GRAY FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#endif

TColor defColor;


/*
 * CConsoleHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CConsoleHandler::setColor(int level)
{
	TColor color;
	switch(level)
	{
	case -1:
		color = defColor;
		break;
	case 0:
		color = CONSOLE_GREEN;
		break;
	case 1:
		color = CONSOLE_RED;
		break;
	case 2:
		color = CONSOLE_MAGENTA;
		break;
	case 3:
		color = CONSOLE_YELLOW;
		break;
	case 4:
		color = CONSOLE_WHITE;
		break;
	case 5:
		color = CONSOLE_GRAY;
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
	thread = NULL;
}
CConsoleHandler::~CConsoleHandler()
{
	delete cb;
	delete thread;
}
void CConsoleHandler::end()
{
	tlog3 << "Killing console... ";
	ThreadHandle th = (ThreadHandle)thread->native_handle();
	_kill_thread(th);
	delete thread;
	thread = NULL;
	tlog3 << "done!\n";
}

void CConsoleHandler::start()
{
	thread = new boost::thread(boost::bind(&CConsoleHandler::run,console));
}