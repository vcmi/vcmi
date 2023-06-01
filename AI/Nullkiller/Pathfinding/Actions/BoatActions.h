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

#include "SpecialAction.h"
#include "../../../../lib/mapObjects/MapObjects.h"

namespace NKAI
{

namespace AIPathfinding
{
	class VirtualBoatAction : public SpecialAction
	{
	public:
		virtual const ChainActor * getActor(const ChainActor * sourceActor) const = 0;
	};
	
	class SummonBoatAction : public VirtualBoatAction
	{
	public:
		virtual void execute(const CGHeroInstance * hero) const override;

		virtual void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstMode,
			const AIPathNode * srcNode) const override;

		virtual bool canAct(const AIPathNode * source) const override;

		virtual const ChainActor * getActor(const ChainActor * sourceActor) const override;

		virtual std::string toString() const override;

	private:
		uint32_t getManaCost(const CGHeroInstance * hero) const;
	};

	class BuildBoatAction : public VirtualBoatAction
	{
	private:
		const IShipyard * shipyard;
		const CPlayerSpecificInfoCallback * cb;

	public:
		BuildBoatAction(const CPlayerSpecificInfoCallback * cb, const IShipyard * shipyard)
			: cb(cb), shipyard(shipyard)
		{
		}

		virtual bool canAct(const AIPathNode * source) const override;

		virtual void execute(const CGHeroInstance * hero) const override;

		virtual Goals::TSubgoal decompose(const CGHeroInstance * hero) const override;

		virtual const ChainActor * getActor(const ChainActor * sourceActor) const override;

		virtual std::string toString() const override;

		virtual const CGObjectInstance * targetObject() const override;
	};
}

}
