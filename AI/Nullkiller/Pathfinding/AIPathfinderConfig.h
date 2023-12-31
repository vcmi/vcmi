/*
* AIPathfinderConfig.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AINodeStorage.h"
#include "../../../lib/pathfinder/PathfinderOptions.h"

namespace NKAI
{

class Nullkiller;

namespace AIPathfinding
{
	class AIPathfinderConfig : public PathfinderConfig
	{
	private:
		std::map<const CGHeroInstance *, std::unique_ptr<CPathfinderHelper>> pathfindingHelpers;
		std::shared_ptr<AINodeStorage> aiNodeStorage;

	public:
		AIPathfinderConfig(
			CPlayerSpecificInfoCallback * cb,
			Nullkiller * ai,
			std::shared_ptr<AINodeStorage> nodeStorage);

		~AIPathfinderConfig();

		virtual CPathfinderHelper * getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs) override;
	};
}

}
