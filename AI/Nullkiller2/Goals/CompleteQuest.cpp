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
	auto object = q.getObject(ccTl);
	return object && (object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD);
}

std::string CompleteQuest::toString() const
{
	return "Complete quest " + questToString();
}

TGoalVec CompleteQuest::decompose(const Nullkiller * aiNk) const
{
	if(isKeyMaster(q))
	{
		return missionKeymaster(aiNk);
	}

	logAi->debug("Trying to realize quest: %s", questToString());
	auto quest = q.getQuest(ccTl);

	if(!quest->mission.artifacts.empty())
		return missionArt(aiNk);

	if(!quest->mission.heroes.empty())
		return missionHero(aiNk);

	if(!quest->mission.creatures.empty())
		return missionArmy(aiNk);

	if(quest->mission.resources.nonZero())
		return missionResources(aiNk);

	if(quest->killTarget != ObjectInstanceID::NONE)
		return missionDestroyObj(aiNk);

	for(auto & s : quest->mission.primary)
		if(s)
			return missionIncreasePrimaryStat(aiNk);

	if(quest->mission.heroLevel > 0)
		return missionLevel(aiNk);

	return TGoalVec();
}

bool CompleteQuest::operator==(const CompleteQuest & other) const
{
	if(isKeyMaster(q))
	{
		return isKeyMaster(other.q) && q.getObject(ccTl)->subID == other.q.getObject(ccTl)->subID;
	}
	else if(isKeyMaster(other.q))
	{
		return false;
	}

	return q.getQuest(ccTl) == other.q.getQuest(ccTl);
}

uint64_t CompleteQuest::getHash() const
{
	if(isKeyMaster(q))
	{
		return q.getObject(ccTl)->subID;
	}

	return q.getObject(ccTl)->id.getNum();
}

std::string CompleteQuest::questToString() const
{
	if(isKeyMaster(q))
	{
		return "find " + LIBRARY->generaltexth->tentColors[q.getObject(ccTl)->subID] + " keymaster tent";
	}

	if(q.getQuest(ccTl)->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	q.getQuest(ccTl)->getRolloverText(ccTl, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest(const Nullkiller * aiNk) const
{
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(ccTl)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !q.getQuest(ccTl)->checkQuest(path.targetHero);
	});
	
	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(ccTl));
}

TGoalVec CompleteQuest::missionArt(const Nullkiller * aiNk) const
{
	TGoalVec solutions = tryCompleteQuest(aiNk);

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.getQuest(ccTl)->mission.artifacts)
	{
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::ARTIFACT, art.getNum())));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionHero(const Nullkiller * aiNk) const
{
	TGoalVec solutions = tryCompleteQuest(aiNk);

	if(solutions.empty())
	{
		//rule of a thumb - quest heroes usually are locked in prisons
		solutions.push_back(sptr(CaptureObjectsBehavior().ofType(Obj::PRISON)));
	}

	return solutions;
}

TGoalVec CompleteQuest::missionArmy(const Nullkiller * aiNk) const
{
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(ccTl)->visitablePos());

	vstd::erase_if(paths, [&](const AIPath & path) -> bool
	{
		return !CQuest::checkMissionArmy(q.getQuest(ccTl), path.heroArmy);
	});

	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(ccTl));
}

TGoalVec CompleteQuest::missionIncreasePrimaryStat(const Nullkiller * aiNk) const
{
	return tryCompleteQuest(aiNk);
}

TGoalVec CompleteQuest::missionLevel(const Nullkiller * aiNk) const
{
	return tryCompleteQuest(aiNk);
}

TGoalVec CompleteQuest::missionKeymaster(const Nullkiller * aiNk) const
{
	if(isObjectPassable(aiNk, q.getObject(ccTl)))
	{
		return CaptureObjectsBehavior(q.getObject(ccTl)).decompose(aiNk);
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.getObject(ccTl)->subID).decompose(aiNk);
	}
}

TGoalVec CompleteQuest::missionResources(const Nullkiller * aiNk) const
{
	TGoalVec solutions = tryCompleteQuest(aiNk);
	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj(const Nullkiller * aiNk) const
{
	auto obj = aiNk->cc->getObj(q.getQuest(ccTl)->killTarget);

	if(!obj)
		return CaptureObjectsBehavior(q.getObject(ccTl)).decompose(aiNk);

	auto relations = aiNk->cc->getPlayerRelations(aiNk->playerID, obj->tempOwner);

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
