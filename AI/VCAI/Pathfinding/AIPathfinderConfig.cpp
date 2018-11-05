/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIPathfinderConfig.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapObjects/MapObjects.h"

namespace AIPathfinding
{
	class BuildBoatAction : public ISpecialAction
	{
	private:
		const IShipyard * shipyard;

	public:
		BuildBoatAction(const IShipyard * shipyard)
			:shipyard(shipyard)
		{
		}

		virtual Goals::TSubgoal whatToDo(HeroPtr hero) const override
		{
			return sptr(Goals::BuildBoat(shipyard));
		}
	};

	class BattleAction : public ISpecialAction
	{
	private:
		const int3  target;
		const HeroPtr  hero;

	public:
		BattleAction(const int3 target)
			:target(target)
		{
		}

		virtual Goals::TSubgoal whatToDo(HeroPtr hero) const override
		{
			return sptr(Goals::VisitTile(target).sethero(hero));
		}
	};

	class AILayerTransitionRule : public LayerTransitionRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		VCAI * ai;
		std::map<int3, std::shared_ptr<const BuildBoatAction>> virtualBoats;
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AILayerTransitionRule(CPlayerSpecificInfoCallback * cb, VCAI * ai, std::shared_ptr<AINodeStorage> nodeStorage)
			:cb(cb), ai(ai), nodeStorage(nodeStorage)
		{
			setup();
		}

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override
		{
			LayerTransitionRule::process(source, destination, pathfinderConfig, pathfinderHelper);

			if(!destination.blocked)
			{
				return;
			}

			if(source.node->layer == EPathfindingLayer::LAND && destination.node->layer == EPathfindingLayer::SAIL
				&& vstd::contains(virtualBoats, destination.coord))
			{
				logAi->trace("Bypassing virtual boat at %s!", destination.coord.toString());

				nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
				{
					std::shared_ptr<const BuildBoatAction> virtualBoat = virtualBoats.at(destination.coord);

					auto boatNodeOptional = nodeStorage->getOrCreateNode(
						node->coord,
						node->layer,
						node->chainMask | AINodeStorage::RESOURCE_CHAIN);

					if(boatNodeOptional)
					{
						AIPathNode * boatNode = boatNodeOptional.get();

						boatNode->specialAction = virtualBoat;
						destination.blocked = false;
						destination.action = CGPathNode::ENodeAction::EMBARK;
						destination.node = boatNode;
					}
					else
					{
						logAi->trace(
							"Can not allocate boat node while moving %s -> %s",
							source.coord.toString(),
							destination.coord.toString());
					}
				});
			}
		}

	private:
		void setup()
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
		}
	};

	class AIMovementAfterDestinationRule : public MovementAfterDestinationRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AIMovementAfterDestinationRule(CPlayerSpecificInfoCallback * cb, std::shared_ptr<AINodeStorage> nodeStorage)
			:cb(cb), nodeStorage(nodeStorage)
		{
		}

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override
		{
			if(nodeStorage->hasBetterChain(source, destination))
			{
				destination.blocked = true;

				return;
			}

			auto blocker = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);

			if(blocker == BlockingReason::NONE)
				return;

			if(blocker == BlockingReason::DESTINATION_BLOCKVIS && destination.nodeObject)
			{
				auto objID = destination.nodeObject->ID;
				if((objID == Obj::HERO && destination.objectRelations != PlayerRelations::ENEMIES)
					|| objID == Obj::SUBTERRANEAN_GATE || objID == Obj::MONOLITH_TWO_WAY
					|| objID == Obj::MONOLITH_ONE_WAY_ENTRANCE || objID == Obj::MONOLITH_ONE_WAY_EXIT
					|| objID == Obj::WHIRLPOOL)
				{
					destination.blocked = true;
				}

				return;
			}

			if(blocker == BlockingReason::DESTINATION_VISIT)
			{
				return;
			}

			if(blocker == BlockingReason::DESTINATION_GUARDED)
			{
				auto srcGuardians = cb->getGuardingCreatures(source.coord);
				auto destGuardians = cb->getGuardingCreatures(destination.coord);

				if(destGuardians.empty())
				{
					destination.blocked = true;

					return;
				}

				vstd::erase_if(destGuardians, [&](const CGObjectInstance * destGuard) -> bool
				{
					return vstd::contains(srcGuardians, destGuard);
				});

				auto guardsAlreadyBypassed = destGuardians.empty() && srcGuardians.size();
				if(guardsAlreadyBypassed && nodeStorage->isBattleNode(source.node))
				{
					logAi->trace(
						"Bypass guard at destination while moving %s -> %s",
						source.coord.toString(),
						destination.coord.toString());

					return;
				}

				const AIPathNode * destNode = nodeStorage->getAINode(destination.node);
				auto battleNodeOptional = nodeStorage->getOrCreateNode(
					destination.coord,
					destination.node->layer,
					destNode->chainMask | AINodeStorage::BATTLE_CHAIN);

				if(!battleNodeOptional)
				{
					logAi->trace(
						"Can not allocate battle node while moving %s -> %s",
						source.coord.toString(),
						destination.coord.toString());

					destination.blocked = true;

					return;
				}

				AIPathNode *  battleNode = battleNodeOptional.get();

				if(battleNode->locked)
				{
					logAi->trace(
						"Block bypass guard at destination while moving %s -> %s",
						source.coord.toString(),
						destination.coord.toString());

					destination.blocked = true;

					return;
				}

				auto hero = nodeStorage->getHero();
				auto danger = evaluateDanger(destination.coord, hero);

				destination.node = battleNode;
				nodeStorage->commit(destination, source);

				if(battleNode->danger < danger)
				{
					battleNode->danger = danger;
				}

				battleNode->specialAction = std::make_shared<BattleAction>(destination.coord);

				logAi->trace(
					"Begin bypass guard at destination with danger %s while moving %s -> %s",
					std::to_string(danger),
					source.coord.toString(),
					destination.coord.toString());

				return;
			}

			destination.blocked = true;
		}
	};

	class AIMovementToDestinationRule : public MovementToDestinationRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AIMovementToDestinationRule(CPlayerSpecificInfoCallback * cb, std::shared_ptr<AINodeStorage> nodeStorage)
			:cb(cb), nodeStorage(nodeStorage)
		{
		}

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override
		{
			auto blocker = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);

			if(blocker == BlockingReason::NONE)
				return;

			if(blocker == BlockingReason::DESTINATION_BLOCKED
				&& destination.action == CGPathNode::EMBARK
				&& nodeStorage->getAINode(destination.node)->specialAction)
			{
				return;
			}

			if(blocker == BlockingReason::SOURCE_GUARDED && nodeStorage->isBattleNode(source.node))
			{
				logAi->trace(
					"Bypass src guard while moving from %s to %s",
					source.coord.toString(),
					destination.coord.toString());

				return;
			}

			destination.blocked = true;
		}
	};

	class AIPreviousNodeRule : public MovementToDestinationRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AIPreviousNodeRule(CPlayerSpecificInfoCallback * cb, std::shared_ptr<AINodeStorage> nodeStorage)
			:cb(cb), nodeStorage(nodeStorage)
		{
		}

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override
		{
			if(source.node->action == CGPathNode::ENodeAction::BLOCKING_VISIT || source.node->action == CGPathNode::ENodeAction::VISIT)
			{
				// we can not directly bypass objects, we need to interact with them first
				destination.node->theNodeBefore = source.node;

				logAi->trace(
					"Link src node %s to destination node %s while bypassing visitable obj",
					source.coord.toString(),
					destination.coord.toString());

				return;
			}

			auto aiSourceNode = nodeStorage->getAINode(source.node);

			if(aiSourceNode->specialAction)
			{
				// there is some action on source tile which should be performed before we can bypass it
				destination.node->theNodeBefore = source.node;
			}
		}
	};

	std::vector<std::shared_ptr<IPathfindingRule>> makeRuleset(
		CPlayerSpecificInfoCallback * cb,
		VCAI * ai,
		std::shared_ptr<AINodeStorage> nodeStorage)
	{
		std::vector<std::shared_ptr<IPathfindingRule>> rules = {
			std::make_shared<AILayerTransitionRule>(cb, ai, nodeStorage),
			std::make_shared<DestinationActionRule>(),
			std::make_shared<AIMovementToDestinationRule>(cb, nodeStorage),
			std::make_shared<MovementCostRule>(),
			std::make_shared<AIPreviousNodeRule>(cb, nodeStorage),
			std::make_shared<AIMovementAfterDestinationRule>(cb, nodeStorage)
		};

		return rules;
	}

	AIPathfinderConfig::AIPathfinderConfig(
		CPlayerSpecificInfoCallback * cb,
		VCAI * ai,
		std::shared_ptr<AINodeStorage> nodeStorage)
		:PathfinderConfig(nodeStorage, makeRuleset(cb, ai, nodeStorage))
	{
	}
}
