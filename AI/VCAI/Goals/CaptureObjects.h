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
#include "../Tasks/Task.h"
#include "Goal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Goals
{
class CaptureObjects : public CGoal<CaptureObjects> {
private:
	std::vector<int> objectTypes;
	std::vector<const CGObjectInstance*> objectsToCapture;
	bool specificObjects;
public:
	CaptureObjects()
		: CGoal(Goals::CAPTURE_OBJECTS) {
		objectTypes = std::vector<int>();
		specificObjects = false;
	}
	CaptureObjects(std::vector<const CGObjectInstance*> objectsToCapture)
		: CGoal(Goals::CAPTURE_OBJECTS) {
		this->objectsToCapture = objectsToCapture;
		specificObjects = true;
	}

	virtual Tasks::TaskList getTasks() override;
	virtual std::string toString() const override;
	CaptureObjects& ofType(int type) {
		objectTypes.push_back(type);

		return *this;
	}

protected:
	virtual bool shouldVisitObject(ObjectIdRef obj, HeroPtr hero, SectorMap& sm);
};
}

