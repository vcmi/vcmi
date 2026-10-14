/*
* ExecuteHeroChain.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"
#include "../Pathfinding/AIPathfinder.h"

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT ExecuteHeroChain : public ElementarGoal<ExecuteHeroChain>
	{
	private:
		AIPath chainPath;
		std::string targetName;

	public:
		float closestWayRatio;

		ExecuteHeroChain(const AIPath & path, const CGObjectInstance * obj = nullptr);

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const ExecuteHeroChain & other) const override;
		const AIPath & getPath() const { return chainPath; }

		int getHeroExchangeCount() const override { return chainPath.exchangeCount; }

		std::vector<ObjectInstanceID> getAffectedObjects() const override;
		bool isObjectAffected(ObjectInstanceID id) const override;

	private:
		bool moveHeroToTile(AIGateway * aiGw, const CGHeroInstance * hero, const int3 & tile);
	};
}

}
