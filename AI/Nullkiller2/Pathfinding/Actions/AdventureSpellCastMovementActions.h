/*
* AdventureSpellCastMovementActions.h, part of VCMI engine
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

namespace NK2AI
{

namespace AIPathfinding
{
	class AdventureCastAction : public SpecialAction
	{
	private:
		SpellID spellToCast;
		const CGHeroInstance * hero;
		int manaCost;
		DayFlags flagsToAdd;

	public:
		AdventureCastAction(SpellID spellToCast, const CGHeroInstance * hero, DayFlags flagsToAdd = DayFlags::NONE);

		void execute(AIGateway * aiGw, const CGHeroInstance * hero) const override;

		virtual void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstMode,
			const AIPathNode * srcNode) const override;

		bool canAct(const Nullkiller * aiNk, const AIPathNode * source) const override;

		std::string toString() const override;
	};

	class WaterWalkingAction : public AdventureCastAction
	{
		SpellID spellToCast;
	public:
		WaterWalkingAction(const CGHeroInstance * hero, SpellID spellToCast);
	};

	class AirWalkingAction : public AdventureCastAction
	{
		SpellID spellToCast;
	public:
		AirWalkingAction(const CGHeroInstance * hero, SpellID spellToCast);
	};
}

}
