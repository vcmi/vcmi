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

#include "SpecialAction.h"
#include "../../../../lib/mapping/CMap.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../Goals/AdventureSpellCast.h"

namespace NKAI
{
namespace AIPathfinding
{
	class TownPortalAction : public SpecialAction
	{
	private:
		const CGTownInstance * target;

	public:
		TownPortalAction(const CGTownInstance * target)
			:target(target)
		{
		}

		virtual void execute(const CGHeroInstance * hero) const override;

		virtual std::string toString() const override;
	};
}

}
