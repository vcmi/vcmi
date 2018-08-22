/*
 * FuzzyHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#include "StdInc.h"
#include "FuzzyHelper.h"

#include "../../lib/mapObjects/CommonConstructors.h"
#include "VCAI.h"

FuzzyHelper * fh;

extern boost::thread_specific_ptr<VCAI> ai;

Goals::TSubgoal FuzzyHelper::chooseSolution(Goals::TGoalVec vec)
{
	if(vec.empty()) //no possibilities found
		return sptr(Goals::Invalid());

	ai->cachedSectorMaps.clear();

	//a trick to switch between heroes less often - calculatePaths is costly
	auto sortByHeroes = [](const Goals::TSubgoal & lhs, const Goals::TSubgoal & rhs) -> bool
	{
		return lhs->hero.h < rhs->hero.h;
	};
	boost::sort(vec, sortByHeroes);

	for(auto g : vec)
	{
		setPriority(g);
	}

	auto compareGoals = [](const Goals::TSubgoal & lhs, const Goals::TSubgoal & rhs) -> bool
	{
		return lhs->priority < rhs->priority;
	};
	return *boost::max_element(vec, compareGoals);
}

ui64 FuzzyHelper::estimateBankDanger(const CBank * bank)
{
	//this one is not fuzzy anymore, just calculate weighted average

	auto objectInfo = VLC->objtypeh->getHandlerFor(bank->ID, bank->subID)->getObjectInfo(bank->appearance);

	CBankInfo * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());

	ui64 totalStrength = 0;
	ui8 totalChance = 0;
	for(auto config : bankInfo->getPossibleGuards())
	{
		totalStrength += config.second.totalStrength * config.first;
		totalChance += config.first;
	}
	return totalStrength / std::max<ui8>(totalChance, 1); //avoid division by zero

}

float FuzzyHelper::evaluate(Goals::VisitTile & g)
{
	return visitTileEngine.evaluate(g);
}
float FuzzyHelper::evaluate(Goals::VisitObj & g)
{
	return visitObjEngine.evaluate(g);
}
float FuzzyHelper::evaluate(Goals::VisitHero & g)
{
	auto obj = ai->myCb->getObj(ObjectInstanceID(g.objid)); //we assume for now that these goals are similar
	if(!obj)
		return -100; //hero died in the meantime
	else
	{
		auto dummyGoal = Goals::VisitTile(obj->visitablePos()).sethero(g.hero).setisAbstract(g.isAbstract);
		g.setpriority(dummyGoal.accept(this));
	}
	return g.priority;
}
float FuzzyHelper::evaluate(Goals::GatherArmy & g)
{
	//the more army we need, the more important goal
	//the more army we lack, the less important goal
	float army = g.hero->getArmyStrength();
	float ratio = g.value / std::max(g.value - army, 2000.0f); //2000 is about the value of hero recruited from tavern
	return 5 * (ratio / (ratio + 2)); //so 50% army gives 2.5, asymptotic 5
}

float FuzzyHelper::evaluate(Goals::ClearWayTo & g)
{
	if (!g.hero.h)
		return 0; //lowest priority

	int3 t = ai->getCachedSectorMap(g.hero)->firstTileToGet(g.hero, g.tile);

	if(t.valid())
	{
		if(isSafeToVisit(g.hero, t))
		{
			g.setpriority(Goals::VisitTile(g.tile).sethero(g.hero).setisAbstract(g.isAbstract).accept(this));
		}
		else
		{
			g.setpriority (Goals::GatherArmy(evaluateDanger(t, g.hero.h)*SAFE_ATTACK_CONSTANT).
				sethero(g.hero).setisAbstract(true).accept(this));
		}
		return g.priority;
	}
	else
		return -1;

}

float FuzzyHelper::evaluate(Goals::BuildThis & g)
{
	return g.priority; //TODO
}
float FuzzyHelper::evaluate(Goals::DigAtTile & g)
{
	return 0;
}
float FuzzyHelper::evaluate(Goals::CollectRes & g)
{
	return g.priority; //handled by ResourceManager
}
float FuzzyHelper::evaluate(Goals::Build & g)
{
	return 0;
}
float FuzzyHelper::evaluate(Goals::BuyArmy & g)
{
	return g.priority;
}
float FuzzyHelper::evaluate(Goals::Explore & g)
{
	return 1;
}
float FuzzyHelper::evaluate(Goals::RecruitHero & g)
{
	return 1;
}
float FuzzyHelper::evaluate(Goals::Invalid & g)
{
	return -1e10;
}
float FuzzyHelper::evaluate(Goals::AbstractGoal & g)
{
	logAi->warn("Cannot evaluate goal %s", g.name());
	return g.priority;
}
void FuzzyHelper::setPriority(Goals::TSubgoal & g) //calls evaluate - Visitor pattern
{
	g->setpriority(g->accept(this)); //this enforces returned value is set
}
