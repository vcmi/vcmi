/*
* AIMovementAfterDestinationRule.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIMovementAfterDestinationRule.h"
#include "../Actions/BattleAction.h"
#include "../Actions/QuestAction.h"
#include "../Actions/WhirlpoolAction.h"
#include "../../Goals/Invalid.h"
#include "AIPreviousNodeRule.h"
#include "../../../../lib/mapObjects/CQuest.h"
#include "../../../../lib/pathfinder/PathfinderOptions.h"
#include "../../../../lib/pathfinder/CPathfinder.h"

namespace NK2AI
{
namespace AIPathfinding
{
	AIMovementAfterDestinationRule::AIMovementAfterDestinationRule(
		const Nullkiller * aiNk,
		std::shared_ptr<AINodeStorage> nodeStorage,
		bool allowBypassObjects)
		:aiNk(aiNk), nodeStorage(nodeStorage), allowBypassObjects(allowBypassObjects)
	{
	}

	void AIMovementAfterDestinationRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		if(nodeStorage->isMovementInefficient(source, destination))
		{
			destination.node->locked = true;
			destination.blocked = true;

			return;
		}
		
		if(!allowBypassObjects
			&& destination.action == EPathNodeAction::EMBARK
			&& source.node->layer == EPathfindingLayer::LAND
			&& destination.node->layer == EPathfindingLayer::SAIL)
		{
			destination.blocked = true;

			return;
		}

		auto blocker = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);

		if(blocker == BlockingReason::NONE)
		{
			destination.blocked = nodeStorage->isDistanceLimitReached(source, destination);

			if(destination.nodeObject
				&& !destination.blocked
				&& !allowBypassObjects
				&& !dynamic_cast<const CGTeleport *>(destination.nodeObject)
				&& destination.nodeObject->ID != Obj::EVENT)
			{
				destination.blocked = true;
				destination.node->locked = true;
			}

			return;
		}
		
		if(!allowBypassObjects)
		{
			if(destination.nodeObject)
			{
				destination.blocked = true;
				destination.node->locked = true;
			}

			return;
		}

#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Movement from tile %s is blocked. Try to bypass. Action: %d, blocker: %d, source: %s",
			destination.coord.toString(),
			(int)destination.action,
			(int)blocker,
			source.coord.toString());
#endif

		auto destGuardians = aiNk->cc->getGuardingCreatures(destination.coord);
		bool allowBypass = false;

		switch(blocker)
		{
		case BlockingReason::DESTINATION_GUARDED:
			allowBypass = bypassDestinationGuards(destGuardians, source, destination, pathfinderConfig, pathfinderHelper);

			break;

		case BlockingReason::DESTINATION_BLOCKVIS:
			if(destination.nodeHero && destination.heroRelations != PlayerRelations::ENEMIES)
			{
				allowBypass = destination.heroRelations == PlayerRelations::SAME_PLAYER
					&& destination.nodeHero == nodeStorage->getHero(destination.node);
			}
			else
			{
				allowBypass = destination.nodeObject && bypassRemovableObject(source, destination, pathfinderConfig, pathfinderHelper);
			}
			
			if(allowBypass && destGuardians.size())
				allowBypass = bypassDestinationGuards(destGuardians, source, destination, pathfinderConfig, pathfinderHelper);

			break;

		case BlockingReason::DESTINATION_VISIT:
			allowBypass = true;

			break;

		case BlockingReason::DESTINATION_BLOCKED:
			allowBypass = bypassBlocker(source, destination, pathfinderConfig, pathfinderHelper);

			break;
		}

		destination.blocked = !allowBypass || nodeStorage->isDistanceLimitReached(source, destination);
		destination.node->locked = !allowBypass;
	}

	bool AIMovementAfterDestinationRule::bypassBlocker(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		auto enemyHero = destination.nodeHero && destination.heroRelations == PlayerRelations::ENEMIES;

		if(enemyHero)
		{
			return bypassBattle(source, destination, pathfinderConfig, pathfinderHelper);
		}

		if(destination.nodeObject
			&& (destination.nodeObject->ID == Obj::GARRISON || destination.nodeObject->ID == Obj::GARRISON2)
			&& destination.objectRelations == PlayerRelations::ENEMIES)
		{
			return bypassBattle(source, destination, pathfinderConfig, pathfinderHelper);
		}

		return false;
	}

	bool AIMovementAfterDestinationRule::bypassQuest(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		const AIPathNode * destinationNode = nodeStorage->getAINode(destination.node);
		auto questObj = dynamic_cast<const IQuestObject *>(destination.nodeObject);
		auto questInfo = QuestInfo(destination.nodeObject->id);
		QuestAction questAction(questInfo);

		if(destination.nodeObject->ID == Obj::QUEST_GUARD
		   && questObj->getQuest().mission == Rewardable::Limiter{}
		   && questObj->getQuest().killTarget == ObjectInstanceID::NONE)
		{
			return false;
		}

		if(!questAction.canAct(aiNk, destinationNode))
		{
			if(!destinationNode->actor->allowUseResources)
			{
				const std::optional<AIPathNode *> questNode = nodeStorage->getOrCreateNode(
					destination.coord,
					destination.node->layer,
					destinationNode->actor->resourceActor);
				if (!questNode)
				{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
					logAi->trace(
						"AIMovementAfterDestinationRule::bypassQuest Failed to allocate node at %s[%d]. ",
						destination.coord.toString(),
						static_cast<int32_t>(destination.node->layer)
					);
#endif
					return false;
				}

				if(questNode.value()->getCost() < destination.cost)
				{
					return false;
				}

				destination.node = questNode.value();
				nodeStorage->commit(destination, source);
				AIPreviousNodeRule(nodeStorage).process(source, destination, pathfinderConfig, pathfinderHelper);
			}

			nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
			{
				node->addSpecialAction(std::make_shared<QuestAction>(questAction));
			});
		}

		return true;
	}

	bool AIMovementAfterDestinationRule::bypassRemovableObject(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		if(destination.nodeObject->ID == Obj::QUEST_GUARD
			|| destination.nodeObject->ID == Obj::BORDERGUARD
			|| destination.nodeObject->ID == Obj::BORDER_GATE)
		{
			return bypassQuest(source, destination, pathfinderConfig, pathfinderHelper);
		}

		auto enemyHero = destination.nodeHero && destination.heroRelations == PlayerRelations::ENEMIES;

		if(!enemyHero && !isObjectRemovable(destination.nodeObject))
		{
			if(nodeStorage->getHero(destination.node) == destination.nodeHero)
				return true;

			return false;
		}

		auto danger = aiNk->dangerEvaluator->evaluateDanger(destination.coord, nodeStorage->getHero(destination.node), true);

		if(danger)
		{
			return bypassBattle(source, destination, pathfinderConfig, pathfinderHelper);
		}

		return true;
	}

	bool AIMovementAfterDestinationRule::bypassDestinationGuards(
		std::vector<const CGObjectInstance *> destGuardians,
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		if(destGuardians.empty())
		{
			return false;
		}

		const auto srcGuardians = aiNk->cc->getGuardingCreatures(source.coord);
		const auto srcNode = nodeStorage->getAINode(source.node);

		vstd::erase_if(destGuardians, [&](const CGObjectInstance * destGuard) -> bool
		{
			return vstd::contains(srcGuardians, destGuard);
		});

		const auto guardsAlreadyBypassed = destGuardians.empty() && srcGuardians.size();
		if(guardsAlreadyBypassed && srcNode->actor->allowBattle)
		{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"Bypass guard at destination while moving %s -> %s",
				source.coord.toString(),
				destination.coord.toString());
#endif

			return true;
		}

		return bypassBattle(source, destination, pathfinderConfig, pathfinderHelper);
	}

	bool AIMovementAfterDestinationRule::bypassBattle(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		const AIPathNode *  srcNode = nodeStorage->getAINode(source.node);
		const AIPathNode * destNode = nodeStorage->getAINode(destination.node);
		const auto battleNodeOptional = nodeStorage->getOrCreateNode(
			destination.coord,
			destination.node->layer,
			destNode->actor->battleActor);

		if(!battleNodeOptional)
		{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"AIMovementAfterDestinationRule::bypassBattle Failed to allocate node at %s[%d]. "
				"Can not allocate battle node while moving %s -> %s",
				destination.coord.toString(),
				static_cast<int32_t>(destination.node->layer),
				source.coord.toString(),
				destination.coord.toString()
			);
#endif
			return false;
		}

		auto * battleNode = battleNodeOptional.value();

		if(battleNode->locked)
		{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"AIMovementAfterDestinationRule::bypassBattle Block bypass guard at destination while moving %s -> %s",
				source.coord.toString(),
				destination.coord.toString());
#endif
			return false;
		}

		auto hero = nodeStorage->getHero(source.node);
		uint64_t danger = aiNk->dangerEvaluator->evaluateDanger(destination.coord, hero, true);
		uint64_t actualArmyValue = srcNode->actor->armyValue - srcNode->armyLoss;
		uint64_t loss = nodeStorage->evaluateArmyLoss(hero, actualArmyValue, danger);

		if(loss < actualArmyValue)
		{
			if(destNode->specialAction)
			{
				battleNode->specialAction = destNode->specialAction;
			}

			destination.node = battleNode;
			nodeStorage->commit(destination, source);

			battleNode->armyLoss += loss;

			vstd::amax(battleNode->danger, danger);

			AIPreviousNodeRule(nodeStorage).process(source, destination, pathfinderConfig, pathfinderHelper);

			battleNode->addSpecialAction(std::make_shared<BattleAction>(destination.coord));

#if NK2AI_PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"AIMovementAfterDestinationRule::bypassBattle Begin bypass guard at destination with danger %s while moving %s -> %s",
				std::to_string(danger),
				source.coord.toString(),
				destination.coord.toString());
#endif
			return true;
		}

		return false;
	}
}

}
