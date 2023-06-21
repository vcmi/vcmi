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

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	return q.quest->qid == other.q.quest->qid;
}

TGoalVec CompleteQuest::getAllPossibleSubgoals()
{
	TGoalVec solutions;

	if(q.quest->missionType && q.quest->progress != CQuest::COMPLETE)
	{
		logAi->debug("Trying to realize quest: %s", questToString());

		switch(q.quest->missionType)
		{
		case CQuest::MISSION_ART:
			return missionArt();

		case CQuest::MISSION_HERO:
			return missionHero();

		case CQuest::MISSION_ARMY:
			return missionArmy();

		case CQuest::MISSION_RESOURCES:
			return missionResources();

		case CQuest::MISSION_KILL_HERO:
		case CQuest::MISSION_KILL_CREATURE:
			return missionDestroyObj();

		case CQuest::MISSION_PRIMARY_STAT:
			return missionIncreasePrimaryStat();

		case CQuest::MISSION_LEVEL:
			return missionLevel();

		case CQuest::MISSION_PLAYER:
			if(ai->playerID.getNum() != q.quest->m13489val)
				logAi->debug("Can't be player of color %d", q.quest->m13489val);

			break;
		
		case CQuest::MISSION_KEYMASTER:
			return missionKeymaster();

		} //end of switch
	}

	return TGoalVec();
}

TSubgoal CompleteQuest::whatToDoToAchieve()
{
	if(q.quest->missionType == CQuest::MISSION_NONE)
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
	if(q.quest->missionType == CQuest::MISSION_NONE)
		return "inactive quest";

	MetaString ms;
	q.quest->getRolloverText(ms, false);

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

	for(auto art : q.quest->m5arts)
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

	for(auto creature : q.quest->m6creatures)
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
		for(int i = 0; i < q.quest->m2stats.size(); ++i)
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
		logAi->debug("Don't know how to reach hero level %d", q.quest->m13489val);
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
			for(int i = 0; i < q.quest->m7resources.size(); ++i)
			{
				if(q.quest->m7resources[i])
					solutions.push_back(sptr(CollectRes(static_cast<EGameResID>(i), q.quest->m7resources[i])));
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

	auto obj = cb->getObjByQuestIdentifier(q.quest->m13489val);

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
