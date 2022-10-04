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

#include "lib/VCMI_Lib.h"
#include "../Goals/CGoal.h"
#include "../AIUtility.h"

namespace NKAI
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

		virtual TGoalVec decompose() const override;
		virtual std::string toString() const override;

		virtual bool operator==(const ClusterBehavior & other) const override
		{
			return true;
		}

	private:
		Goals::TGoalVec decomposeCluster(std::shared_ptr<ObjectCluster> cluster) const;
	};
}

}
