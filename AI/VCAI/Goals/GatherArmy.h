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
	class GatherArmy : public CGoal<GatherArmy>
	{
	private:
		ui64 requiredAmmount;
		bool force;

	public:
		GatherArmy()
			: CGoal(Goals::GATHER_ARMY)
		{
		}
		GatherArmy(ui64 requiredAmmount, bool force = false)
			: CGoal(Goals::GATHER_ARMY)
		{
			this->requiredAmmount = requiredAmmount;
			this->force = force;
		}

		Tasks::TaskList getTasks() override;
	};

}

