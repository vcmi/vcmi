/*
* TownPortalAction.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "ISpecialAction.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../Goals/AdventureSpellCast.h"

namespace AIPathfinding
{
	class TownPortalAction : public ISpecialAction
	{
	private:
		const CGTownInstance * target;

	public:
		TownPortalAction(const CGTownInstance * target)
			:target(target)
		{
		}

		virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const override;
	};
}
