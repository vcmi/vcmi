/*
* PrepareFreeSlotForCreatureBankReward.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*/
#include "StdInc.h"
#include "PrepareFreeSlotForCreatureBankReward.h"
#include "ExecuteHeroChain.h"
#include "../AIGateway.h"

namespace NK2AI
{

using namespace Goals;

PrepareFreeSlotForCreatureBankReward::PrepareFreeSlotForCreatureBankReward(
	const AIPath & exchangePath,
	const CGHeroInstance * rewardHero,
	const CGObjectInstance * creatureBank)
	: ElementarGoal(Goals::PREPARE_FREE_SLOT_FOR_CREATURE_BANK_REWARD), exchangePath(exchangePath), rewardHero(rewardHero)
{
	hero = exchangePath.targetHero;
	setobjid(creatureBank->id.getNum());
}

void PrepareFreeSlotForCreatureBankReward::accept(AIGateway * aiGw)
{
	const auto * creatureBank = aiGw->cc->getObj(ObjectInstanceID(objid), false);
	if(!creatureBank || exchangePath.targetTile() != rewardHero->visitablePos())
		throw cannotFulfillGoalException("Creature bank reward preparation is no longer valid.");

	ExecuteHeroChain(exchangePath, creatureBank).accept(aiGw);
}

std::string PrepareFreeSlotForCreatureBankReward::toString() const
{
	return "Prepare free slot for creature bank reward for " + rewardHero->getNameTextID();
}

bool PrepareFreeSlotForCreatureBankReward::operator==(const PrepareFreeSlotForCreatureBankReward & other) const
{
	return rewardHero == other.rewardHero && objid == other.objid && exchangePath.targetHero == other.exchangePath.targetHero;
}

std::vector<ObjectInstanceID> PrepareFreeSlotForCreatureBankReward::getAffectedObjects() const
{
	auto result = ExecuteHeroChain(exchangePath, nullptr).getAffectedObjects();
	result.push_back(rewardHero->id);
	result.push_back(ObjectInstanceID(objid));
	vstd::removeDuplicates(result);
	return result;
}

bool PrepareFreeSlotForCreatureBankReward::isObjectAffected(ObjectInstanceID id) const
{
	return id == rewardHero->id || id == ObjectInstanceID(objid) || ExecuteHeroChain(exchangePath, nullptr).isObjectAffected(id);
}

}
