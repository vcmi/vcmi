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
	};
	
	class SummonBoatAction : public VirtualBoatAction
	{
		SpellID usedSpell;
	public:
		SummonBoatAction(SpellID usedSpell)
			: usedSpell(usedSpell)
		{
		}

		void execute(AIGateway * ai, const CGHeroInstance * hero) const override;

		virtual void applyOnDestination(
			const CGHeroInstance * hero,
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			AIPathNode * dstMode,
			const AIPathNode * srcNode) const override;

		bool canAct(const Nullkiller * ai, const AIPathNode * source) const override;

		const ChainActor * getActor(const ChainActor * sourceActor) const override;

		std::string toString() const override;

	private:
		int32_t getManaCost(const CGHeroInstance * hero) const;
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

		bool canAct(const Nullkiller * ai, const AIPathNode * source) const override;
		bool canAct(const Nullkiller * ai, const AIPathNodeInfo & source) const override;
		bool canAct(const Nullkiller * ai, const CGHeroInstance * hero, const TResources & reservedResources) const;

		void execute(AIGateway * ai, const CGHeroInstance * hero) const override;

		Goals::TSubgoal decompose(const Nullkiller * ai, const CGHeroInstance * hero) const override;

		const ChainActor * getActor(const ChainActor * sourceActor) const override;

		std::string toString() const override;

		const CGObjectInstance * targetObject() const override;
	};

	class BuildBoatActionFactory : public ISpecialActionFactory
	{
		ObjectInstanceID shipyard;

	public:
		BuildBoatActionFactory(ObjectInstanceID shipyard)
			:shipyard(shipyard)
		{
		}

		std::shared_ptr<SpecialAction> create(const Nullkiller * ai) override;
	};
}

}
