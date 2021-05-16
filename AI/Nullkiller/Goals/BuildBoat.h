/*
* BuildBoat.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace Goals
{
	class DLL_EXPORT BuildBoat : public ElementarGoal<BuildBoat>
	{
	private:
		const IShipyard * shipyard;
		TSubgoal decomposeSingle() const override;

	public:
		BuildBoat(const IShipyard * shipyard)
			: ElementarGoal(Goals::BUILD_BOAT), shipyard(shipyard)
		{
		}

		void accept(VCAI * ai) override;
		std::string toString() const override;
		virtual bool operator==(const BuildBoat & other) const override;
	};
}
