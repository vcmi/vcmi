/*
 * PathfinderCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "PathfinderOptions.h"

class IGameInfoCallback;
class CGHeroInstance;
class PathfinderConfig;
struct CPathsInfo;

class DLL_LINKAGE PathfinderCache
{
	const IGameInfoCallback * cb;
	std::mutex pathCacheMutex;
	std::map<const CGHeroInstance *, std::shared_ptr<CPathsInfo>> pathCache;
	std::vector<std::shared_ptr<CPathsInfo>> reusablePaths;
	PathfinderOptions options;

	std::shared_ptr<PathfinderConfig> createConfig(const CGHeroInstance *h, CPathsInfo &out);
	std::shared_ptr<CPathsInfo> acquirePaths(const CGHeroInstance * h);
	std::shared_ptr<CPathsInfo> buildPaths(const CGHeroInstance *h);
public:
	PathfinderCache(const IGameInfoCallback * cb, const PathfinderOptions & options);

	/// Invalidates and erases all existing paths from the cache
	void invalidatePaths();

	/// Returns compute path information for requested hero
	std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h);
};
