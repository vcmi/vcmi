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

#include <boost/stacktrace.hpp>

#if defined(NDEBUG) && !defined(VCMI_ANDROID)
#define USE_ON_TERMINATE
#endif

#if defined(NDEBUG) && defined(VCMI_WINDOWS)
#define USE_UNHANDLED_EXCEPTION_FILTER
#define CREATE_MEMORY_DUMP
#endif

#ifndef VCMI_WINDOWS
constexpr const char * CONSOLE_GREEN = "\x1b[1;32m";
constexpr const char * CONSOLE_RED = "\x1b[1;31m";
constexpr const char * CONSOLE_MAGENTA = "\x1b[1;35m";
constexpr const char * CONSOLE_YELLOW = "\x1b[1;33m";
constexpr const char * CONSOLE_WHITE = "\x1b[1;37m";
constexpr const char * CONSOLE_GRAY = "\x1b[1;30m";
constexpr const char * CONSOLE_TEAL = "\x1b[1;36m";
#else
#include <windows.h>
#include <dbghelp.h>
#ifndef __MINGW32__
	#pragma comment(lib, "dbghelp.lib")
#endif
HANDLE handleIn;
HANDLE handleOut;
HANDLE handleErr;
constexpr int32_t CONSOLE_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
constexpr int32_t CONSOLE_RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr int32_t CONSOLE_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr int32_t CONSOLE_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
constexpr int32_t CONSOLE_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr int32_t CONSOLE_GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
constexpr int32_t CONSOLE_TEAL = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#endif

VCMI_LIB_NAMESPACE_BEGIN

#ifdef CREATE_MEMORY_DUMP

static void createMemoryDump(MINIDUMP_EXCEPTION_INFORMATION * meinfo)
{
	//create file where dump will be placed
	char *mname = nullptr;
	char buffer[MAX_PATH + 1];
	HMODULE hModule = nullptr;
	GetModuleFileNameA(hModule, buffer, MAX_PATH);
	mname = strrchr(buffer, '\\');
	if (mname != nullptr)
		mname++;
	else
		mname = buffer;

	strcat(mname, "_crashinfo.dmp");
	HANDLE dfile = CreateFileA(mname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	logGlobal->error("Crash info will be put in %s", mname);

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

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dfile, dumpType, meinfo, nullptr, nullptr);
	MessageBoxA(0, "VCMI has crashed. We are sorry. File with information about encountered problem has been created.", "VCMI Crashhandler", MB_OK | MB_ICONERROR);
}

#endif

#ifdef USE_UNHANDLED_EXCEPTION_FILTER
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

#ifdef CREATE_MEMORY_DUMP
	createMemoryDump(&meinfo);
#endif

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#ifdef USE_ON_TERMINATE
[[noreturn]] static void onTerminate()
{
	logGlobal->error("Disaster happened.");
	try
	{
		std::exception_ptr eptr{std::current_exception()};
		if (eptr)
		{
			std::rethrow_exception(eptr);
		}
		else
		{
			logGlobal->error("...but no current exception found!");
		}
	}
	catch (const std::exception& exc)
	{
		logGlobal->error("Reason: %s", exc.what());
	}
	catch (...)
	{
		logGlobal->error("Reason: unknown exception!");
	}

	logGlobal->error("Call stack information:");
	std::stringstream stream;
	stream << boost::stacktrace::stacktrace();
	logGlobal->error("%s", stream.str());

#ifdef CREATE_MEMORY_DUMP
	const DWORD threadId = ::GetCurrentThreadId();
	logGlobal->error("Thread ID: %d", threadId);

	createMemoryDump(nullptr);
#endif
	std::abort();
}
#endif

void CConsoleHandler::setColor(EConsoleTextColor color)
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

int CConsoleHandler::run()
{
	setThreadName("consoleHandler");
	//disabling sync to make in_avail() work (othervice always returns 0)
	{
		std::lock_guard guard(smx);
		std::ios::sync_with_stdio(false);
	}
	std::string buffer;

	while ( std::cin.good() )
	{
#ifndef _MSC_VER
		//check if we have some unreaded symbols
		if (std::cin.rdbuf()->in_avail())
		{
			if ( getline(std::cin, buffer).good() )
				if ( cb )
					cb(buffer, false);
		}

		std::unique_lock guard(shutdownMutex);
		shutdownVariable.wait_for(guard, std::chrono::seconds(1));

		if (shutdownPending)
			return -1;
#else
		std::getline(std::cin, buffer);
		if ( cb )
			cb(buffer, false);
#endif
	}
	return -1;
}

CConsoleHandler::CConsoleHandler()
	:CConsoleHandler(std::function<void(const std::string &, bool)>{})
{}

CConsoleHandler::CConsoleHandler(const std::function<void(const std::string &, bool)> & callback)
	:cb(callback)
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
#else
	defColor = "\x1b[0m";
#endif

#ifdef USE_UNHANDLED_EXCEPTION_FILTER
	SetUnhandledExceptionFilter(onUnhandledException);
#endif

#ifdef USE_ON_TERMINATE
	std::set_terminate(onTerminate);
#endif
}

CConsoleHandler::~CConsoleHandler()
{
	logGlobal->info("Killing console...");
	end();
	logGlobal->info("Killing console... done!");
}
void CConsoleHandler::end()
{
	if (thread.joinable())
	{
#ifndef _MSC_VER
		shutdownPending = true;
		shutdownVariable.notify_all();
#else
		TerminateThread(thread.native_handle(),0);
#endif
		thread.join();
	}
}

void CConsoleHandler::start()
{
	thread = std::thread(std::bind(&CConsoleHandler::run, this));
}

VCMI_LIB_NAMESPACE_END
