/*
* CompleteQuestBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "CompleteQuestBehavior.h"
#include "CaptureObjectsBehavior.h"
#include "../VCAI.h"
#include "../AIhelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

std::string CompleteQuestBehavior::toString() const
{
	return "Complete Quests";
}

TGoalVec CompleteQuestBehavior::decompose() const
{
	TGoalVec solutions;

	auto quests = cb->getMyQuests();

	for(auto & q : quests)
	{
		if(q.quest->missionType == CQuest::MISSION_NONE || q.quest->progress == CQuest::COMPLETE)
		{
			continue;
		}

		vstd::concatenate(solutions, getQuestTasks(q));
	}

	return solutions;
}

TGoalVec CompleteQuestBehavior::getQuestTasks(const QuestInfo & q) const
{
	logAi->debug("Trying to realize quest: %s", questToString(q));

	switch(q.quest->missionType)
	{
	case CQuest::MISSION_ART:
		return missionArt(q);

	case CQuest::MISSION_HERO:
		return missionHero(q);

	case CQuest::MISSION_ARMY:
		return missionArmy(q);

	case CQuest::MISSION_RESOURCES:
		return missionResources(q);

	case CQuest::MISSION_KILL_HERO:
	case CQuest::MISSION_KILL_CREATURE:
		return missionDestroyObj(q);

	case CQuest::MISSION_PRIMARY_STAT:
		return missionIncreasePrimaryStat(q);

	case CQuest::MISSION_LEVEL:
		return missionLevel(q);

	case CQuest::MISSION_PLAYER:
		if(ai->playerID.getNum() != q.quest->m13489val)
			logAi->debug("Can't be player of color %d", q.quest->m13489val);

		break;

	case CQuest::MISSION_KEYMASTER:
		return missionKeymaster(q);

	} //end of switch

	return TGoalVec();
}

std::string CompleteQuestBehavior::questToString(const QuestInfo & q) const
{
	if(q.quest->missionType == CQuest::MISSION_NONE)
		return "inactive quest";

	MetaString ms;
	q.quest->getRolloverText(ms, false);

	return ms.toString();
}

TGoalVec CompleteQuestBehavior::tryCompleteQuest(const QuestInfo & q) const
{
	TGoalVec solutions;

	auto tasks = CaptureObjectsBehavior(q.obj).decompose(); //TODO: choose best / free hero from among many possibilities?

	for(auto task : tasks)
	{
		if(task->hero && q.quest->checkQuest(task->hero.get()))
		{
			solutions.push_back(task);
		}
	}

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionArt(const QuestInfo & q) const
{
	TGoalVec solutions = tryCompleteQuest(q);

	if(!solutions.empty())
		return solutions;

	/*for(auto art : q.quest->m5arts)
	{
		solutions.push_back(sptr(GetArtOfType(art))); //TODO: transport?
	}*/

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionHero(const QuestInfo & q) const
{
	TGoalVec solutions = tryCompleteQuest(q);

	if(solutions.empty())
	{
		//rule of a thumb - quest heroes usually are locked in prisons
		return CaptureObjectsBehavior().ofType(Obj::PRISON).decompose();
	}

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionArmy(const QuestInfo & q) const
{
	TGoalVec solutions = tryCompleteQuest(q);

	if(!solutions.empty())
		return solutions;
	/*
	for(auto creature : q.quest->m6creatures)
	{
		solutions.push_back(sptr(GatherTroops(creature.type->idNumber, creature.count)));
	}*/

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionIncreasePrimaryStat(const QuestInfo & q) const
{
	return tryCompleteQuest(q);
}

TGoalVec CompleteQuestBehavior::missionLevel(const QuestInfo & q) const
{
	return tryCompleteQuest(q);
}

TGoalVec CompleteQuestBehavior::missionKeymaster(const QuestInfo & q) const
{
	TGoalVec solutions = tryCompleteQuest(q);

	if(solutions.empty())
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.obj->subID).decompose();
	}

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionResources(const QuestInfo & q) const
{
	TGoalVec solutions;

	/*auto heroes = cb->getHeroesInfo(); //TODO: choose best / free hero from among many possibilities?

	if(heroes.size())
	{
		if(q.quest->checkQuest(heroes.front())) //it doesn't matter which hero it is
		{
			return solutions;// ai->ah->howToVisitObj(q.obj);
		}
		else
		{
			for(int i = 0; i < q.quest->m7resources.size(); ++i)
			{
				if(q.quest->m7resources[i])
					solutions.push_back(sptr(CollectRes(i, q.quest->m7resources[i])));
			}
		}
	}
	else
	{
		solutions.push_back(sptr(Goals::RecruitHero())); //FIXME: checkQuest requires any hero belonging to player :(
	}*/

	return solutions;
}

TGoalVec CompleteQuestBehavior::missionDestroyObj(const QuestInfo & q) const
{
	auto obj = cb->getObjByQuestIdentifier(q.quest->m13489val);

	if(!obj)
		return CaptureObjectsBehavior(q.obj).decompose();

	if(obj->ID == Obj::HERO)
	{
		auto relations = cb->getPlayerRelations(ai->playerID, obj->tempOwner);

		//if(relations == PlayerRelations::SAME_PLAYER)
		//{
		//	auto heroToProtect = cb->getHero(obj->id);

		//	//solutions.push_back(sptr(GatherArmy().sethero(heroToProtect)));
		//}
		//else 
		if(relations == PlayerRelations::ENEMIES)
		{
			return CaptureObjectsBehavior(obj).decompose();
		}
	}

	return TGoalVec();
}