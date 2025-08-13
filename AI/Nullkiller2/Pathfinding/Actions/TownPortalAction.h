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
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../Goals/AdventureSpellCast.h"

namespace NK2AI
{
namespace AIPathfinding
{
	class TownPortalAction : public SpecialAction
	{
	private:
		const CGTownInstance * target;
		SpellID usedSpell;

	public:
		TownPortalAction(const CGTownInstance * target, SpellID usedSpell)
			:target(target)
			,usedSpell(usedSpell)
		{
		}

		void execute(AIGateway * ai, const CGHeroInstance * hero) const override;

		std::string toString() const override;
	};
}

}
