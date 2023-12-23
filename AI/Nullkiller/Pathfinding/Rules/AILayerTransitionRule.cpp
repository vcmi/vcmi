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
#include "../../Engine/Nullkiller.h"
#include "../../../../lib/pathfinder/CPathfinder.h"
#include "../../../../lib/pathfinder/TurnInfo.h"

namespace NKAI
{
namespace AIPathfinding
{
	AILayerTransitionRule::AILayerTransitionRule(CPlayerSpecificInfoCallback * cb, Nullkiller * ai, std::shared_ptr<AINodeStorage> nodeStorage)
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
			if(source.node->layer == EPathfindingLayer::LAND
				&& (destination.node->layer == EPathfindingLayer::AIR || destination.node->layer == EPathfindingLayer::WATER))
			{
				if(pathfinderHelper->getTurnInfo()->isLayerAvailable(destination.node->layer))
					return;
				else
					destination.blocked = true;
			}
			else
			{
				return;
			}
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::SAIL)
		{
			std::shared_ptr<const VirtualBoatAction> virtualBoat = findVirtualBoat(destination, source);

			if(virtualBoat && tryUseSpecialAction(destination, source, virtualBoat, EPathNodeAction::EMBARK))
			{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 1
				logAi->trace("Embarking to virtual boat while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::WATER)
		{
			if(nodeStorage->getAINode(source.node)->dayFlags & DayFlags::WATER_WALK_CASTED)
			{
				destination.blocked = false;
				return;
			}

			auto action = waterWalkingActions.find(nodeStorage->getHero(source.node));

			if(action != waterWalkingActions.end() && tryUseSpecialAction(destination, source, action->second, EPathNodeAction::NORMAL))
			{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace("Casting water walk while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::AIR)
		{
			if(nodeStorage->getAINode(source.node)->dayFlags & DayFlags::FLY_CASTED)
			{
				destination.blocked = false;
				return;
			}

			auto action = airWalkingActions.find(nodeStorage->getHero(source.node));

			if(action != airWalkingActions.end() && tryUseSpecialAction(destination, source, action->second, EPathNodeAction::NORMAL))
			{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace("Casting fly while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}
	}

	void AILayerTransitionRule::setup()
	{
		SpellID waterWalk = SpellID::WATER_WALK;
		SpellID airWalk = SpellID::FLY;

		for(const CGHeroInstance * hero : nodeStorage->getAllHeroes())
		{
			if(hero->canCastThisSpell(waterWalk.toSpell()) && hero->mana >= hero->getSpellCost(waterWalk.toSpell()))
			{
				waterWalkingActions[hero] = std::make_shared<WaterWalkingAction>(hero);
			}

			if(hero->canCastThisSpell(airWalk.toSpell()) && hero->mana >= hero->getSpellCost(airWalk.toSpell()))
			{
				airWalkingActions[hero] = std::make_shared<AirWalkingAction>(hero);
			}
		}

		collectVirtualBoats();
	}

	void AILayerTransitionRule::collectVirtualBoats()
	{
		std::vector<const IShipyard *> shipyards;

		for(const CGTownInstance * t : cb->getTownsInfo())
		{
			if(t->hasBuilt(BuildingID::SHIPYARD))
				shipyards.push_back(t);
		}

		for(const CGObjectInstance * obj : ai->memory->visitableObjs)
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
				virtualBoats[boatLocation] = std::make_shared<BuildBoatAction>(cb, shipyard);
				logAi->debug("Virtual boat added at %s", boatLocation.toString());
			}
		}

		for(const CGHeroInstance * hero : nodeStorage->getAllHeroes())
		{
			auto summonBoatSpell = SpellID(SpellID::SUMMON_BOAT).toSpell();

			if(hero->canCastThisSpell(summonBoatSpell)
				&& hero->getSpellSchoolLevel(summonBoatSpell) >= MasteryLevel::ADVANCED)
			{
				// TODO: For lower school level we might need to check the existance of some boat
				summonableVirtualBoats[hero] = std::make_shared<SummonBoatAction>();
			}
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
		else
		{
			const CGHeroInstance * hero = nodeStorage->getHero(source.node);

			if(vstd::contains(summonableVirtualBoats, hero)
				&& summonableVirtualBoats.at(hero)->canAct(nodeStorage->getAINode(source.node)))
			{
				virtualBoat = summonableVirtualBoats.at(hero);
			}
		}

		return virtualBoat;
	}

	bool AILayerTransitionRule::tryUseSpecialAction(
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		std::shared_ptr<const SpecialAction> specialAction,
		EPathNodeAction targetAction) const
	{
		bool result = false;

		nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
			{
				auto castNodeOptional = nodeStorage->getOrCreateNode(
					node->coord,
					node->layer,
					specialAction->getActor(node->actor));

				if(castNodeOptional)
				{
					AIPathNode * castNode = castNodeOptional.value();

					if(castNode->action == EPathNodeAction::UNKNOWN)
					{
						castNode->addSpecialAction(specialAction);
						destination.blocked = false;
						destination.action = targetAction;
						destination.node = castNode;
						result = true;
					}
					else
					{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 1
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

}
