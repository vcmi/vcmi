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

namespace NKAI
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

		
		void accept(AIGateway * ai) override;
		std::string toString() const override;
		virtual bool operator==(const ExecuteHeroChain & other) const override;
		const AIPath & getPath() const { return chainPath; }

		virtual int getHeroExchangeCount() const override { return chainPath.exchangeCount; }

	private:
		bool moveHeroToTile(const CGHeroInstance * hero, const int3 & tile);
	};
}

}
