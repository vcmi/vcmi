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
#include "../../Goals/Invalid.h"
#include "AIPreviousNodeRule.h"

namespace AIPathfinding
{
	AIMovementAfterDestinationRule::AIMovementAfterDestinationRule(
		CPlayerSpecificInfoCallback * cb, 
		std::shared_ptr<AINodeStorage> nodeStorage)
		:cb(cb), nodeStorage(nodeStorage)
	{
	}

	void AIMovementAfterDestinationRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		if(nodeStorage->isMovementIneficient(source, destination))
		{
			destination.node->locked = true;
			destination.blocked = true;

			return;
		}

		auto blocker = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);
		if(blocker == BlockingReason::NONE)
		{
			destination.blocked = nodeStorage->isDistanceLimitReached(source, destination);

			return;
		}

#if PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Movement from tile %s is blocked. Try to bypass. Action: %d, blocker: %d",
			destination.coord.toString(),
			(int)destination.action,
			(int)blocker);
#endif

		auto destGuardians = cb->getGuardingCreatures(destination.coord);
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
		auto questInfo = QuestInfo(questObj->quest, destination.nodeObject, destination.coord);
		QuestAction questAction(questInfo);

		if(destination.nodeObject->ID == Obj::QUEST_GUARD && questObj->quest->missionType == CQuest::MISSION_NONE)
		{
			return false;
		}

		if(!questAction.canAct(destinationNode))
		{
			if(!destinationNode->actor->allowUseResources)
			{
				boost::optional<AIPathNode *> questNode = nodeStorage->getOrCreateNode(
					destination.coord,
					destination.node->layer,
					destinationNode->actor->resourceActor);

				if(!questNode || questNode.get()->getCost() < destination.cost)
				{
					return false;
				}

				destination.node = questNode.get();

				nodeStorage->commit(destination, source);
				AIPreviousNodeRule(nodeStorage).process(source, destination, pathfinderConfig, pathfinderHelper);
			}

			nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
			{
				node->specialAction.reset(new QuestAction(questAction));
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

		auto danger = nodeStorage->evaluateDanger(destination.coord, nodeStorage->getHero(destination.node), true);

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
		auto srcGuardians = cb->getGuardingCreatures(source.coord);

		if(destGuardians.empty())
		{
			return false;
		}

		auto srcNode = nodeStorage->getAINode(source.node);

		vstd::erase_if(destGuardians, [&](const CGObjectInstance * destGuard) -> bool
		{
			return vstd::contains(srcGuardians, destGuard);
		});

		auto guardsAlreadyBypassed = destGuardians.empty() && srcGuardians.size();

		if(guardsAlreadyBypassed && srcNode->actor->allowBattle)
		{
#if PATHFINDER_TRACE_LEVEL >= 1
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
		auto battleNodeOptional = nodeStorage->getOrCreateNode(
			destination.coord,
			destination.node->layer,
			destNode->actor->battleActor);

		if(!battleNodeOptional)
		{
#if PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"Can not allocate battle node while moving %s -> %s",
				source.coord.toString(),
				destination.coord.toString());
#endif
			return false;
		}

		AIPathNode * battleNode = battleNodeOptional.get();

		if(battleNode->locked)
		{
#if PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"Block bypass guard at destination while moving %s -> %s",
				source.coord.toString(),
				destination.coord.toString());
#endif
			return false;
		}

		auto hero = nodeStorage->getHero(source.node);
		uint64_t danger = nodeStorage->evaluateDanger(destination.coord, hero, true);
		uint64_t actualArmyValue = srcNode->actor->armyValue - srcNode->armyLoss;
		uint64_t loss = nodeStorage->evaluateArmyLoss(hero, actualArmyValue, danger);

		if(loss < actualArmyValue)
		{
			destination.node = battleNode;
			nodeStorage->commit(destination, source);

			battleNode->armyLoss += loss;

			vstd::amax(battleNode->danger, danger);

			AIPreviousNodeRule(nodeStorage).process(source, destination, pathfinderConfig, pathfinderHelper);

			battleNode->specialAction = std::make_shared<BattleAction>(destination.coord);

#if PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"Begin bypass guard at destination with danger %s while moving %s -> %s",
				std::to_string(danger),
				source.coord.toString(),
				destination.coord.toString());
#endif
			return true;
		}

		return false;
	}
}
