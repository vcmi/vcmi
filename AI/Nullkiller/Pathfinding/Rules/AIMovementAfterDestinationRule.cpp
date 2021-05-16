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
#include "../../Goals/Invalid.h"

namespace AIPathfinding
{
	class QuestAction : public ISpecialAction
	{
	public:
		QuestAction(QuestInfo questInfo)
		{
		}

		virtual bool canAct(const CGHeroInstance * hero) const override
		{
			return false;
		}

		virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const override
		{
			return Goals::sptr(Goals::Invalid());
		}
	};

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
			auto enemyHero = destination.nodeHero && destination.heroRelations == PlayerRelations::ENEMIES;

			if(!enemyHero && !isObjectRemovable(destination.nodeObject))
			{
				if(nodeStorage->getHero(destination.node) == destination.nodeHero)
					return;

				destination.blocked = true;
			}

			if(destination.nodeObject->ID == Obj::QUEST_GUARD || destination.nodeObject->ID == Obj::BORDERGUARD)
			{
				auto questObj = dynamic_cast<const IQuestObject *>(destination.nodeObject);
				auto nodeHero = pathfinderHelper->hero;
				
				if(!destination.nodeObject->wasVisited(nodeHero->tempOwner)
					|| !questObj->checkQuest(nodeHero))
				{
					nodeStorage->updateAINode(destination.node, [&](AIPathNode * node)
					{
						auto questInfo = QuestInfo(questObj->quest, destination.nodeObject, destination.coord);

						node->specialAction.reset(new QuestAction(questInfo));
					});
				}
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
			auto srcNode = nodeStorage->getAINode(source.node);
			if(guardsAlreadyBypassed && srcNode->actor->allowBattle)
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
				destNode->actor->battleActor);

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

			AIPathNode * battleNode = battleNodeOptional.get();

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

			auto hero = nodeStorage->getHero(source.node);
			auto danger = nodeStorage->evaluateDanger(destination.coord, hero);
			double actualArmyValue = srcNode->actor->armyValue - srcNode->armyLoss;
			double ratio = (double)danger / (actualArmyValue * hero->getFightingStrength());

			uint64_t loss = (uint64_t)(actualArmyValue * ratio * ratio * ratio);

			if(loss < actualArmyValue)
			{
				destination.node = battleNode;
				nodeStorage->commit(destination, source);

				battleNode->armyLoss += loss;

				vstd::amax(battleNode->danger, danger);

				battleNode->specialAction = std::make_shared<BattleAction>(destination.coord);

				if(source.nodeObject && isObjectRemovable(source.nodeObject))
				{
					battleNode->theNodeBefore = source.node;
				}

#ifdef VCMI_TRACE_PATHFINDER
				logAi->trace(
					"Begin bypass guard at destination with danger %s while moving %s -> %s",
					std::to_string(danger),
					source.coord.toString(),
					destination.coord.toString());
#endif
				return;
			}
		}

		destination.blocked = true;
	}
}
