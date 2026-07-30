/*
 * AssetCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AssetCache.h"

#ifdef VCMI_WINDOWS
#include <windows.h>
#elif defined(VCMI_APPLE)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(VCMI_UNIX)
#include <unistd.h>
#endif

namespace AssetCache
{

size_t getTotalSystemMemory()
{
#ifdef VCMI_WINDOWS
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	if(GlobalMemoryStatusEx(&status))
		return status.ullTotalPhys;
	return 0;
#elif defined(VCMI_APPLE)
	int mib[2] = { CTL_HW, HW_MEMSIZE };
	uint64_t size = 0;
	size_t length = sizeof(size);
	if(sysctl(mib, 2, &size, &length, nullptr, 0) == 0)
		return size;
	return 0;
#elif defined(VCMI_UNIX)
	long pages = sysconf(_SC_PHYS_PAGES);
	long pageSize = sysconf(_SC_PAGE_SIZE);
	if(pages > 0 && pageSize > 0)
		return static_cast<size_t>(pages) * static_cast<size_t>(pageSize);
	return 0;
#else
	return 0;
#endif
}

size_t getRetentionBudget(int configuredMegabytes)
{
	static constexpr size_t megabyte = 1024 * 1024;

	if(configuredMegabytes > 0)
		return static_cast<size_t>(configuredMegabytes) * megabyte;

	// Auto mode. The budget is derived from *total* rather than currently free RAM
	// on purpose: free memory fluctuates with whatever else the machine is doing,
	// and a cache that shrinks and grows with it would reintroduce exactly the
	// reload thrashing this cache exists to prevent. A small fixed share of total
	// RAM is predictable and leaves the OS free to reclaim page cache instead.
	const size_t totalMemory = getTotalSystemMemory();

#ifdef VCMI_MOBILE
	// Mobile systems kill background apps aggressively, so stay modest
	static constexpr size_t minimalBudget = 32 * megabyte;
	static constexpr size_t maximalBudget = 192 * megabyte;
	static constexpr size_t memoryShare = 32;
#else
	static constexpr size_t minimalBudget = 64 * megabyte;
	static constexpr size_t maximalBudget = 512 * megabyte;
	static constexpr size_t memoryShare = 16;
#endif

	if(totalMemory == 0)
		return minimalBudget; // could not detect RAM - be conservative

	return std::clamp(totalMemory / memoryShare, minimalBudget, maximalBudget);
}

}
