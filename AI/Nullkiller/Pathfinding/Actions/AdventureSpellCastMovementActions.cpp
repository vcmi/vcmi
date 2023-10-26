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
	AdventureCastAction::AdventureCastAction(SpellID spellToCast, const CGHeroInstance * hero)
		:spellToCast(spellToCast), hero(hero)
	{
		manaCost = hero->getSpellCost(spellToCast.toSpell());
	}

	WaterWalkingAction::WaterWalkingAction(const CGHeroInstance * hero)
		:AdventureCastAction(SpellID::WATER_WALK, hero)
	{ }

	AirWalkingAction::AirWalkingAction(const CGHeroInstance * hero)
		: AdventureCastAction(SpellID::FLY, hero)
	{
	}

	void AdventureCastAction::applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
		dstMode->manaCost = srcNode->manaCost + manaCost;
		dstMode->theNodeBefore = source.node;
	}

	void AdventureCastAction::execute(const CGHeroInstance * hero) const
	{
		assert(hero == this->hero);

		Goals::AdventureSpellCast(hero, spellToCast).accept(ai);
	}

	bool AdventureCastAction::canAct(const AIPathNode * source) const
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
