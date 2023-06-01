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
#include "../../../../lib/mapObjects/MapObjects.h"

namespace AIPathfinding
{
	class VirtualBoatAction : public ISpecialAction
	{
	private:
		uint64_t specialChain;

	public:
		VirtualBoatAction(uint64_t specialChain)
			:specialChain(specialChain)
		{
		}

		uint64_t getSpecialChain() const
		{
			return specialChain;
		}
	};
	
	class SummonBoatAction : public VirtualBoatAction
	{
	public:
		SummonBoatAction()
			:VirtualBoatAction(AINodeStorage::CAST_CHAIN)
		{
		}

		virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const override;

		virtual void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstMode,
			const AIPathNode * srcNode) const override;

		bool isAffordableBy(const CGHeroInstance * hero, const AIPathNode * source) const;

	private:
		uint32_t getManaCost(const CGHeroInstance * hero) const;
	};

	class BuildBoatAction : public VirtualBoatAction
	{
	private:
		const IShipyard * shipyard;

	public:
		BuildBoatAction(const IShipyard * shipyard)
			:VirtualBoatAction(AINodeStorage::RESOURCE_CHAIN), shipyard(shipyard)
		{
		}

		virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const override;
	};
}
