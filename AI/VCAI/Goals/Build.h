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

#include "lib/VCMI_Lib.h"
#include "lib/CBuildingHandler.h"
#include "lib/CCreatureHandler.h"
#include "lib/CTownHandler.h"
#include "../AIUtility.h"
#include "../Tasks/Task.h"
#include "Goal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Goals
{
	/// Helper class for Build goal
	class BuildingInfo;

	class Build : public CGoal<Build>
	{
	public:
		TResources resourcesNeeded;
		Build()
			: CGoal(Goals::BUILD)
		{
			priority = 1;
		}
		Tasks::TaskList getTasks() override;

	private:
		BuildingInfo getBuildingOrPrerequisite(
			const CGTownInstance * town,
			BuildingID toBuild,
			bool excludeDwellingDependencies = true);
	};
}

