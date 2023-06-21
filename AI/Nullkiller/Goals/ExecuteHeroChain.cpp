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

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

ExecuteHeroChain::ExecuteHeroChain(const AIPath & path, const CGObjectInstance * obj)
	:ElementarGoal(Goals::EXECUTE_HERO_CHAIN), chainPath(path), closestWayRatio(1)
{
	hero = path.targetHero;
	tile = path.targetTile();

	if(obj)
	{
		objid = obj->id.getNum();
		targetName = obj->getObjectName() + tile.toString();
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

void ExecuteHeroChain::accept(AIGateway * ai)
{
	logAi->debug("Executing hero chain towards %s. Path %s", targetName, chainPath.toString());

	ai->nullkiller->setActive(chainPath.targetHero, tile);
	ai->nullkiller->setTargetObject(objid);

	std::set<int> blockedIndexes;

	for(int i = chainPath.nodes.size() - 1; i >= 0; i--)
	{
		auto & node = chainPath.nodes[i];

		const CGHeroInstance * hero = node.targetHero;
		HeroPtr heroPtr = hero;

		if(node.parentIndex >= i)
		{
			logAi->error("Invalid parentIndex while executing node " + node.coord.toString());
		}

		if(vstd::contains(blockedIndexes, i))
		{
			blockedIndexes.insert(node.parentIndex);
			ai->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);

			continue;
		}

		logAi->debug("Executing chain node %d. Moving hero %s to %s", i, hero->getNameTranslated(), node.coord.toString());

		try
		{
			if(hero->movement)
			{
				ai->nullkiller->setActive(hero, node.coord);

				if(node.specialAction)
				{
					if(node.actionIsBlocked)
					{
						throw cannotFulfillGoalException("Path is nondeterministic.");
					}
					
					node.specialAction->execute(hero);
					
					if(!heroPtr.validAndSet())
					{
						logAi->error("Hero %s was lost trying to execute special action. Exit hero chain.", heroPtr.name);

						return;
					}
				}

				if(node.turns == 0 && node.coord != hero->visitablePos())
				{
					auto targetNode = cb->getPathsInfo(hero)->getPathInfo(node.coord);

					if(targetNode->accessible == EPathAccessibility::NOT_SET
						|| targetNode->accessible == EPathAccessibility::BLOCKED
						|| targetNode->accessible == EPathAccessibility::FLYABLE
						|| targetNode->turns != 0)
					{
						logAi->error(
							"Unable to complete chain. Expected hero %s to arrive to %s in 0 turns but he cannot do this",
							hero->getNameTranslated(),
							node.coord.toString());

						return;
					}
				}

				if(hero->movement)
				{
					try
					{
						if(moveHeroToTile(hero, node.coord))
						{
							continue;
						}
					}
					catch(const cannotFulfillGoalException &)
					{
						if(!heroPtr.validAndSet())
						{
							logAi->error("Hero %s was lost. Exit hero chain.", heroPtr.name);

							return;
						}

						if(hero->movement > 0)
						{
							CGPath path;
							bool isOk = cb->getPathsInfo(hero)->getPath(path, node.coord);

							if(isOk && path.nodes.back().turns > 0)
							{
								logAi->warn("Hero %s has %d mp which is not enough to continue his way towards %s.", hero->getNameTranslated(), hero->movement, node.coord.toString());

								ai->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);
								return;
							}
						}

						throw;
					}
				}
			}

			if(node.coord == hero->visitablePos())
				continue;

			if(node.turns == 0)
			{
				logAi->error(
					"Unable to complete chain. Expected hero %s to arive to %s but he is at %s",
					hero->getNameTranslated(),
					node.coord.toString(),
					hero->visitablePos().toString());

				return;
			}
			
			// no exception means we were not able to reach the tile
			ai->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);
			blockedIndexes.insert(node.parentIndex);
		}
		catch(const goalFulfilledException &)
		{
			if(!heroPtr.validAndSet())
			{
				logAi->debug("Hero %s was killed while attempting to reach %s", heroPtr.name, node.coord.toString());

				return;
			}
		}
	}
}

std::string ExecuteHeroChain::toString() const
{
	return "ExecuteHeroChain " + targetName + " by " + chainPath.targetHero->getNameTranslated();
}

bool ExecuteHeroChain::moveHeroToTile(const CGHeroInstance * hero, const int3 & tile)
{
	if(tile == hero->visitablePos() && cb->getVisitableObjs(hero->visitablePos()).size() < 2)
	{
		logAi->warn("Why do I want to move hero %s to tile %s? Already standing on that tile! ", hero->getNameTranslated(), tile.toString());

		return true;
	}

	return ai->moveHeroToTile(tile, hero);
}

}
