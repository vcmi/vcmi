/*
* CompleteQuest.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "CompleteQuest.h"

#include "CollectRes.h"
#include "FindObj.h"
#include "GatherArmy.h"
#include "GatherTroops.h"
#include "GetArtOfType.h"
#include "RecruitHero.h"

#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapObjects/CQuest.h"

using namespace Goals;

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	return q.getQuest(cb) == other.q.getQuest(cb);
}

bool isKeyMaster(const QuestInfo & q)
{
	auto object = q.getObject(cb);
	return object && (object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD);
}

TGoalVec CompleteQuest::getAllPossibleSubgoals()
{
	TGoalVec solutions;
	auto quest = q.getQuest(cb);

	if(!quest->isCompleted)
	{
		logAi->debug("Trying to realize quest: %s", questToString());
		
		if(isKeyMaster(q))
			return missionKeymaster();

		if(!quest->mission.artifacts.empty())
			return missionArt();

		if(!quest->mission.heroes.empty())
			return missionHero();

		if(!quest->mission.creatures.empty())
			return missionArmy();

		if(quest->mission.resources.nonZero())
			return missionResources();

		if(quest->killTarget != ObjectInstanceID::NONE)
			return missionDestroyObj();

		for(auto & s : quest->mission.primary)
			if(s)
				return missionIncreasePrimaryStat();

		if(quest->mission.heroLevel > 0)
			return missionLevel();
	}

	return TGoalVec();
}

TSubgoal CompleteQuest::whatToDoToAchieve()
{
	if(q.getQuest(cb)->mission == Rewardable::Limiter{})
	{
		throw cannotFulfillGoalException("Can not complete inactive quest");
	}

	TGoalVec solutions = getAllPossibleSubgoals();

	if(solutions.empty())
		throw cannotFulfillGoalException("Can not complete quest " + questToString());

	TSubgoal result = fh->chooseSolution(solutions);

	logAi->trace(
		"Returning %s, tile: %s, objid: %d, hero: %s",
		result->name(),
		result->tile.toString(),
		result->objid,
		result->hero.validAndSet() ? result->hero->getNameTranslated() : "not specified");

	return result;
}

std::string CompleteQuest::name() const
{
	return "CompleteQuest";
}

std::string CompleteQuest::completeMessage() const
{
	return "Completed quest " + questToString();
}

std::string CompleteQuest::questToString() const
{
	auto quest = q.getQuest(cb);

	if(quest->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	quest->getRolloverText(cb, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest() const
{
	TGoalVec solutions;

	auto heroes = cb->getHeroesInfo(); //TODO: choose best / free hero from among many possibilities?

	for(auto hero : heroes)
	{
		if(q.getQuest(cb)->checkQuest(hero))
		{
			vstd::concatenate(solutions, ai->ah->howToVisitObj(hero, ObjectIdRef(q.getObject(cb)->id)));
		}
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArt() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(!solutions.empty())
		return solutions;

	for(auto art : q.getQuest(cb)->mission.artifacts)
	{
		solutions.push_back(sptr(GetArtOfType(art.getNum()))); //TODO: transport?
	}

	return solutions;
}

TGoalVec CompleteQuest::missionHero() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		//rule of a thumb - quest heroes usually are locked in prisons
		solutions.push_back(sptr(FindObj(Obj::PRISON)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArmy() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(!solutions.empty())
		return solutions;

	for(auto creature : q.getQuest(cb)->mission.creatures)
	{
		solutions.push_back(sptr(GatherTroops(creature.getId().getNum(), creature.getCount())));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionIncreasePrimaryStat() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		for(int i = 0; i < q.getQuest(cb)->mission.primary.size(); ++i)
		{
			// TODO: library, school and other boost objects
			logAi->debug("Don't know how to increase primary stat %d", i);
		}
	}

	return solutions;
}

TGoalVec CompleteQuest::missionLevel() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		logAi->debug("Don't know how to reach hero level %d", q.getQuest(cb)->mission.heroLevel);
	}

	return solutions;
}

TGoalVec CompleteQuest::missionKeymaster() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		solutions.push_back(sptr(Goals::FindObj(Obj::KEYMASTER, q.getObject(cb)->subID)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionResources() const
{
	TGoalVec solutions;

	auto heroes = cb->getHeroesInfo(); //TODO: choose best / free hero from among many possibilities?

	if(heroes.size())
	{
		if(q.getQuest(cb)->checkQuest(heroes.front())) //it doesn't matter which hero it is
		{
			return ai->ah->howToVisitObj(q.getObject(cb));
		}
		else
		{
			for(int i = 0; i < q.getQuest(cb)->mission.resources.size(); ++i)
			{
				if(q.getQuest(cb)->mission.resources[i])
					solutions.push_back(sptr(CollectRes(static_cast<EGameResID>(i), q.getQuest(cb)->mission.resources[i])));
			}
		}
	}
	else
	{
		solutions.push_back(sptr(Goals::RecruitHero())); //FIXME: checkQuest requires any hero belonging to player :(
	}

	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj() const
{
	TGoalVec solutions;

	auto obj = cb->getObj(q.getQuest(cb)->killTarget);

	if(!obj)
		return ai->ah->howToVisitObj(q.getObject(cb));

	if(obj->ID == Obj::HERO)
	{
		auto relations = cb->getPlayerRelations(ai->playerID, obj->tempOwner);

		if(relations == PlayerRelations::SAME_PLAYER)
		{
			auto heroToProtect = cb->getHero(obj->id);

			solutions.push_back(sptr(GatherArmy().sethero(heroToProtect)));
		}
		else if(relations == PlayerRelations::ENEMIES)
		{
			solutions = ai->ah->howToVisitObj(obj);
		}
	}

	return solutions;
}
