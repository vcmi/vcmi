/*
 * PathfinderCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PathfinderCache.h"

#include "CGPathNode.h"
#include "PathfinderOptions.h"

#include "../callback/IGameInfoCallback.h"
#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<PathfinderConfig> PathfinderCache::createConfig(const CGHeroInstance * h, CPathsInfo & out)
{
	auto config = std::make_shared<SingleHeroPathfinderConfig>(out, *cb, h);
	config->options = options;

	return config;
}

std::shared_ptr<CPathsInfo> PathfinderCache::buildPaths(const CGHeroInstance * h)
{
	auto result = std::make_shared<CPathsInfo>(cb->getMapSize(), h);
	auto config = createConfig(h, *result);

	cb->calculatePaths(config);
	return result;
}

PathfinderCache::PathfinderCache(const IGameInfoCallback * cb, const PathfinderOptions & options)
	: cb(cb)
	, options(options)
{
}

void PathfinderCache::invalidatePaths()
{
	std::lock_guard lock(pathCacheMutex);
	pathCache.clear();
}

std::shared_ptr<const CPathsInfo> PathfinderCache::getPathsInfo(const CGHeroInstance * h)
{
	std::lock_guard lock(pathCacheMutex);

	auto iter = pathCache.find(h);
	if(iter == std::end(pathCache) || iter->second->heroBonusTreeVersion != h->getTreeVersion())
	{
		auto result = buildPaths(h);
		pathCache[h] = result;

		return result;
	}
	else
		return iter->second;
}

VCMI_LIB_NAMESPACE_END
