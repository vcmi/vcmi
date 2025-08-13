#pragma once

#include "Engine/Nullkiller.h"

namespace NKAI
{

template<typename TFunc>
void pforeachTilePos(const int3 & mapSize, TFunc fn)
{
	for(int z = 0; z < mapSize.z; ++z)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(0, mapSize.x), [&](const tbb::blocked_range<size_t> & r)
			{
				int3 pos(0, 0, z);

				for(pos.x = r.begin(); pos.x != r.end(); ++pos.x)
				{
					for(pos.y = 0; pos.y < mapSize.y; ++pos.y)
					{
						fn(pos);
					}
				}
			});
	}
}

template<typename TFunc>
void pforeachTilePaths(const int3 & mapSize, const Nullkiller * ai, TFunc fn)
{
	for(int z = 0; z < mapSize.z; ++z)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(0, mapSize.x), [&](const tbb::blocked_range<size_t> & r)
			{
				int3 pos(0, 0, z);
				std::vector<AIPath> paths;

				for(pos.x = r.begin(); pos.x != r.end(); ++pos.x)
				{
					for(pos.y = 0; pos.y < mapSize.y; ++pos.y)
					{
						ai->pathfinder->calculatePathInfo(paths, pos);
						fn(pos, paths);
					}
				}
			});
	}
}

}