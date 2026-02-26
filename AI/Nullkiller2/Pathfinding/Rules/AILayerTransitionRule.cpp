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
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AILayerTransitionRule.h"

#include "../../../../lib/pathfinder/CPathfinder.h"
#include "../../../../lib/pathfinder/TurnInfo.h"
#include "../../../../lib/spells/ISpellMechanics.h"
#include "../../../../lib/spells/adventure/SummonBoatEffect.h"

namespace NK2AI
{
namespace AIPathfinding
{
	AILayerTransitionRule::AILayerTransitionRule(Nullkiller * aiNk, std::shared_ptr<AINodeStorage> nodeStorage) : aiNk(aiNk), nodeStorage(nodeStorage)
	{
		setup();
	}

	void AILayerTransitionRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper
	) const
	{
		LayerTransitionRule::process(source, destination, pathfinderConfig, pathfinderHelper);

#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Layer transitioning %s -> %s, action: %d, blocked: %s",
			source.coord.toString(),
			destination.coord.toString(),
			static_cast<int32_t>(destination.action),
			destination.blocked ? "true" : "false"
		);
#endif

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
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 1
				logAi->trace("Embarking to virtual boat while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::WATER)
		{
			if(nodeStorage->getAINode(source.node)->dayFlags & DayFlags::WATER_WALK_CAST)
			{
				destination.blocked = false;
				return;
			}

			auto action = waterWalkingActions.find(nodeStorage->getHero(source.node));

			if(action != waterWalkingActions.end() && tryUseSpecialAction(destination, source, action->second, EPathNodeAction::NORMAL))
			{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace("Casting water walk while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}

		if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::AIR)
		{
			if(nodeStorage->getAINode(source.node)->dayFlags & DayFlags::FLY_CAST)
			{
				destination.blocked = false;
				return;
			}

			auto action = airWalkingActions.find(nodeStorage->getHero(source.node));

			if(action != airWalkingActions.end() && tryUseSpecialAction(destination, source, action->second, EPathNodeAction::NORMAL))
			{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace("Casting fly while moving %s -> %s!", source.coord.toString(), destination.coord.toString());
#endif
			}
		}
	}

	void AILayerTransitionRule::setup()
	{
		for(const CGHeroInstance * hero : nodeStorage->getAllHeroes())
		{
			for(const auto & spell : LIBRARY->spellh->objects)
			{
				if(!spell || !spell->isAdventure())
					continue;

				if(spell->getAdventureMechanics().givesBonus(hero, BonusType::WATER_WALKING) && hero->canCastThisSpell(spell.get())
				   && hero->mana >= hero->getSpellCost(spell.get()))
				{
					waterWalkingActions[hero] = std::make_shared<WaterWalkingAction>(hero, spell->id);
				}

				if(spell->getAdventureMechanics().givesBonus(hero, BonusType::FLYING_MOVEMENT) && hero->canCastThisSpell(spell.get())
				   && hero->mana >= hero->getSpellCost(spell.get()))
				{
					airWalkingActions[hero] = std::make_shared<AirWalkingAction>(hero, spell->id);
				}
			}
		}

		collectVirtualBoats();
	}

	void AILayerTransitionRule::collectVirtualBoats()
	{
		std::vector<const IShipyard *> shipyards;

		for(const CGTownInstance * t : aiNk->cc->getTownsInfo())
		{
			if(t->hasBuilt(BuildingID::SHIPYARD))
				shipyards.push_back(t);
		}

		for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
		{
			const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
			if(obj && obj->ID != Obj::TOWN) //towns were handled in the previous loop
			{
				if(const auto * shipyard = dynamic_cast<const IShipyard *>(obj))
					shipyards.push_back(shipyard);
			}
		}

		for(const IShipyard * shipyard : shipyards)
		{
			if(shipyard->shipyardStatus() == IShipyard::GOOD)
			{
				int3 boatLocation = shipyard->bestLocation();
				virtualBoats[boatLocation] = std::make_shared<BuildBoatAction>(aiNk->cc.get(), shipyard);
				logAi->debug("Virtual boat added at %s", boatLocation.toString());
			}
		}

		for(const CGHeroInstance * hero : nodeStorage->getAllHeroes())
		{
			for(const auto & spell : LIBRARY->spellh->objects)
			{
				if(!spell || !spell->isAdventure())
					continue;

				auto effect = spell->getAdventureMechanics().getEffectAs<SummonBoatEffect>(hero);

				if(!effect || !hero->canCastThisSpell(spell.get()))
					continue;

				if(effect->canCreateNewBoat() && effect->getSuccessChance(hero) == 100)
				{
					// TODO: For lower school level we might need to check the existence of some boat
					summonableVirtualBoats[hero] = std::make_shared<SummonBoatAction>(spell->id);
				}
			}
		}
	}

	std::shared_ptr<const VirtualBoatAction> AILayerTransitionRule::findVirtualBoat(CDestinationNodeInfo & destination, const PathNodeInfo & source) const
	{
		std::shared_ptr<const VirtualBoatAction> virtualBoat;

		if(vstd::contains(virtualBoats, destination.coord))
		{
			virtualBoat = virtualBoats.at(destination.coord);
		}
		else
		{
			const CGHeroInstance * hero = nodeStorage->getHero(source.node);

			if(vstd::contains(summonableVirtualBoats, hero) && summonableVirtualBoats.at(hero)->canAct(aiNk, nodeStorage->getAINode(source.node)))
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
		EPathNodeAction targetAction
	) const
	{
		bool result = false;

		nodeStorage->updateAINode(
			destination.node,
			[&](const AIPathNode * node)
			{
				const auto castNodeOptional = nodeStorage->getOrCreateNode(node->coord, node->layer, specialAction->getActor(node->actor));
				if(!castNodeOptional)
				{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 1
					logAi->warn(
						"P:Step2A AILayerTransitionRule::tryUseSpecialAction Failed to allocate node at %s[%d]. "
						"Can not allocate special transition node while moving %s -> %s",
						node->coord.toString(),
						static_cast<int32_t>(node->layer),
						source.coord.toString(),
						destination.coord.toString()
					);
#endif
				}
				else
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
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
						logAi->trace(
							"AILayerTransitionRule::tryUseSpecialAction Special transition node already allocated. Blocked moving %s -> %s",
							source.coord.toString(),
							destination.coord.toString()
						);
#endif
					}
				}
			}
		);

		return result;
	}
}

}
