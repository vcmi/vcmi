/*
* DimensionDoorAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "DimensionDoorAction.h"

#include "../../AIGateway.h"
#include "../../Engine/Nullkiller.h"
#include "../../Goals/AdventureSpellCast.h"
#include "../Actors.h"
#include "../AINodeStorage.h"
#include "../../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../../lib/spells/CSpell.h"
#include "../../../../lib/spells/ISpellMechanics.h"

namespace NK2AI::AIPathfinding
{
	bool hasDimensionDoorActionResources(const DimensionDoorActionValidation & validation)
	{
		if(!validation.hero)
			return false;

		const int plannedAndPerformedCasts = validation.castsAlreadyPerformed
			+ validation.plannedDimensionDoorCasts;

		return validation.hero->mana >= validation.manaAlreadySpent + validation.manaCost
			&& validation.movementPointsRemaining > validation.movementPointsRequired
			&& (validation.castsLimit <= 0 || plannedAndPerformedCasts < validation.castsLimit);
	}

	bool canUseDimensionDoorAction(const DimensionDoorActionValidation & validation)
	{
		return validation.hero
			&& validation.spell
			&& validation.hero->canCastThisSpell(validation.spell)
			&& hasDimensionDoorActionResources(validation);
	}

	DimensionDoorAction::DimensionDoorAction(const DimensionDoorActionParameters & parameters)
		: usedSpell(parameters.usedSpell)
		, destination(parameters.destination)
		, manaCost(parameters.manaCost)
		, movementPointsRequired(parameters.movementPointsRequired)
		, movementPointsTaken(parameters.movementPointsTaken)
		, plannedSourceTurn(parameters.plannedSourceTurn)
		, plannedSourceMoveRemains(parameters.plannedSourceMoveRemains)
		, plannedSourceMoveLimit(parameters.plannedSourceMoveLimit)
		, plannedDimensionDoorCasts(parameters.plannedDimensionDoorCasts)
		, guardedLandingDanger(parameters.guardedLandingDanger)
		, guardedLandingArmyLoss(parameters.guardedLandingArmyLoss)
	{
	}

	bool DimensionDoorAction::canAct(const Nullkiller * aiNk, const AIPathNode * source) const
	{
		const auto * hero = source->actor->hero;
		const auto * spell = usedSpell.toSpell();

		if(!hero || !spell)
			return false;

		const auto & mechanics = spell->getAdventureMechanics();
		return canUseDimensionDoorAction({
			hero,
			spell,
			manaCost,
			source->manaCost,
			plannedSourceMoveRemains,
			movementPointsRequired,
			mechanics.getCastsLimit(hero, aiNk->cc->getMapSize()),
			plannedSourceTurn == 0 ? mechanics.getCastsAlreadyPerformed(hero) : 0,
			plannedDimensionDoorCasts
		});
	}

	void DimensionDoorAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
	{
		auto goal = Goals::AdventureSpellCast(hero, usedSpell);
		goal.tile = destination;
		goal.accept(aiGw);
	}

	void DimensionDoorAction::applyOnDestination(
		const CGHeroInstance *,
		CDestinationNodeInfo & destinationInfo,
		const PathNodeInfo & source,
		AIPathNode * dstNode,
		const AIPathNode * srcNode) const
	{
		dstNode->manaCost = srcNode->manaCost + manaCost;
		dstNode->dimensionDoorCasts = plannedDimensionDoorCasts + 1;
		dstNode->theNodeBefore = source.node;
		dstNode->moveRemains = std::max(0, plannedSourceMoveRemains - movementPointsTaken);
		dstNode->setCost(srcNode->getCost() + static_cast<float>(std::min(plannedSourceMoveRemains, movementPointsTaken)) / plannedSourceMoveLimit);
		dstNode->armyLoss += guardedLandingArmyLoss;
		dstNode->danger = std::max(dstNode->danger, guardedLandingDanger);
		destinationInfo.cost = dstNode->getCost();
		destinationInfo.movementLeft = dstNode->moveRemains;
	}

	const ChainActor * DimensionDoorAction::getActor(const ChainActor * sourceActor) const
	{
		return sourceActor->castActor;
	}

	std::string DimensionDoorAction::toString() const
	{
		return "Cast " + usedSpell.toSpell()->getNameTranslated() + " to " + destination.toString();
	}
}
