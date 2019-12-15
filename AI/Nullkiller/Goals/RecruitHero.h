/*
* RecruitHero.h, part of VCMI engine
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
	class DLL_EXPORT RecruitHero : public CGoal<RecruitHero>
	{
	public:
		RecruitHero()
			: CGoal(Goals::RECRUIT_HERO)
		{
			priority = 1;
		}

		virtual bool operator==(const RecruitHero & other) const override
		{
			return true;
		}
	};
}
