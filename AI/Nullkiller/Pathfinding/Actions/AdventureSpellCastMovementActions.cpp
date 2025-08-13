/*
* AdventureSpellCastMovementActions.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "../../AIGateway.h"
#include "../../Goals/AdventureSpellCast.h"
#include "../../Goals/CaptureObject.h"
#include "../../Goals/Invalid.h"
#include "../../Goals/BuildBoat.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "AdventureSpellCastMovementActions.h"

namespace NKAI
{

namespace AIPathfinding
{
	AdventureCastAction::AdventureCastAction(SpellID spellToCast, const CGHeroInstance * hero, DayFlags flagsToAdd)
		:spellToCast(spellToCast), hero(hero), flagsToAdd(flagsToAdd)
	{
		manaCost = hero->getSpellCost(spellToCast.toSpell());
	}

	WaterWalkingAction::WaterWalkingAction(const CGHeroInstance * hero, SpellID spellToCast)
		:AdventureCastAction(spellToCast, hero, DayFlags::WATER_WALK_CAST)
	{ }

	AirWalkingAction::AirWalkingAction(const CGHeroInstance * hero, SpellID spellToCast)
		: AdventureCastAction(spellToCast, hero, DayFlags::FLY_CAST)
	{
	}

	void AdventureCastAction::applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstNode,
		const AIPathNode * srcNode) const
	{
		dstNode->manaCost = srcNode->manaCost + manaCost;
		dstNode->theNodeBefore = source.node;
		dstNode->dayFlags = static_cast<DayFlags>(dstNode->dayFlags | flagsToAdd);
	}

	void AdventureCastAction::execute(AIGateway * ai, const CGHeroInstance * hero) const
	{
		assert(hero == this->hero);

		Goals::AdventureSpellCast(hero, spellToCast).accept(ai);
	}

	bool AdventureCastAction::canAct(const Nullkiller * ai, const AIPathNode * source) const
	{
		assert(hero == this->hero);

		auto hero = source->actor->hero;

#ifdef VCMI_TRACE_PATHFINDER
		logAi->trace(
			"Hero %s has %d mana and needed %d and already spent %d",
			hero->name,
			hero->mana,
			getManaCost(hero),
			source->manaCost);
#endif

		return hero->mana >= source->manaCost + manaCost;
	}

	std::string AdventureCastAction::toString() const
	{
		return "Cast " + spellToCast.toSpell()->getNameTranslated() + " by " + hero->getNameTranslated();
	}
}

}
