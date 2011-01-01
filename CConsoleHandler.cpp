#define VCMI_DLL
#include "stdafx.h"
#include "CConsoleHandler.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <iomanip>

/*
* CConsoleHandler.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#ifndef _WIN32
	typedef std::string TColor;
	#define	_kill_thread(a) pthread_cancel(a)
	typedef pthread_t ThreadHandle;
	#define CONSOLE_GREEN "\x1b[1;32m"
	#define CONSOLE_RED "\x1b[1;32m"
	#define CONSOLE_MAGENTA "\x1b[1;35m"
	#define CONSOLE_YELLOW "\x1b[1;32m"
	#define CONSOLE_WHITE "\x1b[1;39m"
	#define CONSOLE_GRAY "\x1b[1;30m"
	#define CONSOLE_TEAL "\x1b[1;36m"
#else
	#define WIN32_LEAN_AND_MEAN //excludes rarely used stuff from windows headers - delete this line if something is missing
	#include <windows.h>
	#include <dbghelp.h>
	#pragma comment(lib, "dbghelp.lib")

	typedef WORD TColor;
	#define _kill_thread(a) TerminateThread(a,0)
	HANDLE handleIn;
	HANDLE handleOut;
	typedef void* ThreadHandle;
	#define CONSOLE_GREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_RED FOREGROUND_RED | FOREGROUND_INTENSITY
	#define CONSOLE_MAGENTA FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_GRAY FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
	#define CONSOLE_TEAL FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#endif

TColor defColor;

#ifdef _WIN32

void printWinError()
{
	//Get error code
	int error = GetLastError();
	if(!error)
	{
		tlog0 << "No Win error information set.\n";
		return;
	}
	tlog1 << "Error " << error << " encountered:\n";

	//Get error description
	char* pTemp = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error,  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&pTemp, 1, NULL);
	tlog1 << pTemp << std::endl;
	LocalFree( pTemp );
}

const char* exceptionName(DWORD exc)
{
#define EXC_CASE(EXC)	case EXCEPTION_##EXC : return "EXCEPTION_" #EXC
	switch (exc)
	{
		EXC_CASE(ACCESS_VIOLATION);
		EXC_CASE(DATATYPE_MISALIGNMENT);
		EXC_CASE(BREAKPOINT);
		EXC_CASE(SINGLE_STEP);
		EXC_CASE(ARRAY_BOUNDS_EXCEEDED);
		EXC_CASE(FLT_DENORMAL_OPERAND);
		EXC_CASE(FLT_DIVIDE_BY_ZERO);
		EXC_CASE(FLT_INEXACT_RESULT);
		EXC_CASE(FLT_INVALID_OPERATION);
		EXC_CASE(FLT_OVERFLOW);
		EXC_CASE(FLT_STACK_CHECK);
		EXC_CASE(FLT_UNDERFLOW);
		EXC_CASE(INT_DIVIDE_BY_ZERO);
		EXC_CASE(INT_OVERFLOW);
		EXC_CASE(PRIV_INSTRUCTION);
		EXC_CASE(IN_PAGE_ERROR);
		EXC_CASE(ILLEGAL_INSTRUCTION);
		EXC_CASE(NONCONTINUABLE_EXCEPTION);
		EXC_CASE(STACK_OVERFLOW);
		EXC_CASE(INVALID_DISPOSITION);
		EXC_CASE(GUARD_PAGE);
		EXC_CASE(INVALID_HANDLE);
	default:
		return "UNKNOWN EXCEPTION";
	}
#undef EXC_CASE
}



LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* exception)
{
	tlog1 << "Disaster happened.\n";

	PEXCEPTION_RECORD einfo = exception->ExceptionRecord;
	tlog1 << "Reason: 0x" << std::hex << einfo->ExceptionCode << " - " << exceptionName(einfo->ExceptionCode);
	tlog1 << " at " << std::setfill('0') << std::setw(4) << exception->ContextRecord->SegCs << ":" << (void*)einfo->ExceptionAddress << std::endl;;

	if (einfo->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		tlog1 << "Attempt to " << (einfo->ExceptionInformation[0] == 1 ? "write to " : "read from ") 
			<< "0x" <<  std::setw(8) << (void*)einfo->ExceptionInformation[1] << std::endl;;
	}
	const DWORD threadId = ::GetCurrentThreadId();
	tlog1 << "Thread ID: " << threadId << " [" << std::dec << std::setw(0) << threadId << "]\n";

	//exception info to be placed in the dump
	MINIDUMP_EXCEPTION_INFORMATION meinfo = {threadId, exception, TRUE};

	//create file where dump will be placed
	char *mname = NULL;
	char buffer[MAX_PATH + 1];
	HMODULE hModule = NULL;	
	GetModuleFileNameA(hModule, buffer, MAX_PATH);
	mname = strrchr(buffer, '\\');
	if (mname != 0)
		mname++;
	else
		mname = buffer;

	strcat(mname, "_crashinfo.dmp");
	HANDLE dfile = CreateFileA(mname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	tlog1 << "Crash info will be put in " << mname << std::endl;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dfile, MiniDumpWithDataSegs, &meinfo, 0, 0);
	MessageBoxA(0, "VCMI has crashed. We are sorry. File with information about encountered problem has been created.", "VCMI Crashhandler", MB_OK | MB_ICONERROR);
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif


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
	case -2:
		color = CONSOLE_TEAL;
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
#ifndef _DEBUG
	SetUnhandledExceptionFilter(onUnhandledException);
#endif
#else
	defColor = "\x1b[0m";
#endif
	cb = new boost::function<void(const std::string &)>;
	thread = NULL;
}
CConsoleHandler::~CConsoleHandler()
{
	tlog3 << "Killing console... ";
	end();
	delete cb;
	tlog3 << "done!\n";
}
void CConsoleHandler::end()
{
	if (thread) {
		ThreadHandle th = (ThreadHandle)thread->native_handle();
		_kill_thread(th);
		thread->join();
		delete thread;
		thread = NULL;
	}
}

void CConsoleHandler::start()
{
	thread = new boost::thread(boost::bind(&CConsoleHandler::run,console));
}
