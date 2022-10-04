/*
* ArmyUpgrade.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Pathfinding/AINodeStorage.h"
#include "../Analyzers/ArmyManager.h"

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT ArmyUpgrade : public CGoal<ArmyUpgrade>
	{
	private:
		const CGObjectInstance * upgrader;
		uint64_t initialValue;
		uint64_t upgradeValue;
		uint64_t goldCost;

	public:
		ArmyUpgrade(const AIPath & upgradePath, const CGObjectInstance * upgrader, const ArmyUpgradeInfo & upgrade);

		virtual bool operator==(const ArmyUpgrade & other) const override;
		virtual std::string toString() const override;

		uint64_t getUpgradeValue() const { return upgradeValue; }
		uint64_t getInitialArmyValue() const { return initialValue; }
	};
}

}
