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
#include "../../../lib/CPathfinder.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/CGeneralTextHandler.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

bool isKeyMaster(const QuestInfo & q)
{
	return q.obj && (q.obj->ID == Obj::BORDER_GATE || q.obj->ID == Obj::BORDERGUARD);
}

std::string CompleteQuest::toString() const
{
	return "Complete quest " + questToString();
}

TGoalVec CompleteQuest::decompose() const
{
	if(isKeyMaster(q))
	{
		return missionKeymaster();
	}

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

	if(q.quest->missionType == CQuest::MISSION_NONE)
		return "inactive quest";

	MetaString ms;
	q.quest->getRolloverText(ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest() const
{
	auto paths = ai->nullkiller->pathfinder->getPathInfo(q.obj->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !q.quest->checkQuest(path.targetHero);
	});
	
	return CaptureObjectsBehavior::getVisitGoals(paths, q.obj);
}

TGoalVec CompleteQuest::missionArt() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.quest->m5arts)
	{
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::ARTIFACT, art)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionHero() const
{
	TGoalVec solutions = tryCompleteQuest();

	if(solutions.empty())
	{
		//rule of a thumb - quest heroes usually are locked in prisons
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::PRISON)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArmy() const
{
	auto paths = ai->nullkiller->pathfinder->getPathInfo(q.obj->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !CQuest::checkMissionArmy(q.quest, path.heroArmy);
	});

	return CaptureObjectsBehavior::getVisitGoals(paths, q.obj);
}

TGoalVec CompleteQuest::missionIncreasePrimaryStat() const
{
	return tryCompleteQuest();
}

TGoalVec CompleteQuest::missionLevel() const
{
	return tryCompleteQuest();
}

TGoalVec CompleteQuest::missionKeymaster() const
{
	if(isObjectPassable(q.obj))
	{
		return CaptureObjectsBehavior(q.obj).decompose();
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.obj->subID).decompose();
	}
}

TGoalVec CompleteQuest::missionResources() const
{
	TGoalVec solutions = tryCompleteQuest();

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

TGoalVec CompleteQuest::missionDestroyObj() const
{
	auto obj = cb->getObjByQuestIdentifier(q.quest->m13489val);

	if(!obj)
		return CaptureObjectsBehavior(q.obj).decompose();

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

	return TGoalVec();
}

}
