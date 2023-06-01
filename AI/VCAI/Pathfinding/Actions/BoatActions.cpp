/*
* BoatActions.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "../../Goals/AdventureSpellCast.h"
#include "../../Goals/BuildBoat.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "BoatActions.h"

namespace AIPathfinding
{
	Goals::TSubgoal BuildBoatAction::whatToDo(const HeroPtr & hero) const
	{
		return Goals::sptr(Goals::BuildBoat(shipyard));
	}

	Goals::TSubgoal SummonBoatAction::whatToDo(const HeroPtr & hero) const
	{
		return Goals::sptr(Goals::AdventureSpellCast(hero, SpellID::SUMMON_BOAT));
	}

	void SummonBoatAction::applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
		dstMode->manaCost = srcNode->manaCost + getManaCost(hero);
		dstMode->theNodeBefore = source.node;
	}

	bool SummonBoatAction::isAffordableBy(const CGHeroInstance * hero, const AIPathNode * source) const
	{
#ifdef VCMI_TRACE_PATHFINDER
		logAi->trace(
			"Hero %s has %d mana and needed %d and already spent %d",
			hero->name,
			hero->mana,
			getManaCost(hero),
			source->manaCost);
#endif

		return hero->mana >= (si32)(source->manaCost + getManaCost(hero));
	}

	uint32_t SummonBoatAction::getManaCost(const CGHeroInstance * hero) const
	{
		SpellID summonBoat = SpellID::SUMMON_BOAT;

		return hero->getSpellCost(summonBoat.toSpell());
	}
}
