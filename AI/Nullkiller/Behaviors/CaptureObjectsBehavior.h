/*
* CaptureObjectsBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/VCMI_Lib.h"
#include "../AIUtility.h"
#include "../Goals/CGoal.h"

namespace Goals
{
	class CaptureObjectsBehavior : public CGoal<CaptureObjectsBehavior>
	{
	private:
		std::vector<int> objectTypes;
		std::vector<int> objectSubTypes;
		std::vector<const CGObjectInstance *> objectsToCapture;
		bool specificObjects;
	public:
		CaptureObjectsBehavior()
		{
			objectTypes = std::vector<int>();
			specificObjects = false;
		}

		CaptureObjectsBehavior(std::vector<const CGObjectInstance *> objectsToCapture)
		{
			this->objectsToCapture = objectsToCapture;
			specificObjects = true;
		}

		CaptureObjectsBehavior(const CGObjectInstance * objectToCapture)
		{
			objectsToCapture = std::vector<const CGObjectInstance *>();
			objectsToCapture.push_back(objectToCapture);
			specificObjects = true;
		}

		virtual Goals::TGoalVec decompose() const override;
		virtual std::string toString() const override;

		CaptureObjectsBehavior & ofType(int type)
		{
			objectTypes.push_back(type);

			return *this;
		}
		CaptureObjectsBehavior & ofType(int type, int subType)
		{
			objectTypes.push_back(type);
			objectSubTypes.push_back(subType);

			return *this;
		}

		virtual bool operator==(const CaptureObjectsBehavior & other) const override
		{
			return false;
		}

	private:
		bool shouldVisitObject(ObjectIdRef obj) const;
	};
}

