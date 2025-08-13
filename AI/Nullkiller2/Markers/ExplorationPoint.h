/*
* ExplorationPoint.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Pathfinding/AINodeStorage.h"

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT ExplorationPoint : public CGoal<ExplorationPoint>
	{
	public:
		ExplorationPoint(int3 tile, int tilesToReviel)
			: CGoal(Goals::EXPLORATION_POINT)
		{
			settile(tile);
			setvalue(tilesToReviel);
		}

		bool operator==(const ExplorationPoint & other) const override;

		std::string toString() const override;
	};
}

}
