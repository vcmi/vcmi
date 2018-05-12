/*
* Goals.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "AIUtility.h"
#include "Task.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Tasks
{
	class BuildStructure : public TemplateTask<BuildStructure> {
	public:
		BuildingID buildingID;
		const CGTownInstance* town;

		BuildStructure(BuildingID buildingID, const CGTownInstance* town) {
			this->town = town;
			this->buildingID = buildingID;
		}

		virtual void execute() override;
		virtual std::string toString() override;
	};
}