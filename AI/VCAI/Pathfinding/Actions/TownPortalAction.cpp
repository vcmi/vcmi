/*
* TownPortalAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "../../Goals/AdventureSpellCast.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "TownPortalAction.h"

using namespace AIPathfinding;

Goals::TSubgoal TownPortalAction::whatToDo(const HeroPtr & hero) const
{
	const CGTownInstance * targetTown = target; // const pointer is not allowed in settown

	return Goals::sptr(Goals::AdventureSpellCast(hero, SpellID::TOWN_PORTAL).settown(targetTown).settile(targetTown->visitablePos()));
}
