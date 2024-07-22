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
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../AIGateway.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

namespace NKAI
{

using namespace Goals;

bool isKeyMaster(const QuestInfo & q)
{
	return q.obj && (q.obj->ID == Obj::BORDER_GATE || q.obj->ID == Obj::BORDERGUARD);
}

std::string CompleteQuest::toString() const
{
	return "Complete quest " + questToString();
}

TGoalVec CompleteQuest::decompose(const Nullkiller * ai) const
{
	if(isKeyMaster(q))
	{
		return missionKeymaster(ai);
	}

	logAi->debug("Trying to realize quest: %s", questToString());
	
	if(!q.quest->mission.artifacts.empty())
		return missionArt(ai);

	if(!q.quest->mission.heroes.empty())
		return missionHero(ai);

	if(!q.quest->mission.creatures.empty())
		return missionArmy(ai);

	if(q.quest->mission.resources.nonZero())
		return missionResources(ai);

	if(q.quest->killTarget != ObjectInstanceID::NONE)
		return missionDestroyObj(ai);

	for(auto & s : q.quest->mission.primary)
		if(s)
			return missionIncreasePrimaryStat(ai);

	if(q.quest->mission.heroLevel > 0)
		return missionLevel(ai);

	return TGoalVec();
}

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	if(isKeyMaster(q))
	{
		return isKeyMaster(other.q) && q.obj->subID == other.q.obj->subID;
	}
	else if(isKeyMaster(other.q))
	{
		return false;
	}

	return q.quest->qid == other.q.quest->qid;
}

uint64_t CompleteQuest::getHash() const
{
	if(isKeyMaster(q))
	{
		return q.obj->subID;
	}

	return q.quest->qid;
}

std::string CompleteQuest::questToString() const
{
	if(isKeyMaster(q))
	{
		return "find " + VLC->generaltexth->tentColors[q.obj->subID] + " keymaster tent";
	}

	if(q.quest->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	q.quest->getRolloverText(q.obj->cb, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest(const Nullkiller * ai) const
{
	auto paths = ai->pathfinder->getPathInfo(q.obj->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !q.quest->checkQuest(path.targetHero);
	});
	
	return CaptureObjectsBehavior::getVisitGoals(paths, ai, q.obj);
}

TGoalVec CompleteQuest::missionArt(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.quest->mission.artifacts)
	{
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::ARTIFACT, art)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionHero(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);

	if(solutions.empty())
	{
		//rule of a thumb - quest heroes usually are locked in prisons
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::PRISON)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArmy(const Nullkiller * ai) const
{
	auto paths = ai->pathfinder->getPathInfo(q.obj->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !CQuest::checkMissionArmy(q.quest, path.heroArmy);
	});

	return CaptureObjectsBehavior::getVisitGoals(paths, ai, q.obj);
}

TGoalVec CompleteQuest::missionIncreasePrimaryStat(const Nullkiller * ai) const
{
	return tryCompleteQuest(ai);
}

TGoalVec CompleteQuest::missionLevel(const Nullkiller * ai) const
{
	return tryCompleteQuest(ai);
}

TGoalVec CompleteQuest::missionKeymaster(const Nullkiller * ai) const
{
	if(isObjectPassable(ai, q.obj))
	{
		return CaptureObjectsBehavior(q.obj).decompose(ai);
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.obj->subID).decompose(ai);
	}
}

TGoalVec CompleteQuest::missionResources(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);

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
					solutions.push_back(sptr(CollectRes(static_cast<EGameResID>(i), q.quest->m7resources[i])));
			}
		}
	}
	else
	{
		solutions.push_back(sptr(Goals::RecruitHero())); //FIXME: checkQuest requires any hero belonging to player :(
	}*/

	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj(const Nullkiller * ai) const
{
	auto obj = ai->cb->getObj(q.quest->killTarget);

	if(!obj)
		return CaptureObjectsBehavior(q.obj).decompose(ai);

	auto relations = ai->cb->getPlayerRelations(ai->playerID, obj->tempOwner);

	//if(relations == PlayerRelations::SAME_PLAYER)
	//{
	//	auto heroToProtect = cb->getHero(obj->id);

	//	//solutions.push_back(sptr(GatherArmy().sethero(heroToProtect)));
	//}
	//else 
	if(relations == PlayerRelations::ENEMIES)
	{
		return CaptureObjectsBehavior(obj).decompose(ai);
	}

	return TGoalVec();
}

}
