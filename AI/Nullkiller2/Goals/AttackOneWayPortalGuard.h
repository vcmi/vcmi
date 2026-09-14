/*
* AttackOneWayPortalGuard.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace NK2AI
{

namespace Goals
{
	class DLL_EXPORT AttackOneWayPortalGuard : public ElementarGoal<AttackOneWayPortalGuard>
	{
	private:
		ObjectInstanceID exit;

	public:
		AttackOneWayPortalGuard(
			const CGHeroInstance * hero,
			const CGObjectInstance * guard,
			const CGObjectInstance * exit);

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const AttackOneWayPortalGuard & other) const override;

		std::vector<ObjectInstanceID> getAffectedObjects() const override;
		bool isObjectAffected(ObjectInstanceID id) const override;

		static const CGObjectInstance * findTrappingGuard(
			const CGHeroInstance * hero,
			const Nullkiller * aiNk);
	};
}

}
