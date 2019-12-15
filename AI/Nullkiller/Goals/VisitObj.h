/*
* VisitObj.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT VisitObj : public CGoal<VisitObj> //this goal was previously known as GetObj
	{
	public:
		VisitObj() = delete; // empty constructor not allowed
		VisitObj(int Objid);

		virtual bool operator==(const VisitObj & other) const override;
	};
}
