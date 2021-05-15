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
#include "VisitTile.h"
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"
#include "../Engine/Nullkiller.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

ExecuteHeroChain::ExecuteHeroChain(const AIPath & path, const CGObjectInstance * obj)
	:CGoal(Goals::EXECUTE_HERO_CHAIN), chainPath(path)
{
	if(obj)
		objid = obj->id.getNum();

	evaluationContext.danger = path.getTotalDanger(hero);
	evaluationContext.movementCost = path.movementCost();
	evaluationContext.armyLoss = path.armyLoss;
	evaluationContext.heroStrength = path.getHeroStrength();
	hero = path.targetHero;
	tile = path.firstTileToGet();
}

bool ExecuteHeroChain::operator==(const ExecuteHeroChain & other) const
{
	return false;
}

TSubgoal ExecuteHeroChain::whatToDoToAchieve()
{
	return iAmElementar();
}

void ExecuteHeroChain::accept(VCAI * ai)
{
	logAi->debug("Executing hero chain towards %s", tile.toString());

#ifdef VCMI_TRACE_PATHFINDER
	std::stringstream str;

	str << "Path ";

	for(auto node : chainPath.nodes)
		str << node.targetHero->name << "->" << node.coord.toString() << "; ";

	logAi->trace(str.str());
#endif

	std::set<int> blockedIndexes;

	for(int i = chainPath.nodes.size() - 1; i >= 0; i--)
	{
		auto & node = chainPath.nodes[i];

		HeroPtr hero = node.targetHero;
		auto vt = Goals::sptr(Goals::VisitTile(node.coord).sethero(hero));

		if(vstd::contains(blockedIndexes, i))
		{
			blockedIndexes.insert(node.parentIndex);
			ai->setGoal(hero, vt);

			continue;
		}

		logAi->debug("Moving hero %s to %s", hero.name, node.coord.toString());

		try
		{
			ai->nullkiller->setActive(hero);
			vt->accept(ai);

			blockedIndexes.insert(node.parentIndex);
		}
		catch(goalFulfilledException)
		{
			if(!hero)
			{
				logAi->debug("Hero %s was killed while attempting to rich %s", hero.name, node.coord.toString());

				return;
			}
		}
	}
}

std::string ExecuteHeroChain::name() const
{
	return "ExecuteHeroChain";
}

std::string ExecuteHeroChain::completeMessage() const
{
	return "Hero chain completed";
}
