/*
* AILayerTransitionRule.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AILayerTransitionRule.h"

namespace AIPathfinding
{
	AILayerTransitionRule::AILayerTransitionRule(CPlayerSpecificInfoCallback * cb, VCAI * ai, std::shared_ptr<AINodeStorage> nodeStorage)
		:cb(cb), ai(ai), nodeStorage(nodeStorage)
	{
		setup();
	}

	void AILayerTransitionRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		LayerTransitionRule::process(source, destination, pathfinderConfig, pathfinderHelper);

		if(!destination.blocked)
		{
			return;
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::SAIL)
		{
			std::shared_ptr<const VirtualBoatAction> virtualBoat = findVirtualBoat(destination, source);

			if(virtualBoat && tryEmbarkVirtualBoat(destination, source, virtualBoat))
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace("Embarking to virtual boat while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}
	}

	void AILayerTransitionRule::setup()
	{
		std::vector<const IShipyard *> shipyards;

		for(const CGTownInstance * t : cb->getTownsInfo())
		{
			if(t->hasBuilt(BuildingID::SHIPYARD))
				shipyards.push_back(t);
		}

		for(const CGObjectInstance * obj : ai->visitableObjs)
		{
			if(obj->ID != Obj::TOWN) //towns were handled in the previous loop
			{
				if(const IShipyard * shipyard = IShipyard::castFrom(obj))
					shipyards.push_back(shipyard);
			}
		}

		for(const IShipyard * shipyard : shipyards)
		{
			if(shipyard->shipyardStatus() == IShipyard::GOOD)
			{
				int3 boatLocation = shipyard->bestLocation();
				virtualBoats[boatLocation] = std::make_shared<BuildBoatAction>(shipyard);
				logAi->debug("Virtual boat added at %s", boatLocation.toString());
			}
		}

		auto hero = nodeStorage->getHero();
		auto summonBoatSpell = SpellID(SpellID::SUMMON_BOAT).toSpell();

		if(hero->canCastThisSpell(summonBoatSpell)
			&& hero->getSpellSchoolLevel(summonBoatSpell) >= SecSkillLevel::ADVANCED)
		{
			// TODO: For lower school level we might need to check the existance of some boat
			summonableVirtualBoat.reset(new SummonBoatAction());
		}
	}

	std::shared_ptr<const VirtualBoatAction> AILayerTransitionRule::findVirtualBoat(
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source) const
	{
		std::shared_ptr<const VirtualBoatAction> virtualBoat;

		if(vstd::contains(virtualBoats, destination.coord))
		{
			virtualBoat = virtualBoats.at(destination.coord);
		}
		else if(
			summonableVirtualBoat
			&& summonableVirtualBoat->isAffordableBy(nodeStorage->getHero(), nodeStorage->getAINode(source.node)))
		{
			virtualBoat = summonableVirtualBoat;
		}

		return virtualBoat;
	}

	bool AILayerTransitionRule::tryEmbarkVirtualBoat(
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		std::shared_ptr<const VirtualBoatAction> virtualBoat) const
	{
		bool result = false;

		nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
		{
			auto boatNodeOptional = nodeStorage->getOrCreateNode(
				node->coord,
				node->layer,
				(int)(node->chainMask | virtualBoat->getSpecialChain()));

			if(boatNodeOptional)
			{
				AIPathNode * boatNode = boatNodeOptional.value();

				if(boatNode->action == EPathNodeAction::UNKNOWN)
				{
					boatNode->specialAction = virtualBoat;
					destination.blocked = false;
					destination.action = EPathNodeAction::EMBARK;
					destination.node = boatNode;
					result = true;
				}
				else
				{
#ifdef VCMI_TRACE_PATHFINDER
					logAi->trace(
						"Special transition node already allocated. Blocked moving %s -> %s",
						source.coord.toString(),
						destination.coord.toString());
#endif
				}
			}
			else
			{
				logAi->debug(
					"Can not allocate special transition node while moving %s -> %s",
					source.coord.toString(),
					destination.coord.toString());
			}
		});

		return result;
	}
}
