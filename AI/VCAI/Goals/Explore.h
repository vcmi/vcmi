/*
 * Goals.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "AIUtility.h"
#include "Tasks/Task.h"
#include "Goals/Goal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Goals
{
	class Explore : public CGoal<Explore>
	{
	public:
		Explore()
			: CGoal(Goals::EXPLORE)
		{
			priority = 1;
		}
		Explore(HeroPtr h)
			: CGoal(Goals::EXPLORE)
		{
			hero = h;
			priority = 1;
		}
		Tasks::TaskList getTasks() override;
	private:
		std::vector<const CGObjectInstance *> getExplorationHelperObjects();
	};
}

