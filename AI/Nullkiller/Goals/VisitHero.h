/*
* VisitHero.h, part of VCMI engine
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
	class DLL_EXPORT VisitHero : public CGoal<VisitHero>
	{
	public:
		VisitHero()
			: CGoal(Goals::VISIT_HERO)
		{
		}
		VisitHero(int hid)
			: CGoal(Goals::VISIT_HERO)
		{
			objid = hid;
			priority = 4;
		}
		virtual bool operator==(const VisitHero & other) const override;
	};
}
