/*
* ExecuteHeroChain.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExecuteHeroChain.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"

namespace NK2AI
{

using namespace Goals;

ExecuteHeroChain::ExecuteHeroChain(const AIPath & path, const CGObjectInstance * obj)
	:ElementarGoal(Goals::EXECUTE_HERO_CHAIN), chainPath(path), closestWayRatio(1)
{
	hero = path.targetHero;
	tile = path.targetTile();
	closestWayRatio = 1;

	if(obj)
	{
		objid = obj->id.getNum();

#if NK2AI_TRACE_LEVEL >= 1
		targetName = obj->getObjectName() + tile.toString();
#else
		targetName = obj->getTypeName() + tile.toString();
#endif
	}
	else
	{
		targetName = "tile" + tile.toString();
	}
}

bool ExecuteHeroChain::operator==(const ExecuteHeroChain & other) const
{
	return tile == other.tile 
		&& chainPath.targetHero == other.chainPath.targetHero
		&& chainPath.nodes.size() == other.chainPath.nodes.size()
		&& chainPath.chainMask == other.chainPath.chainMask;
}

std::vector<ObjectInstanceID> ExecuteHeroChain::getAffectedObjects() const
{
	std::vector<ObjectInstanceID> affectedObjects = { chainPath.targetHero->id };

	if(objid != -1)
		affectedObjects.push_back(ObjectInstanceID(objid));

	for(auto & node : chainPath.nodes)
	{
		if(node.targetHero)
			affectedObjects.push_back(node.targetHero->id);
	}

	vstd::removeDuplicates(affectedObjects);

	return affectedObjects;
}

bool ExecuteHeroChain::isObjectAffected(ObjectInstanceID id) const
{
	if(chainPath.targetHero->id == id || objid == id.getNum())
		return true;

	for(auto & node : chainPath.nodes)
	{
		if(node.targetHero && node.targetHero->id == id)
			return true;
	}

	return false;
}

void ExecuteHeroChain::accept(AIGateway * aiGw)
{
	logAi->debug("Executing hero chain towards %s. Path %s", targetName, chainPath.toString());

	aiGw->nullkiller->setActive(chainPath.targetHero, tile);
	aiGw->nullkiller->setTargetObject(objid);
	aiGw->nullkiller->objectClusterizer->reset();

	auto targetObject = aiGw->cc->getObj(static_cast<ObjectInstanceID>(objid), false);

	if(chainPath.turn() == 0 && targetObject && targetObject->ID == Obj::TOWN)
	{
		auto relations = aiGw->cc->getPlayerRelations(aiGw->playerID, targetObject->getOwner());

		if(relations == PlayerRelations::ENEMIES)
		{
			aiGw->nullkiller->armyFormation->rearrangeArmyForSiege(
				dynamic_cast<const CGTownInstance *>(targetObject),
				chainPath.targetHero);
		}
	}

	std::set<int> blockedIndexes;

	for(int i = chainPath.nodes.size() - 1; i >= 0; i--)
	{
		auto  * node = &chainPath.nodes[i];

		const CGHeroInstance * hero = node->targetHero;
		HeroPtr heroPtr(hero, aiGw->cc.get());

		if(!heroPtr.isVerified())
		{
			logAi->error("Hero %s was lost. Exit hero chain.", heroPtr.nameOrDefault());

			return;
		}

		if(node->parentIndex >= i)
		{
			logAi->error("Invalid parentIndex while executing node " + node->coord.toString());
		}

		if(vstd::contains(blockedIndexes, i))
		{
			blockedIndexes.insert(node->parentIndex);
			aiGw->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);

			continue;
		}

		logAi->debug("Executing chain node %d. Moving hero %s to %s", i, hero->getNameTranslated(), node->coord.toString());

		try
		{
			if(hero->movementPointsRemaining() > 0)
			{
				aiGw->nullkiller->setActive(hero, node->coord);

				if(node->specialAction)
				{
					if(node->actionIsBlocked)
					{
						throw cannotFulfillGoalException("Path is nondeterministic.");
					}
					
					node->specialAction->execute(aiGw, hero);
					
					if(!heroPtr.isVerified())
					{
						logAi->error("Hero %s was lost trying to execute special action. Exit hero chain.", heroPtr.nameOrDefault());

						return;
					}
				}
				else if(i > 0 && aiGw->nullkiller->isObjectGraphAllowed())
				{
					auto chainMask = i < chainPath.nodes.size() - 1 ? chainPath.nodes[i + 1].chainMask : node->chainMask;

					for(auto j = i - 1; j >= 0; j--)
					{
						auto & nextNode = chainPath.nodes[j];

						if(nextNode.specialAction || nextNode.chainMask != chainMask)
							break;

						auto targetNode = aiGw->nullkiller->getPathsInfo(hero)->getPathInfo(nextNode.coord);

						if(!targetNode->reachable()
							|| targetNode->getCost() > nextNode.cost)
							break;

						i = j;
						node = &nextNode;

						if(targetNode->action == EPathNodeAction::BATTLE || targetNode->action == EPathNodeAction::TELEPORT_BATTLE)
							break;
					}
				}

				if(node->turns == 0 && node->coord != hero->visitablePos())
				{
					auto targetNode = aiGw->nullkiller->getPathsInfo(hero)->getPathInfo(node->coord);

					if(targetNode->accessible == EPathAccessibility::NOT_SET
						|| targetNode->accessible == EPathAccessibility::BLOCKED
						|| targetNode->accessible == EPathAccessibility::FLYABLE
						|| targetNode->turns != 0)
					{
						logAi->error(
							"Unable to complete chain. Expected hero %s to arrive to %s in 0 turns but he cannot do this",
							hero->getNameTranslated(),
							node->coord.toString());

						return;
					}
				}

				auto findWhirlpool = [&aiGw](const int3 & pos) -> ObjectInstanceID
				{
					auto objs = aiGw->cc->getVisitableObjs(pos);
					auto whirlpool = std::find_if(objs.begin(), objs.end(), [](const CGObjectInstance * o)->bool
						{
							return o->ID == Obj::WHIRLPOOL;
						});

					return whirlpool != objs.end() ? dynamic_cast<const CGWhirlpool *>(*whirlpool)->id : ObjectInstanceID(-1);
				};

				auto sourceWhirlpool = findWhirlpool(hero->visitablePos());
				auto targetWhirlpool = findWhirlpool(node->coord);
				
				if(i != chainPath.nodes.size() - 1 && sourceWhirlpool.hasValue() && sourceWhirlpool == targetWhirlpool)
				{
					logAi->trace("AI exited whirlpool at %s but expected at %s", hero->visitablePos().toString(), node->coord.toString());
					continue;
				}

				if(hero->movementPointsRemaining())
				{
					try
					{
						if(moveHeroToTile(aiGw, hero, node->coord))
						{
							continue;
						}
					}
					catch(const cannotFulfillGoalException &)
					{
						if(!heroPtr.isVerified())
						{
							logAi->error("Hero %s was lost. Exit hero chain.", heroPtr.nameOrDefault());

							return;
						}

						if(hero->movementPointsRemaining() > 0)
						{
							CGPath path;
							bool isOk = aiGw->nullkiller->getPathsInfo(hero)->getPath(path, node->coord);

							if(isOk && path.nodes.back().turns > 0)
							{
								logAi->warn("Hero %s has %d mp which is not enough to continue his way towards %s.", hero->getNameTranslated(), hero->movementPointsRemaining(), node->coord.toString());

								aiGw->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);
								return;
							}
						}

						throw;
					}
				}
			}

			if(node->coord == hero->visitablePos())
				continue;

			if(node->turns == 0)
			{
				logAi->error(
					"Unable to complete chain. Expected hero %s to arrive to %s but he is at %s",
					hero->getNameTranslated(),
					node->coord.toString(),
					hero->visitablePos().toString());

				return;
			}
			
			// no exception means we were not able to reach the tile
			aiGw->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);
			blockedIndexes.insert(node->parentIndex);
		}
		catch(const goalFulfilledException &)
		{
			if(!heroPtr.isVerified())
			{
				logAi->debug("Hero %s was killed while attempting to reach %s", heroPtr.nameOrDefault(), node->coord.toString());

				return;
			}
		}
	}
}

std::string ExecuteHeroChain::toString() const
{
#if NK2AI_TRACE_LEVEL >= 1
	return "ExecuteHeroChain " + targetName + " by " + chainPath.toString();
#else
	return "ExecuteHeroChain " + targetName + " by " + chainPath.targetHero->getNameTranslated();
#endif
}

bool ExecuteHeroChain::moveHeroToTile(AIGateway * aiGw, const CGHeroInstance * hero, const int3 & tile)
{
	if(tile == hero->visitablePos() && aiGw->cc->getVisitableObjs(hero->visitablePos()).size() < 2)
	{
		logAi->warn("Why do I want to move hero %s to tile %s? Already standing on that tile! ", hero->getNameTranslated(), tile.toString());

		return true;
	}

	return aiGw->moveHeroToTile(tile, HeroPtr(hero, aiGw->cc.get()));
}

}
