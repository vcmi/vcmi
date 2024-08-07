/*
* WhirlpoolAction.cpp, part of VCMI engine
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
#include "WhirlpoolAction.h"
#include "../../AIGateway.h"

namespace NKAI
{

using namespace AIPathfinding;

std::shared_ptr<WhirlpoolAction> WhirlpoolAction::instance = std::make_shared<WhirlpoolAction>();

void WhirlpoolAction::execute(AIGateway * ai, const CGHeroInstance * hero) const
{
	ai->nullkiller->armyFormation->rearrangeArmyForWhirlpool(hero);
}

std::string WhirlpoolAction::toString() const
{
	return "Prepare for whirlpool";
}
/*
bool TownPortalAction::canAct(const CGHeroInstance * hero, const AIPathNode * source) const
{
#ifdef VCMI_TRACE_PATHFINDER
	logAi->trace(
		"Hero %s has %d mana and needed %d and already spent %d",
		hero->name,
		hero->mana,
		getManaCost(hero),
		source->manaCost);
#endif

	return hero->mana >= source->manaCost + getManaCost(hero);
}

uint32_t TownPortalAction::getManaCost(const CGHeroInstance * hero) const
{
	SpellID summonBoat = SpellID::TOWN_PORTAL;

	return hero->getSpellCost(summonBoat.toSpell());
}*/

}
