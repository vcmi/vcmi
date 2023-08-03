/*
* BoatActions.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "../../AIGateway.h"
#include "../../Goals/AdventureSpellCast.h"
#include "../../Goals/CaptureObject.h"
#include "../../Goals/Invalid.h"
#include "../../Goals/BuildBoat.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "BoatActions.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

namespace AIPathfinding
{
	void BuildBoatAction::execute(const CGHeroInstance * hero) const
	{
		return Goals::BuildBoat(shipyard).accept(ai.get());
	}

	Goals::TSubgoal BuildBoatAction::decompose(const CGHeroInstance * hero) const
	{
		if(cb->getPlayerRelations(ai->playerID, shipyard->getObject()->getOwner()) == PlayerRelations::ENEMIES)
		{
			return Goals::sptr(Goals::CaptureObject(targetObject()));
		}
		
		return Goals::sptr(Goals::Invalid());
	}

	bool BuildBoatAction::canAct(const AIPathNode * source) const
	{
		auto hero = source->actor->hero;

		if(cb->getPlayerRelations(hero->tempOwner, shipyard->getObject()->getOwner()) == PlayerRelations::ENEMIES)
		{
#if NKAI_TRACE_LEVEL > 1
			logAi->trace("Can not build a boat. Shipyard is enemy.");
#endif
			return false;
		}

		TResources boatCost;

		shipyard->getBoatCost(boatCost);

		if(!cb->getResourceAmount().canAfford(source->actor->armyCost + boatCost))
		{
#if NKAI_TRACE_LEVEL > 1
			logAi->trace("Can not build a boat. Not enough resources.");
#endif

			return false;
		}

		return true;
	}

	const CGObjectInstance * BuildBoatAction::targetObject() const
	{
		return dynamic_cast<const CGObjectInstance*>(shipyard);
	}

	const ChainActor * BuildBoatAction::getActor(const ChainActor * sourceActor) const
	{
		return sourceActor->resourceActor;
	}

	void SummonBoatAction::execute(const CGHeroInstance * hero) const
	{
		Goals::AdventureSpellCast(hero, SpellID::SUMMON_BOAT).accept(ai.get());
	}

	const ChainActor * SummonBoatAction::getActor(const ChainActor * sourceActor) const
	{
		return sourceActor->castActor;
	}

	void SummonBoatAction::applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
		dstMode->manaCost = srcNode->manaCost + getManaCost(hero);
		dstMode->theNodeBefore = source.node;
	}

	std::string BuildBoatAction::toString() const
	{
		return "Build Boat at " + shipyard->getObject()->visitablePos().toString();
	}

	bool SummonBoatAction::canAct(const AIPathNode * source) const
	{
		auto hero = source->actor->hero;

#ifdef VCMI_TRACE_PATHFINDER
		logAi->trace(
			"Hero %s has %d mana and needed %d and already spent %d",
			hero->name,
			hero->mana,
			getManaCost(hero),
			source->manaCost);
#endif

		return hero->mana >= (si32)(source->manaCost + getManaCost(hero));
	}

	std::string SummonBoatAction::toString() const
	{
		return "Summon Boat";
	}

	uint32_t SummonBoatAction::getManaCost(const CGHeroInstance * hero) const
	{
		SpellID summonBoat = SpellID::SUMMON_BOAT;

		return hero->getSpellCost(summonBoat.toSpell());
	}
}

}
