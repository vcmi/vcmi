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

#include "lib/VCMI_Lib.h"
#include "lib/CBuildingHandler.h"
#include "lib/CCreatureHandler.h"
#include "lib/CTownHandler.h"
#include "../AIUtility.h"
#include "../VCAI.h"
#include "Task.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Tasks
{
	class VisitTile : public TemplateTask<VisitTile> {
	private:
		std::string objInfo;
	public:
		VisitTile(int3 tile, HeroPtr hero, const CGObjectInstance* obj = NULL) {
			this->tile = tile;
			this->hero = hero;

			if (obj) {
				objInfo = obj->getObjectName();
			}
		}

		virtual void execute() override;
		virtual bool canExecute() override;
		virtual std::string toString() override;
	};
}