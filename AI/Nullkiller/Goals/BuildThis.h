/*
* BuildThis.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"
#include "../Analyzers/BuildAnalyzer.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT BuildThis : public ElementarGoal<BuildThis>
	{
	public:
		BuildingInfo buildingInfo;
		TownDevelopmentInfo townInfo;

		BuildThis() //should be private, but unit test uses it
			: ElementarGoal(Goals::BUILD_STRUCTURE)
		{
		}
		BuildThis(const BuildingInfo & buildingInfo, const TownDevelopmentInfo & townInfo) //should be private, but unit test uses it
			: ElementarGoal(Goals::BUILD_STRUCTURE), buildingInfo(buildingInfo), townInfo(townInfo)
		{
			bid = buildingInfo.id;
			town = townInfo.town;
		}
		BuildThis(BuildingID Bid, const CGTownInstance * tid)
			: ElementarGoal(Goals::BUILD_STRUCTURE)
		{
			bid = Bid;
			town = tid;
			priority = 1;
		}
		BuildThis(BuildingID Bid)
			: ElementarGoal(Goals::BUILD_STRUCTURE)
		{
			bid = Bid;
			priority = 1;
		}
		virtual bool operator==(const BuildThis & other) const override;
		virtual std::string toString() const override;
	};
}
