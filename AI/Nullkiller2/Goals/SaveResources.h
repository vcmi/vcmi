/*
* SaveResources.h, part of VCMI engine
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
	// TODO: Mircea: Inspect if it's really in use. See aiNk->getLockedResources()
	class DLL_EXPORT SaveResources : public ElementarGoal<SaveResources>
	{
	private:
		TResources resources;

	public:
		SaveResources(TResources resources)
			: ElementarGoal(Goals::SAVE_RESOURCES), resources(resources)
		{
		}

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const SaveResources & other) const override;
	};
}

}
