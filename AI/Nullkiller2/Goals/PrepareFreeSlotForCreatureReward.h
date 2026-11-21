/*
* PrepareFreeSlotForCreatureReward.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*/
#pragma once

#include "CGoal.h"
#include "../Pathfinding/AINodeStorage.h"

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT PrepareFreeSlotForCreatureReward : public ElementarGoal<PrepareFreeSlotForCreatureReward>
	{
	private:
		AIPath exchangePath;
		AIPath rewardPath;
		std::string bankNameAndPos;

	public:
		PrepareFreeSlotForCreatureReward(const AIPath & exchangePath, const AIPath & rewardPath, const CGObjectInstance * creatureBank);

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const PrepareFreeSlotForCreatureReward & other) const override;
		const AIPath & getPath() const { return exchangePath; }
		const CGHeroInstance * getRewardHero() const { return rewardPath.targetHero; }

		int getHeroExchangeCount() const override { return exchangePath.exchangeCount; }
		std::vector<ObjectInstanceID> getAffectedObjects() const override;
		bool isObjectAffected(ObjectInstanceID id) const override;
	};
}
}
