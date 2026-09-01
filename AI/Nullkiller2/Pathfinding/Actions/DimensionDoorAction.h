/*
* DimensionDoorAction.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "SpecialAction.h"

class CSpell;

namespace NK2AI::AIPathfinding
{
	struct DimensionDoorActionValidation
	{
		const CGHeroInstance * hero = nullptr;
		const CSpell * spell = nullptr;
		int manaCost = 0;
		int manaAlreadySpent = 0;
		int movementPointsRemaining = 0;
		int movementPointsRequired = 0;
		int castsLimit = 0;
		int castsAlreadyPerformed = 0;
		int plannedDimensionDoorCasts = 0;
	};

	bool hasDimensionDoorActionResources(const DimensionDoorActionValidation & validation);
	bool canUseDimensionDoorAction(const DimensionDoorActionValidation & validation);

	struct DimensionDoorActionParameters
	{
		SpellID usedSpell;
		int3 destination;
		int manaCost = 0;
		int movementPointsRequired = 0;
		int movementPointsTaken = 0;
		int plannedSourceTurn = 0;
		int plannedSourceMoveRemains = 0;
		int plannedSourceMoveLimit = 1;
		int plannedDimensionDoorCasts = 0;
		uint64_t guardedLandingDanger = 0;
		uint64_t guardedLandingArmyLoss = 0;
	};

	class DimensionDoorAction : public SpecialAction
	{
	private:
		SpellID usedSpell;
		int3 destination;
		int manaCost;
		int movementPointsRequired;
		int movementPointsTaken;
		int plannedSourceTurn;
		int plannedSourceMoveRemains;
		int plannedSourceMoveLimit;
		int plannedDimensionDoorCasts;
		uint64_t guardedLandingDanger;
		uint64_t guardedLandingArmyLoss;

	public:
		explicit DimensionDoorAction(const DimensionDoorActionParameters & parameters);

		bool canAct(const Nullkiller * aiNk, const AIPathNode * source) const override;
		void execute(AIGateway * aiGw, const CGHeroInstance * hero) const override;
		void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstNode,
			const AIPathNode * srcNode) const override;
		const ChainActor * getActor(const ChainActor * sourceActor) const override;
		std::string toString() const override;
	};
}
