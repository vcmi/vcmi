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
#include "../Analyzers/DangerHitMapAnalyzer.h"

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT StayAtTown : public ElementarGoal<StayAtTown>
	{
	private:
		float movementWasted;

	public:
		StayAtTown(const CGTownInstance * town, AIPath & path);

		bool operator==(const StayAtTown & other) const override;
		std::string toString() const override;
		void accept(AIGateway * aiGw) override;
		float getMovementWasted() const { return movementWasted; }
	};
}

}
