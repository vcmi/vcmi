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
class Nullkiller;

struct NeighbourExplorationCandidate
{
	bool sameDay = false;
	bool accessible = false;
	bool safe = false;
	int tilesDiscovered = 0;
	float strategicProgress = 0.0f;
	float movementCost = 0.0f;
};

struct NeighbourExplorationEvaluation
{
	bool accepted = false;
	float value = 0.0f;
};

struct NeighbourExplorationTarget
{
	int3 tile = int3(-1);
	int tilesDiscovered = 0;
	float movementCost = 0.0f;
	float strategicProgress = 0.0f;
	float value = 0.0f;
};

NeighbourExplorationEvaluation evaluateNeighbourExplorationCandidate(
	const NeighbourExplorationCandidate & candidate);

namespace Goals
{
	class DLL_EXPORT ExploreNeighbourTile : public ElementarGoal<ExploreNeighbourTile>
	{
	private:
		int tilesToExplore;
		bool lockAfterMove;

	public:
		ExploreNeighbourTile(const CGHeroInstance * hero, int amount, bool lockAfterMove = false)
			: ElementarGoal(Goals::EXPLORE_NEIGHBOUR_TILE)
			, lockAfterMove(lockAfterMove)
		{
			tilesToExplore = amount;
			sethero(hero);
		}

		bool operator==(const ExploreNeighbourTile & other) const override;

		static float evaluateTileScore(int tilesDiscovered, float movementCost)
		{
			return static_cast<float>(tilesDiscovered) / std::max(0.1f, movementCost);
		}

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		static std::optional<NeighbourExplorationTarget> findTarget(
			const CGHeroInstance * hero,
			const Nullkiller * aiNk);

	private:
		//TSubgoal decomposeSingle() const override;
	};
}

}
