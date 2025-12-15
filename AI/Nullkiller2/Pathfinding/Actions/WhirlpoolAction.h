/*
* WhirlpoolAction.h, part of VCMI engine
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
#include "../../Goals/AdventureSpellCast.h"

namespace NK2AI
{
namespace AIPathfinding
{
	class WhirlpoolAction : public SpecialAction
	{
	public:
		WhirlpoolAction()
		{
		}

		static std::shared_ptr<WhirlpoolAction> instance;

		void execute(AIGateway * aiGw, const CGHeroInstance * hero) const override;

		std::string toString() const override;
	};
}
}
