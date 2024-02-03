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
#include "Goals.h"
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapObjects/CQuest.h"

using namespace Goals;

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	return q.quest->qid == other.q.quest->qid;
}

bool isKeyMaster(const QuestInfo & q)
{
	return q.obj && (q.obj->ID == Obj::BORDER_GATE || q.obj->ID == Obj::BORDERGUARD);
}

TGoalVec CompleteQuest::getAllPossibleSubgoals()
{
	TGoalVec solutions;

	if(!q.quest->isCompleted)
	{
		logAi->debug("Trying to realize quest: %s", questToString());
		
		if(isKeyMaster(q))
			return missionKeymaster();

		if(!q.quest->mission.artifacts.empty())
			return missionArt();

		if(!q.quest->mission.heroes.empty())
			return missionHero();

		if(!q.quest->mission.creatures.empty())
			return missionArmy();

		if(q.quest->mission.resources.nonZero())
			return missionResources();

		if(q.quest->killTarget != ObjectInstanceID::NONE)
			return missionDestroyObj();

		for(auto & s : q.quest->mission.primary)
			if(s)
				return missionIncreasePrimaryStat();

		if(q.quest->mission.heroLevel > 0)
			return missionLevel();
	}

	return TGoalVec();
}

TSubgoal CompleteQuest::whatToDoToAchieve()
{
	if(q.quest->mission == Rewardable::Limiter{})
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
	if(q.quest->questName == CQuest::missionName(0))
		return "inactive quest";

	MetaString ms;
	q.quest->getRolloverText(q.obj->cb, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest() const
{
	TGoalVec solutions;

	auto heroes = cb->getHeroesInfo(); //TODO: choose best / free hero from among many possibilities?

	for(auto hero : heroes)
	{
		if(q.quest->checkQuest(hero))
		{
			vstd::concatenate(solutions, ai->ah->howToVisitObj(hero, ObjectIdRef(q.obj->id)));
		}
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArt() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(!solutions.empty())
		return solutions;

	for(auto art : q.quest->mission.artifacts)
	{
		solutions.push_back(sptr(GetArtOfType(art))); //TODO: transport?
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

	for(auto creature : q.quest->mission.creatures)
	{
		solutions.push_back(sptr(GatherTroops(creature.type->getId(), creature.count)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionIncreasePrimaryStat() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		for(int i = 0; i < q.quest->mission.primary.size(); ++i)
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
		logAi->debug("Don't know how to reach hero level %d", q.quest->mission.heroLevel);
	}

	return solutions;
}

TGoalVec CompleteQuest::missionKeymaster() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		solutions.push_back(sptr(Goals::FindObj(Obj::KEYMASTER, q.obj->subID)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionResources() const
{
	TGoalVec solutions;

	auto heroes = cb->getHeroesInfo(); //TODO: choose best / free hero from among many possibilities?

	if(heroes.size())
	{
		if(q.quest->checkQuest(heroes.front())) //it doesn't matter which hero it is
		{
			return ai->ah->howToVisitObj(q.obj);
		}
		else
		{
			for(int i = 0; i < q.quest->mission.resources.size(); ++i)
			{
				if(q.quest->mission.resources[i])
					solutions.push_back(sptr(CollectRes(static_cast<EGameResID>(i), q.quest->mission.resources[i])));
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

	auto obj = cb->getObj(q.quest->killTarget);

	if(!obj)
		return ai->ah->howToVisitObj(q.obj);

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
