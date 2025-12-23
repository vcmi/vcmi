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
#include "../../../lib/GameLibrary.h"
#include "../../../lib/mapObjects/CQuest.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

namespace NKAI
{

using namespace Goals;

bool isKeyMaster(const QuestInfo & q)
{
	auto object = q.getObject(cb);
	return object && (object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD);
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
	auto quest = q.getQuest(cb);

	if(!quest->mission.artifacts.empty())
		return missionArt(ai);

	if(!quest->mission.heroes.empty())
		return missionHero(ai);

	if(!quest->mission.creatures.empty())
		return missionArmy(ai);

	if(quest->mission.resources.nonZero())
		return missionResources(ai);

	if(quest->killTarget != ObjectInstanceID::NONE)
		return missionDestroyObj(ai);

	for(auto & s : quest->mission.primary)
		if(s)
			return missionIncreasePrimaryStat(ai);

	if(quest->mission.heroLevel > 0)
		return missionLevel(ai);

	return TGoalVec();
}

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	if(isKeyMaster(q))
	{
		return isKeyMaster(other.q) && q.getObject(cb)->subID == other.q.getObject(cb)->subID;
	}
	else if(isKeyMaster(other.q))
	{
		return false;
	}

	return q.getQuest(cb) == other.q.getQuest(cb);
}

uint64_t CompleteQuest::getHash() const
{
	if(isKeyMaster(q))
	{
		return q.getObject(cb)->subID;
	}

	return q.getObject(cb)->id.getNum();
}

std::string CompleteQuest::questToString() const
{
	if(isKeyMaster(q))
	{
		return "find " + LIBRARY->generaltexth->tentColors[q.getObject(cb)->subID] + " keymaster tent";
	}

	if(q.getQuest(cb)->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	q.getQuest(cb)->getRolloverText(cb, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest(const Nullkiller * ai) const
{
	auto paths = ai->pathfinder->getPathInfo(q.getObject(cb)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !q.getQuest(cb)->checkQuest(path.targetHero);
	});
	
	return CaptureObjectsBehavior::getVisitGoals(paths, ai, q.getObject(cb));
}

TGoalVec CompleteQuest::missionArt(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.getQuest(cb)->mission.artifacts)
	{
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::ARTIFACT, art.getNum())));
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
	auto paths = ai->pathfinder->getPathInfo(q.getObject(cb)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !CQuest::checkMissionArmy(q.getQuest(cb), path.heroArmy);
	});

	return CaptureObjectsBehavior::getVisitGoals(paths, ai, q.getObject(cb));
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
	if(isObjectPassable(ai, q.getObject(cb)))
	{
		return CaptureObjectsBehavior(q.getObject(cb)).decompose(ai);
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.getObject(cb)->subID).decompose(ai);
	}
}

TGoalVec CompleteQuest::missionResources(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);
	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj(const Nullkiller * ai) const
{
	auto obj = ai->cb->getObj(q.getQuest(cb)->killTarget);

	if(!obj)
		return CaptureObjectsBehavior(q.getObject(cb)).decompose(ai);

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
