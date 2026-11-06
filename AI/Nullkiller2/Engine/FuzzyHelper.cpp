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
#include "../../../lib/IGameSettings.h"
#include "../../../lib/mapObjects/CRewardableObject.h"
#include "../../../lib/rewardable/Info.h"
#include "../../../lib/StartInfo.h"

namespace NK2AI
{

static constexpr ui64 UNKNOWN_DANGER = 190000000;

static ui64 evaluateRewardableDanger(const CGObjectInstance * obj, const RewardableObjectInfo & rewardableInfo)
{
	const bool guardsVisible = obj->cb->getSettings().getBoolean(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION);
	const auto * rewardableObject = dynamic_cast<const CRewardableObject *>(obj);
	// Some banks trigger combat immediately on visit.
	const bool forceCombat = rewardableObject && rewardableObject->configuration.forceCombat;
	const bool knowsRange = !guardsVisible || forceCombat || (!rewardableInfo.scouted &&
		static_cast<int>(obj->cb->getStartInfo()->getDifficulty()) >= static_cast<int>(EMapDifficulty::EXPERT));
	if((!rewardableInfo.scouted || rewardableInfo.guarded) && knowsRange)
	{
		auto objectInfo = obj->getObjectHandler()->getObjectInfo();
		const auto * rewardable = dynamic_cast<const Rewardable::Info *>(objectInfo.get());
		if(rewardable && rewardable->hasGuards())
		{
			if(const auto guardRange = rewardable->getCreatureBankGuardRange())
			{
				if(!guardsVisible || forceCombat)
				{
					// banks with hidden guards and modded banks that force combat need maximal danger estimation
					return guardRange->second;
				}
				// standard banks use minimum danger estimation so a hero with just-sufficient army can visit the bank and clear it if it is minimum tier
				return guardRange->first;
			}
			return UNKNOWN_DANGER;
		}
	}
	return rewardableInfo.guardStrength;
}

ui64 FuzzyHelper::evaluateDanger(const int3 & tile, const CGHeroInstance * visitor, bool checkGuards)
{
	auto cb = aiNk->cc.get();
	const TerrainTile * t = cb->tryGetTile(tile);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return UNKNOWN_DANGER;

	ui64 objectDanger = 0;
	ui64 guardDanger = 0;

	auto visitableObjects = cb->getVisitableObjs(tile);
	if(visitor)
		vstd::erase_if(visitableObjects, [visitor](const CGObjectInstance * obj) { return obj == visitor; });

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

			if(hero->getVisitedTown() && !hero->getVisitedTown()->getGarrisonHero())
			{
				objectDanger += evaluateDanger(hero->getVisitedTown());
			}
			objectDanger *= aiNk->heroManager->getFightingStrengthCached(hero);
		}
		if (objWithID<Obj::TOWN>(dangerousObject))
		{
			auto town = dynamic_cast<const CGTownInstance*>(dangerousObject);
			auto hero = town->getGarrisonHero();

			if (hero)
				objectDanger *= aiNk->heroManager->getFightingStrengthCached(hero);
		}

		if(dangerousObject->ID == Obj::SUBTERRANEAN_GATE)
		{
			//check guard on the other side of the gate
			auto it = aiNk->memory->knownSubterraneanGates.find(dangerousObject);
			// exit may be hidden again after it was seen, e.g. by Cover of Darkness
			if(it != aiNk->memory->knownSubterraneanGates.end() && cb->isVisible(it->second->visitablePos()))
			{
				auto guards = cb->getGuardingCreatures(it->second->visitablePos());

				for(auto cre : guards)
					vstd::amax(guardDanger, evaluateDanger(cre));
			}
		}
	}

	if(checkGuards)
	{
		auto guards = cb->getGuardingCreatures(tile);
		for(auto cre : guards)
			vstd::amax(guardDanger, evaluateDanger(cre)); //we are interested in strongest monster around
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 FuzzyHelper::evaluateDanger(const CGObjectInstance * obj)
{
	auto cb = aiNk->cc.get();

	if(obj->tempOwner.isValidPlayer() && cb->getPlayerRelations(obj->tempOwner, aiNk->playerID) != PlayerRelations::ENEMIES) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::TOWN:
	{
		const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(obj);
		auto danger = town->getUpperArmy()->estimateCombatValue();

		if(danger || town->getVisitingHero())
		{
			auto fortLevel = town->fortLevel();

			if (fortLevel == CGTownInstance::EFortLevel::CASTLE)
				danger += 10000;
			else if(fortLevel == CGTownInstance::EFortLevel::CITADEL)
				danger += 4000;
		}

		return danger;
	}

	case Obj::HERO:
	{
		const CGHeroInstance * hero = dynamic_cast<const CGHeroInstance *>(obj);
		return getHeroArmyStrengthWithCommander(hero, hero);
	}

	case Obj::PANDORAS_BOX:
		return dynamic_cast<const CArmedInstance *>(obj)->estimateCombatValue();

	case Obj::ARTIFACT:
	case Obj::RESOURCE:
	{
		if(!vstd::contains(aiNk->memory->alreadyVisited, obj->id))
			return 0;
		[[fallthrough]];
	}
	default:
	{
		if(const auto info = aiNk->cc->getRewardableObjectInfo(obj))
			return evaluateRewardableDanger(obj, *info);

		const CArmedInstance * a = dynamic_cast<const CArmedInstance *>(obj);
		if (a)
			return a->estimateCombatValue();
		else
			return 0;
	}
	}
}
}
