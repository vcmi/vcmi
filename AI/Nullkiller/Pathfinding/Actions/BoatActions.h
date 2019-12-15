/*
* BoatActions.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "ISpecialAction.h"
#include "../../../../lib/mapping/CMap.h"
#include "../../../../lib/mapObjects/MapObjects.h"

namespace AIPathfinding
{
	class VirtualBoatAction : public ISpecialAction
	{
	public:
		virtual const ChainActor * getActor(const ChainActor * sourceActor) const = 0;
	};
	
	class SummonBoatAction : public VirtualBoatAction
	{
	public:
		virtual Goals::TSubgoal whatToDo(const CGHeroInstance * hero) const override;

		virtual void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstMode,
			const AIPathNode * srcNode) const override;

		bool isAffordableBy(const CGHeroInstance * hero, const AIPathNode * source) const;

		virtual const ChainActor * getActor(const ChainActor * sourceActor) const override;

	private:
		uint32_t getManaCost(const CGHeroInstance * hero) const;
	};

	class BuildBoatAction : public VirtualBoatAction
	{
	private:
		const IShipyard * shipyard;

	public:
		BuildBoatAction(const IShipyard * shipyard)
			: shipyard(shipyard)
		{
		}

		virtual Goals::TSubgoal whatToDo(const CGHeroInstance * hero) const override;

		virtual const ChainActor * getActor(const ChainActor * sourceActor) const override;
	};
}