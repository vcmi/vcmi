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
#include "../VCAI.h"
#include "../../../lib/pathfinder/PathfinderOptions.h"

namespace AIPathfinding
{
	class AIPathfinderConfig : public PathfinderConfig
	{
	private:
		const CGHeroInstance * hero;
		std::unique_ptr<CPathfinderHelper> helper;

	public:
		AIPathfinderConfig(
			CPlayerSpecificInfoCallback * cb,
			VCAI * ai,
			std::shared_ptr<AINodeStorage> nodeStorage);

		~AIPathfinderConfig();

		virtual CPathfinderHelper * getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs) override;
	};
}
