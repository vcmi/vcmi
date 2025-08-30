/*
* RecruitHeroBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/GameLibrary.h"
#include "../Goals/CGoal.h"
#include "../AIUtility.h"

namespace NK2AI
{

struct ObjectCluster;

namespace Goals
{
	class ClusterBehavior : public CGoal<ClusterBehavior>
	{
	public:
		ClusterBehavior()
			:CGoal(CLUSTER_BEHAVIOR)
		{
		}

		TGoalVec decompose(const Nullkiller * aiNk) const override;
		std::string toString() const override;

		bool operator==(const ClusterBehavior & other) const override
		{
			return true;
		}

	private:
		Goals::TGoalVec decomposeCluster(const Nullkiller * aiNk, std::shared_ptr<ObjectCluster> cluster) const;
	};
}

}
