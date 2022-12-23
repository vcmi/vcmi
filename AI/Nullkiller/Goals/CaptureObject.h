/*
* CaptureObject.h, part of VCMI engine
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

struct HeroPtr;
class AIGateway;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT CaptureObject : public CGoal<CaptureObject>
	{
	private:
		std::string name;

	public:
		CaptureObject(const CGObjectInstance * obj)
			: CGoal(Goals::CAPTURE_OBJECT)
		{
			objid = obj->id.getNum();
			tile = obj->visitablePos();
			name = obj->getObjectName();
		}

		virtual bool operator==(const CaptureObject & other) const override;
		virtual Goals::TGoalVec decompose() const override;
		virtual std::string toString() const override;
		virtual bool hasHash() const override { return true; }
		virtual uint64_t getHash() const override;
	};
}

}
