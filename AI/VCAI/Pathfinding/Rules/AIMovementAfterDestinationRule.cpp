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
			auto enemyHero = objID == Obj::HERO && destination.objectRelations == PlayerRelations::ENEMIES;

			if(!enemyHero && !isObjectRemovable(destination.nodeObject))
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
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace(
					"Bypass guard at destination while moving %s -> %s",
					source.coord.toString(),
					destination.coord.toString());
#endif

				return;
			}

			const AIPathNode * destNode = nodeStorage->getAINode(destination.node);
			auto battleNodeOptional = nodeStorage->getOrCreateNode(
				destination.coord,
				destination.node->layer,
				destNode->chainMask | AINodeStorage::BATTLE_CHAIN);

			if(!battleNodeOptional)
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace(
					"Can not allocate battle node while moving %s -> %s",
					source.coord.toString(),
					destination.coord.toString());
#endif

				destination.blocked = true;

				return;
			}

			auto * battleNode = battleNodeOptional.value();

			if(battleNode->locked)
			{
#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace(
					"Block bypass guard at destination while moving %s -> %s",
					source.coord.toString(),
					destination.coord.toString());
#endif
				destination.blocked = true;

				return;
			}

			auto danger = nodeStorage->evaluateDanger(destination.coord);

			destination.node = battleNode;
			nodeStorage->commit(destination, source);

			if(battleNode->danger < danger)
			{
				battleNode->danger = danger;
			}

			battleNode->specialAction = std::make_shared<BattleAction>(destination.coord);
#ifdef VCMI_TRACE_PATHFINDER
			logAi->trace(
				"Begin bypass guard at destination with danger %s while moving %s -> %s",
				std::to_string(danger),
				source.coord.toString(),
				destination.coord.toString());
#endif
			return;
		}

		destination.blocked = true;
	}
}
