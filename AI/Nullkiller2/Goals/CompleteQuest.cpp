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

namespace NK2AI
{

using namespace Goals;

bool isKeyMaster(const QuestInfo & q)
{
	auto object = q.getObject(cbcTl);
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
	auto quest = q.getQuest(cbcTl);

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
		return isKeyMaster(other.q) && q.getObject(cbcTl)->subID == other.q.getObject(cbcTl)->subID;
	}
	else if(isKeyMaster(other.q))
	{
		return false;
	}

	return q.getQuest(cbcTl) == other.q.getQuest(cbcTl);
}

uint64_t CompleteQuest::getHash() const
{
	if(isKeyMaster(q))
	{
		return q.getObject(cbcTl)->subID;
	}

	return q.getObject(cbcTl)->id.getNum();
}

std::string CompleteQuest::questToString() const
{
	if(isKeyMaster(q))
	{
		return "find " + LIBRARY->generaltexth->tentColors[q.getObject(cbcTl)->subID] + " keymaster tent";
	}

	if(q.getQuest(cbcTl)->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	q.getQuest(cbcTl)->getRolloverText(cbcTl, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest(const Nullkiller * aiNk) const
{
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(cbcTl)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !q.getQuest(cbcTl)->checkQuest(path.targetHero);
	});
	
	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(cbcTl));
}

TGoalVec CompleteQuest::missionArt(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.getQuest(cbcTl)->mission.artifacts)
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

TGoalVec CompleteQuest::missionArmy(const Nullkiller * aiNk) const
{
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(cbcTl)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !CQuest::checkMissionArmy(q.getQuest(cbcTl), path.heroArmy);
	});

	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(cbcTl));
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
	if(isObjectPassable(ai, q.getObject(cbcTl)))
	{
		return CaptureObjectsBehavior(q.getObject(cbcTl)).decompose(ai);
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.getObject(cbcTl)->subID).decompose(ai);
	}
}

TGoalVec CompleteQuest::missionResources(const Nullkiller * ai) const
{
	TGoalVec solutions = tryCompleteQuest(ai);
	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj(const Nullkiller * aiNk) const
{
	auto obj = aiNk->cbc->getObj(q.getQuest(cbcTl)->killTarget);

	if(!obj)
		return CaptureObjectsBehavior(q.getObject(cbcTl)).decompose(aiNk);

	auto relations = aiNk->cbc->getPlayerRelations(aiNk->playerID, obj->tempOwner);

	//if(relations == PlayerRelations::SAME_PLAYER)
	//{
	//	auto heroToProtect = cb->getHero(obj->id);

	//	//solutions.push_back(sptr(GatherArmy().sethero(heroToProtect)));
	//}
	//else 
	if(relations == PlayerRelations::ENEMIES)
	{
		return CaptureObjectsBehavior(obj).decompose(aiNk);
	}

	return TGoalVec();
}

}
