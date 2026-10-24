/*
* PrepareFreeSlotForCreatureReward.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*/
#include "StdInc.h"
#include "PrepareFreeSlotForCreatureReward.h"
#include "ExecuteHeroChain.h"
#include "../AIGateway.h"
#include "../../../lib/ScopeGuard.h"

namespace NK2AI
{

using namespace Goals;

PrepareFreeSlotForCreatureReward::PrepareFreeSlotForCreatureReward(
	const AIPath & exchangePath,
	const AIPath & rewardPath,
	const CGObjectInstance * creatureBank)
	: ElementarGoal(Goals::PREPARE_FREE_SLOT_FOR_CREATURE_REWARD), exchangePath(exchangePath), rewardPath(rewardPath)
{
	hero = exchangePath.targetHero;
	if(creatureBank)
	{
		objid = creatureBank->id.getNum();
		bankNameAndPos = creatureBank->getTypeName() +" at "+ creatureBank->visitablePos().toString();
	}
}

void PrepareFreeSlotForCreatureReward::accept(AIGateway * aiGw)
{
	const auto * creatureBank = aiGw->cc->tryGetObj(ObjectInstanceID(objid));
	const auto * rewardHero = rewardPath.targetHero;
	if(!creatureBank
		|| exchangePath.targetTile() != rewardHero->visitablePos()
		|| rewardPath.targetTile() != creatureBank->visitablePos())
		throw cannotFulfillGoalException("Creature bank reward preparation is no longer valid.");

	logAi->debug("Executing slot preparation for %s. Creature bank: %s", rewardHero->getNameTextID(), bankNameAndPos);
	{
		aiGw->beginCreatureBankRewardPreparation(rewardHero->id, exchangePath.targetHero->id);
		auto clearPreparation = vstd::makeScopeGuard([aiGw]()
																							 { aiGw->endCreatureBankRewardPreparation(); });
		ExecuteHeroChain(exchangePath, creatureBank).accept(aiGw);
	}

	if(rewardHero->stacksCount() == GameConstants::ARMY_SIZE)
		throw cannotFulfillGoalException("Creature bank reward preparation did not free an army slot.");

	ExecuteHeroChain(rewardPath, creatureBank).accept(aiGw);
}

std::string PrepareFreeSlotForCreatureReward::toString() const
{
	return "Prepare free slot for "+ rewardPath.targetHero->getNameTextID() +" to receive reward from creature bank ("+ bankNameAndPos +")";
}

bool PrepareFreeSlotForCreatureReward::operator==(const PrepareFreeSlotForCreatureReward & other) const
{
	return rewardPath.targetHero == other.rewardPath.targetHero
		&& objid == other.objid
		&& exchangePath.targetHero == other.exchangePath.targetHero;
}

std::vector<ObjectInstanceID> PrepareFreeSlotForCreatureReward::getAffectedObjects() const
{
	auto result = ExecuteHeroChain(exchangePath, nullptr).getAffectedObjects();
	vstd::concatenate(result, ExecuteHeroChain(rewardPath, nullptr).getAffectedObjects());
	result.emplace_back(objid);
	vstd::removeDuplicates(result);
	return result;
}

bool PrepareFreeSlotForCreatureReward::isObjectAffected(ObjectInstanceID id) const
{
	return id == ObjectInstanceID(objid)
		|| ExecuteHeroChain(exchangePath, nullptr).isObjectAffected(id)
		|| ExecuteHeroChain(rewardPath, nullptr).isObjectAffected(id);
}

}
