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

#include "../../../lib/mapObjects/CommonConstructors.h"
#include "../Goals/Goals.h"
#include "Nullkiller.h"

namespace NKAI
{

ui64 FuzzyHelper::estimateBankDanger(const CBank * bank)
{
	//this one is not fuzzy anymore, just calculate weighted average

	auto objectInfo = VLC->objtypeh->getHandlerFor(bank->ID, bank->subID)->getObjectInfo(bank->appearance);

	auto * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());

	ui64 totalStrength = 0;
	ui8 totalChance = 0;
	for(auto config : bankInfo->getPossibleGuards())
	{
		totalStrength += config.second.totalStrength * config.first;
		totalChance += config.first;
	}
	return totalStrength / std::max<ui8>(totalChance, 1); //avoid division by zero

}

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
		vstd::erase_if(visitableObjects, [](const CGObjectInstance * obj)
		{
			return !objWithID<Obj::HERO>(obj);
		});
	}

	if(const CGObjectInstance * dangerousObject = vstd::backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject); //unguarded objects can also be dangerous or unhandled
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

	if(obj->tempOwner < PlayerColor::PLAYER_LIMIT && cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::TOWN:
	{
		const auto * cre = dynamic_cast<const CGTownInstance *>(obj);
		return cre->getUpperArmy()->getArmyStrength();
	}
	case Obj::ARTIFACT:
	case Obj::RESOURCE:
	{
		if(!vstd::contains(ai->memory->alreadyVisited, obj))
			return 0;
		FALLTHROUGH;
	}
	case Obj::MONSTER:
	case Obj::HERO:
	case Obj::GARRISON:
	case Obj::GARRISON2:
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
	{
		const auto * a = dynamic_cast<const CArmedInstance *>(obj);
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

}
