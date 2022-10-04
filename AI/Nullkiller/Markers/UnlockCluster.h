/*
* FindObj.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Analyzers/ObjectClusterizer.h"


namespace NKAI
{

struct HeroPtr;
class AIGateway;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT UnlockCluster : public CGoal<UnlockCluster>
	{
	private:
		std::shared_ptr<ObjectCluster> cluster;
		AIPath pathToCenter;

	public:
		UnlockCluster(std::shared_ptr<ObjectCluster> cluster, const AIPath & pathToCenter)
			: CGoal(Goals::UNLOCK_CLUSTER), cluster(cluster), pathToCenter(pathToCenter)
		{
			tile = cluster->blocker->visitablePos();
			sethero(pathToCenter.targetHero);
		}

		virtual bool operator==(const UnlockCluster & other) const override;
		virtual std::string toString() const override;
		std::shared_ptr<ObjectCluster> getCluster() const { return cluster; }
		const AIPath & getPathToCenter() { return pathToCenter; }
	};
}

}
