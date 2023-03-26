/*
 * CThreadHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CThreadHelper.h"

#ifdef VCMI_WINDOWS
	#include <windows.h>
#elif defined(VCMI_HAIKU)
	#include <OS.h>
#elif !defined(VCMI_APPLE) && !defined(VCMI_FREEBSD) && !defined(VCMI_HURD)
	#include <sys/prctl.h>
#endif

VCMI_LIB_NAMESPACE_BEGIN

CThreadHelper::CThreadHelper(std::vector<std::function<void()>> * Tasks, int Threads):
	currentTask(0),
	amount(static_cast<int>(Tasks->size())),
	tasks(Tasks),
	threads(Threads)
{
}
void CThreadHelper::run()
{
	boost::thread_group grupa;
	for(int i=0;i<threads;i++)
		grupa.create_thread(std::bind(&CThreadHelper::processTasks,this));
	grupa.join_all();

	//thread group deletes threads, do not free manually
}
void CThreadHelper::processTasks()
{
	while(true)
	{
		int pom;
		{
			boost::unique_lock<boost::mutex> lock(rtinm);
			if((pom = currentTask) >= amount)
				break;
			else
				++currentTask;
		}
		(*tasks)[pom]();
	}
}

// set name for this thread.
// NOTE: on *nix string will be trimmed to 16 symbols
void setThreadName(const std::string &name)
{
#ifdef VCMI_WINDOWS
#ifndef __GNUC__
	//follows http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
	const DWORD MS_VC_EXCEPTION=0x406D1388;
#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name.c_str();
	info.dwThreadID = -1;
	info.dwFlags = 0;


	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#else
//not supported
#endif

#elif defined(__linux__)
	prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#elif defined(VCMI_APPLE)
	pthread_setname_np(name.c_str());
#elif defined(VCMI_HAIKU)
	rename_thread(find_thread(NULL), name.c_str());
#endif
}

VCMI_LIB_NAMESPACE_END
