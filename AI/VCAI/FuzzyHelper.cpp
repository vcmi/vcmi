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

#include "Goals/Goals.h"
#include "VCAI.h"

#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/CBankInstanceConstructor.h"
#include "../../lib/mapObjects/CBank.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGDwelling.h"
#include "../../lib/gameState/InfoAboutArmy.h"

FuzzyHelper * fh;

Goals::TSubgoal FuzzyHelper::chooseSolution(Goals::TGoalVec vec)
{
	if(vec.empty())
	{
		logAi->debug("FuzzyHelper found no goals. Returning Goals::Invalid.");

		//no possibilities found
		return sptr(Goals::Invalid());
	}

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

	for(auto goal : vec)
	{
		logAi->trace("FuzzyHelper evaluated goal %s, priority=%.4f", goal->name(), goal->priority);
	}

	Goals::TSubgoal result = *boost::max_element(vec, compareGoals);

	logAi->debug("FuzzyHelper returned goal %s, priority=%.4f", result->name(), result->priority);

	return result;
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
	if(g.parent)
	{
		g.parent->accept(this);
	}

	return visitTileEngine.evaluate(g);
}

float FuzzyHelper::evaluate(Goals::BuildBoat & g)
{
	const float buildBoatPenalty = 0.25;

	if(!g.parent)
	{
		return 0;
	}

	return g.parent->accept(this) - buildBoatPenalty;
}

float FuzzyHelper::evaluate(Goals::AdventureSpellCast & g)
{
	if(!g.parent)
	{
		return 0;
	}

	const CSpell * spell = g.getSpell();
	const float spellCastPenalty = (float)g.hero->getSpellCost(spell) / g.hero->mana;

	return g.parent->accept(this) - spellCastPenalty;
}

float FuzzyHelper::evaluate(Goals::CompleteQuest & g)
{
	// TODO: How to evaluate quest complexity?
	const float questPenalty = 0.2f;

	if(!g.parent)
	{
		return 0;
	}

	return g.parent->accept(this) * questPenalty;
}

float FuzzyHelper::evaluate(Goals::VisitObj & g)
{
	if(g.parent)
	{
		g.parent->accept(this);
	}

	return visitObjEngine.evaluate(g);
}

float FuzzyHelper::evaluate(Goals::VisitHero & g)
{
	auto obj = ai->myCb->getObj(ObjectInstanceID(g.objid)); //we assume for now that these goals are similar
	if(!obj)
	{
		return -100; //hero died in the meantime
	}
	else
	{
		g.setpriority(Goals::VisitTile(obj->visitablePos()).sethero(g.hero).accept(this));
	}
	return g.priority;
}
float FuzzyHelper::evaluate(Goals::GatherArmy & g)
{
	//the more army we need, the more important goal
	//the more army we lack, the less important goal
	float army = static_cast<float>(g.hero->getArmyStrength());
	float ratio = g.value / std::max(g.value - army, 2000.0f); //2000 is about the value of hero recruited from tavern
	return 5 * (ratio / (ratio + 2)); //so 50% army gives 2.5, asymptotic 5
}

float FuzzyHelper::evaluate(Goals::ClearWayTo & g)
{
	if (!g.hero.h)
		return 0; //lowest priority

	return g.whatToDoToAchieve()->accept(this);
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

ui64 FuzzyHelper::evaluateDanger(crint3 tile, const CGHeroInstance * visitor)
{
	return evaluateDanger(tile, visitor, ai);
}

ui64 FuzzyHelper::evaluateDanger(crint3 tile, const CGHeroInstance * visitor, const VCAI * ai)
{
	auto cb = ai->myCb;
	const TerrainTile * t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0;
	ui64 guardDanger = 0;

	auto visitableObjects = cb->getVisitableObjs(tile);
	// in some scenarios hero happens to be "under" the object (eg town). Then we consider ONLY the hero.
	if(vstd::contains_if(visitableObjects, objWithID<Obj::HERO>))
	{
		vstd::erase_if(visitableObjects, [](const CGObjectInstance * obj)
		{
			return !objWithID<Obj::HERO>(obj);
		});
	}

	if(const CGObjectInstance * dangerousObject = vstd::backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject, ai); //unguarded objects can also be dangerous or unhandled
		if(objectDanger)
		{
			//TODO: don't downcast objects AI shouldn't know about!
			auto armedObj = dynamic_cast<const CArmedInstance *>(dangerousObject);
			if(armedObj)
			{
				float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(visitor, armedObj);
				objectDanger = static_cast<ui64>(objectDanger * tacticalAdvantage); //this line tends to go infinite for allied towns (?)
			}
		}
		if(dangerousObject->ID == Obj::SUBTERRANEAN_GATE)
		{
			//check guard on the other side of the gate
			auto it = ai->knownSubterraneanGates.find(dangerousObject);
			if(it != ai->knownSubterraneanGates.end())
			{
				auto guards = cb->getGuardingCreatures(it->second->visitablePos());
				for(auto cre : guards)
				{
					float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance *>(cre));

					vstd::amax(guardDanger, evaluateDanger(cre, ai) * tacticalAdvantage);
				}
			}
		}
	}

	auto guards = cb->getGuardingCreatures(tile);
	for(auto cre : guards)
	{
		float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance *>(cre));

		vstd::amax(guardDanger, evaluateDanger(cre, ai) * tacticalAdvantage); //we are interested in strongest monster around
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 FuzzyHelper::evaluateDanger(const CGObjectInstance * obj, const VCAI * ai)
{
	auto cb = ai->myCb;

	if(obj->tempOwner < PlayerColor::PLAYER_LIMIT && cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::HERO:
	{
		InfoAboutHero iah;
		cb->getHeroInfo(obj, iah);
		return iah.army.getStrength();
	}
	case Obj::TOWN:
	case Obj::GARRISON:
	case Obj::GARRISON2:
	{
		InfoAboutTown iat;
		cb->getTownInfo(obj, iat);
		return iat.army.getStrength();
	}
	case Obj::MONSTER:
	{
		//TODO!!!!!!!!
		const CGCreature * cre = dynamic_cast<const CGCreature *>(obj);
		return cre->getArmyStrength();
	}
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
	{
		const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
		return d->getArmyStrength();
	}
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
	{
		const CArmedInstance * a = dynamic_cast<const CArmedInstance *>(obj);
		return a->getArmyStrength();
	}
	case Obj::CRYPT: //crypt
	case Obj::CREATURE_BANK: //crebank
	case Obj::DRAGON_UTOPIA:
	case Obj::SHIPWRECK: //shipwreck
	case Obj::DERELICT_SHIP: //derelict ship
							 //	case Obj::PYRAMID:
		return estimateBankDanger(dynamic_cast<const CBank *>(obj));
	case Obj::PYRAMID:
	{
		if(obj->subID == 0)
			return estimateBankDanger(dynamic_cast<const CBank *>(obj));
		else
			return 0;
	}
	default:
		return 0;
	}
}
