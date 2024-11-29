/*
 * FuzzyHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#include "../StdInc.h"
#include "FuzzyHelper.h"

#include "../Goals/Goals.h"
#include "Nullkiller.h"

#include "../../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjectConstructors/CBankInstanceConstructor.h"

namespace NKAI
{

ui64 FuzzyHelper::evaluateDanger(const int3 & tile, const CGHeroInstance * visitor, bool checkGuards)
{
	auto cb = ai->cb.get();
	const TerrainTile * t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0;
	ui64 guardDanger = 0;

	auto visitableObjects = cb->getVisitableObjs(tile);
	// in some scenarios hero happens to be "under" the object (eg town). Then we consider ONLY the hero.
	if(vstd::contains_if(visitableObjects, objWithID<Obj::HERO>))
	{
		vstd::erase_if(visitableObjects, [](const CGObjectInstance * obj) -> bool
		{
				return !objWithID<Obj::HERO>(obj);
		});
	}

	if(const CGObjectInstance * dangerousObject = vstd::backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject); //unguarded objects can also be dangerous or unhandled

		if(objWithID<Obj::HERO>(dangerousObject))
		{
			auto hero = dynamic_cast<const CGHeroInstance *>(dangerousObject);

			if(hero->visitedTown && !hero->visitedTown->garrisonHero)
			{
				objectDanger += evaluateDanger(hero->visitedTown.get());
			}
			objectDanger *= ai->heroManager->getFightingStrengthCached(hero);
		}
		if (objWithID<Obj::TOWN>(dangerousObject))
		{
			auto town = dynamic_cast<const CGTownInstance*>(dangerousObject);
			auto hero = town->garrisonHero;

			if (hero)
				objectDanger *= ai->heroManager->getFightingStrengthCached(hero);
		}

		if(objectDanger)
		{
			//TODO: don't downcast objects AI shouldn't know about!
			auto armedObj = dynamic_cast<const CArmedInstance *>(dangerousObject);
			if(armedObj)
			{
				float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(visitor, armedObj);
				objectDanger *= tacticalAdvantage; //this line tends to go infinite for allied towns (?)
			}
		}
		if(dangerousObject->ID == Obj::SUBTERRANEAN_GATE)
		{
			//check guard on the other side of the gate
			auto it = ai->memory->knownSubterraneanGates.find(dangerousObject);
			if(it != ai->memory->knownSubterraneanGates.end())
			{
				auto guards = cb->getGuardingCreatures(it->second->visitablePos());

				for(auto cre : guards)
				{
					float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(
						visitor, 
						dynamic_cast<const CArmedInstance *>(cre));

					vstd::amax(guardDanger, evaluateDanger(cre) * tacticalAdvantage);
				}
			}
		}
	}

	if(checkGuards)
	{
		auto guards = cb->getGuardingCreatures(tile);
		for(auto cre : guards)
		{
			float tacticalAdvantage = tacticalAdvantageEngine.getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance *>(cre));

			vstd::amax(guardDanger, evaluateDanger(cre) * tacticalAdvantage); //we are interested in strongest monster around
		}
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 FuzzyHelper::evaluateDanger(const CGObjectInstance * obj)
{
	auto cb = ai->cb.get();

	if(obj->tempOwner.isValidPlayer() && cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::TOWN:
	{
		const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(obj);
		auto danger = town->getUpperArmy()->getArmyStrength();

		if(danger || town->visitingHero)
		{
			auto fortLevel = town->fortLevel();

			if (fortLevel == CGTownInstance::EFortLevel::CASTLE)
				danger = std::max(danger * 2, danger + 10000);
			else if(fortLevel == CGTownInstance::EFortLevel::CITADEL)
				danger = std::max(ui64(danger * 1.4), danger + 4000);
		}

		return danger;
	}

	case Obj::HERO:
	{
		const CGHeroInstance * hero = dynamic_cast<const CGHeroInstance *>(obj);
		return getHeroArmyStrengthWithCommander(hero, hero);
	}

	case Obj::ARTIFACT:
	case Obj::RESOURCE:
	{
		if(!vstd::contains(ai->memory->alreadyVisited, obj))
			return 0;
		[[fallthrough]];
	}
	default:
	{
		const CArmedInstance * a = dynamic_cast<const CArmedInstance *>(obj);
		if (a)
			return a->getArmyStrength();
		else
			return 0;
	}
	}
}
}
