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
#include "../../../lib/GameLibrary.h"
#include "../../../lib/mapObjects/CQuest.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../AIGateway.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "CompleteQuest.h"

namespace NK2AI
{

using namespace Goals;

bool isKeyMaster(const QuestInfo & q, CCallback & cc)
{
	const auto * const object = q.getObject(&cc);
	return object && (object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD);
}

std::string CompleteQuest::toString() const
{
	return "Complete quest " + questToString();
}

TGoalVec CompleteQuest::decompose(const Nullkiller * aiNk) const
{
	if(isKeyMaster(q, *aiNk->cc))
	{
		return missionKeymaster(aiNk);
	}

	logAi->debug("Trying to realize quest: %s", questToString());
	const auto quest = q.getQuest(aiNk->cc.get());

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
	if(isKeyMaster(q, cc))
	{
		return isKeyMaster(other.q, cc) && q.getObject(&cc)->subID == other.q.getObject(&cc)->subID;
	}
	else if(isKeyMaster(other.q, cc))
	{
		return false;
	}

	return q.getQuest(&cc) == other.q.getQuest(&cc);
}

uint64_t CompleteQuest::getHash() const
{
	if(isKeyMaster(q, cc))
	{
		return q.getObject(&cc)->subID;
	}

	return q.getObject(&cc)->id.getNum();
}

std::string CompleteQuest::questToString() const
{
	if(isKeyMaster(q, cc))
	{
		return "find " + LIBRARY->generaltexth->tentColors[q.getObject(&cc)->subID] + " keymaster tent";
	}

	if(q.getQuest(&cc)->questName == CQuest::missionName(EQuestMission::NONE))
		return "inactive quest";

	MetaString ms;
	q.getQuest(&cc)->getRolloverText(&cc, ms, false);

	return ms.toString();
}

TGoalVec CompleteQuest::tryCompleteQuest(const Nullkiller * aiNk) const
{
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(aiNk->cc.get())->visitablePos());

	vstd::erase_if(
		paths,
		[&](const AIPath & path) -> bool
		{
			return !q.getQuest(aiNk->cc.get())->checkQuest(path.targetHero);
		}
	);

	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(aiNk->cc.get()));
}

TGoalVec CompleteQuest::missionArt(const Nullkiller * aiNk) const
{
	TGoalVec solutions = tryCompleteQuest(aiNk);

	if(!solutions.empty())
		return solutions;

	CaptureObjectsBehavior findArts;

	for(auto art : q.getQuest(aiNk->cc.get())->mission.artifacts)
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
	auto paths = aiNk->pathfinder->getPathInfo(q.getObject(aiNk->cc.get())->visitablePos());

	vstd::erase_if(
		paths,
		[&](const AIPath & path) -> bool
		{
			return !CQuest::checkMissionArmy(q.getQuest(aiNk->cc.get()), path.heroArmy);
		}
	);

	return CaptureObjectsBehavior::getVisitGoals(paths, aiNk, q.getObject(aiNk->cc.get()));
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
	if(isObjectPassable(aiNk, q.getObject(aiNk->cc.get())))
	{
		return CaptureObjectsBehavior(q.getObject(aiNk->cc.get())).decompose(aiNk);
	}
	else
	{
		return CaptureObjectsBehavior().ofType(Obj::KEYMASTER, q.getObject(aiNk->cc.get())->subID).decompose(aiNk);
	}
}

TGoalVec CompleteQuest::missionResources(const Nullkiller * aiNk) const
{
	TGoalVec solutions = tryCompleteQuest(aiNk);
	return solutions;
}

TGoalVec CompleteQuest::missionDestroyObj(const Nullkiller * aiNk) const
{
	const auto obj = aiNk->cc->getObj(q.getQuest(aiNk->cc.get())->killTarget);
	if(!obj)
		return CaptureObjectsBehavior(q.getObject(aiNk->cc.get())).decompose(aiNk);

	const auto relations = aiNk->cc->getPlayerRelations(aiNk->playerID, obj->tempOwner);

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
