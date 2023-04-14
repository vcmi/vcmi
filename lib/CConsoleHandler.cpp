/*
 * CConsoleHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CConsoleHandler.h"
#include "CConfigHandler.h"

#include "CThreadHelper.h"

VCMI_LIB_NAMESPACE_BEGIN

boost::mutex CConsoleHandler::smx;

DLL_LINKAGE CConsoleHandler * console = nullptr;

VCMI_LIB_NAMESPACE_END

#ifndef VCMI_WINDOWS
	using TColor = std::string;
	#define CONSOLE_GREEN "\x1b[1;32m"
	#define CONSOLE_RED "\x1b[1;31m"
	#define CONSOLE_MAGENTA "\x1b[1;35m"
	#define CONSOLE_YELLOW "\x1b[1;33m"
	#define CONSOLE_WHITE "\x1b[1;37m"
	#define CONSOLE_GRAY "\x1b[1;30m"
	#define CONSOLE_TEAL "\x1b[1;36m"
#else
	#include <windows.h>
	#include <dbghelp.h>
#ifndef __MINGW32__
	#pragma comment(lib, "dbghelp.lib")
#endif
	typedef WORD TColor;
	HANDLE handleIn;
	HANDLE handleOut;
	HANDLE handleErr;
	#define CONSOLE_GREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_RED FOREGROUND_RED | FOREGROUND_INTENSITY
	#define CONSOLE_MAGENTA FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
	#define CONSOLE_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	#define CONSOLE_GRAY FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
	#define CONSOLE_TEAL FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY

	static TColor defErrColor;
#endif

static TColor defColor;

VCMI_LIB_NAMESPACE_BEGIN

#ifdef VCMI_WINDOWS

void printWinError()
{
	//Get error code
	int error = GetLastError();
	if(!error)
	{
		logGlobal->error("No Win error information set.");
		return;
	}
	logGlobal->error("Error %d encountered:", error);

	//Get error description
	char* pTemp = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, error,  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&pTemp, 1, nullptr);
	logGlobal->error(pTemp);
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
	logGlobal->error("Disaster happened.");

	PEXCEPTION_RECORD einfo = exception->ExceptionRecord;
	logGlobal->error("Reason: 0x%x - %s at %04x:%x", einfo->ExceptionCode, exceptionName(einfo->ExceptionCode), exception->ContextRecord->SegCs, (void*)einfo->ExceptionAddress);

	if (einfo->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		logGlobal->error("Attempt to %s 0x%8x", (einfo->ExceptionInformation[0] == 1 ? "write to" : "read from"), (void*)einfo->ExceptionInformation[1]);
	}
	const DWORD threadId = ::GetCurrentThreadId();
	logGlobal->error("Thread ID: %d", threadId);

	//exception info to be placed in the dump
	MINIDUMP_EXCEPTION_INFORMATION meinfo = {threadId, exception, TRUE};

	//create file where dump will be placed
	char *mname = nullptr;
	char buffer[MAX_PATH + 1];
	HMODULE hModule = nullptr;
	GetModuleFileNameA(hModule, buffer, MAX_PATH);
	mname = strrchr(buffer, '\\');
	if (mname != 0)
		mname++;
	else
		mname = buffer;

	strcat(mname, "_crashinfo.dmp");
	HANDLE dfile = CreateFileA(mname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	logGlobal->error("Crash info will be put in %s", mname);
	
	// flush loggers
	std::string padding(1000, '@');

	logGlobal->error(padding);
	logAi->error(padding);
	logNetwork->error(padding);

	auto dumpType = MiniDumpWithDataSegs;
	
	if(settings["general"]["extraDump"].Bool())
	{
		dumpType = (MINIDUMP_TYPE)(
			MiniDumpWithFullMemory
			| MiniDumpWithFullMemoryInfo
			| MiniDumpWithHandleData
			| MiniDumpWithUnloadedModules
			| MiniDumpWithThreadInfo);
	}

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dfile, dumpType, &meinfo, 0, 0);
	MessageBoxA(0, "VCMI has crashed. We are sorry. File with information about encountered problem has been created.", "VCMI Crashhandler", MB_OK | MB_ICONERROR);
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif


void CConsoleHandler::setColor(EConsoleTextColor::EConsoleTextColor color)
{
	TColor colorCode;
	switch(color)
	{
	case EConsoleTextColor::DEFAULT:
		colorCode = defColor;
		break;
	case EConsoleTextColor::GREEN:
		colorCode = CONSOLE_GREEN;
		break;
	case EConsoleTextColor::RED:
		colorCode = CONSOLE_RED;
		break;
	case EConsoleTextColor::MAGENTA:
		colorCode = CONSOLE_MAGENTA;
		break;
	case EConsoleTextColor::YELLOW:
		colorCode = CONSOLE_YELLOW;
		break;
	case EConsoleTextColor::WHITE:
		colorCode = CONSOLE_WHITE;
		break;
	case EConsoleTextColor::GRAY:
		colorCode = CONSOLE_GRAY;
		break;
	case EConsoleTextColor::TEAL:
		colorCode = CONSOLE_TEAL;
		break;
	default:
		colorCode = defColor;
		break;
	}
#ifdef VCMI_WINDOWS
    SetConsoleTextAttribute(handleOut, colorCode);
	if (color == EConsoleTextColor::DEFAULT)
		colorCode = defErrColor;
	SetConsoleTextAttribute(handleErr, colorCode);
#else
    std::cout << colorCode;
#endif
}

int CConsoleHandler::run() const
{
	setThreadName("CConsoleHandler::run");
	//disabling sync to make in_avail() work (othervice always returns 0)
	{
		TLockGuard _(smx);
		std::ios::sync_with_stdio(false);
	}
	std::string buffer;

	while ( std::cin.good() )
	{
#ifndef VCMI_WINDOWS
		//check if we have some unreaded symbols
		if (std::cin.rdbuf()->in_avail())
		{
			if ( getline(std::cin, buffer).good() )
				if ( cb && *cb )
					(*cb)(buffer, false);
		}
		else
			boost::this_thread::sleep(boost::posix_time::millisec(100));

		boost::this_thread::interruption_point();
#else
		std::getline(std::cin, buffer);
		if ( cb && *cb )
			(*cb)(buffer, false);
#endif
	}
	return -1;
}
CConsoleHandler::CConsoleHandler():
	cb(new std::function<void(const std::string &, bool)>), 
	thread(nullptr)
{
#ifdef VCMI_WINDOWS
	handleIn = GetStdHandle(STD_INPUT_HANDLE);
	handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	handleErr = GetStdHandle(STD_ERROR_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handleOut,&csbi);
	defColor = csbi.wAttributes;

	GetConsoleScreenBufferInfo(handleErr, &csbi);
	defErrColor = csbi.wAttributes;
#ifndef _DEBUG
	SetUnhandledExceptionFilter(onUnhandledException);
#endif
#else
	defColor = "\x1b[0m";
#endif
}
CConsoleHandler::~CConsoleHandler()
{
	logGlobal->info("Killing console...");
	end();
	delete cb;
	logGlobal->info("Killing console... done!");
}
void CConsoleHandler::end()
{
	if (thread)
	{
#ifndef VCMI_WINDOWS
		thread->interrupt();
#else
		TerminateThread(thread->native_handle(),0);
#endif
		thread->join();
		delete thread;
		thread = nullptr;
	}
}

void CConsoleHandler::start()
{
	thread = new boost::thread(std::bind(&CConsoleHandler::run,console));
}

VCMI_LIB_NAMESPACE_END
