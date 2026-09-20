/*
* PrepareFreeSlotForCreatureBankReward.h, part of VCMI engine
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
	class DLL_EXPORT PrepareFreeSlotForCreatureBankReward : public ElementarGoal<PrepareFreeSlotForCreatureBankReward>
	{
	private:
		AIPath exchangePath;
		const CGHeroInstance * rewardHero;

	public:
		PrepareFreeSlotForCreatureBankReward(
			const AIPath & exchangePath,
			const CGHeroInstance * rewardHero,
			const CGObjectInstance * creatureBank);

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const PrepareFreeSlotForCreatureBankReward & other) const override;
		const AIPath & getPath() const { return exchangePath; }

		int getHeroExchangeCount() const override { return exchangePath.exchangeCount; }
		std::vector<ObjectInstanceID> getAffectedObjects() const override;
		bool isObjectAffected(ObjectInstanceID id) const override;
	};
}
}
