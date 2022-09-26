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

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT SaveResources : public ElementarGoal<SaveResources>
	{
	private:
		TResources resources;

	public:
		SaveResources(TResources resources)
			: ElementarGoal(Goals::SAVE_RESOURCES), resources(resources)
		{
		}

		void accept(AIGateway * ai) override;
		std::string toString() const override;
		virtual bool operator==(const SaveResources & other) const override;
	};
}

}
