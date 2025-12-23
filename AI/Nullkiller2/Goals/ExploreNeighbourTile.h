/*
* ExploreNeighbourTile.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace NK2AI
{

class AIGateway;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT ExploreNeighbourTile : public ElementarGoal<ExploreNeighbourTile>
	{
	private:
		int tilesToExplore;

	public:
		ExploreNeighbourTile(const CGHeroInstance * hero,  int amount)
			: ElementarGoal(Goals::EXPLORE_NEIGHBOUR_TILE)
		{
			tilesToExplore = amount;
			sethero(hero);
		}

		bool operator==(const ExploreNeighbourTile & other) const override;

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;

	private:
		//TSubgoal decomposeSingle() const override;
	};
}

}
