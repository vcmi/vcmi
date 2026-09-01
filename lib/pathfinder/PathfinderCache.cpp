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

std::shared_ptr<PathfinderConfig> PathfinderCache::createConfig(const CGHeroInstance * h, CPathsInfo & out)
{
	auto config = std::make_shared<SingleHeroPathfinderConfig>(out, *cb, h);
	config->options = options;

	return config;
}

std::shared_ptr<CPathsInfo> PathfinderCache::buildPaths(const CGHeroInstance * h)
{
	auto result = acquirePaths(h);
	auto config = createConfig(h, *result);

	cb->calculatePaths(config);
	return result;
}

std::shared_ptr<CPathsInfo> PathfinderCache::acquirePaths(const CGHeroInstance * h)
{
	if(reusablePaths.empty())
		return std::make_shared<CPathsInfo>(cb->getMapSize(), h);

	auto result = std::move(reusablePaths.back());
	reusablePaths.pop_back();
	result->prepareForReuse(h);
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
	reusablePaths.reserve(reusablePaths.size() + pathCache.size());
	for(auto & entry : pathCache)
	{
		if(entry.second.use_count() == 1)
			reusablePaths.push_back(std::move(entry.second));
	}
	pathCache.clear();
}

std::shared_ptr<const CPathsInfo> PathfinderCache::getPathsInfo(const CGHeroInstance * h)
{
	std::lock_guard lock(pathCacheMutex);

	auto iter = pathCache.find(h);
	if(iter == std::end(pathCache) || iter->second->heroBonusTreeVersion != h->getTreeVersion())
	{
		if(iter != std::end(pathCache) && iter->second.use_count() == 1)
		{
			reusablePaths.reserve(reusablePaths.size() + 1);
			reusablePaths.push_back(std::move(iter->second));
			pathCache.erase(iter);
		}

		auto result = buildPaths(h);
		pathCache[h] = result;

		return result;
	}
	else
		return iter->second;
}
