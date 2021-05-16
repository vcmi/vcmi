/*
* GetArtOfType.h, part of VCMI engine
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
	class DLL_EXPORT GetArtOfType : public CGoal<GetArtOfType>
	{
	public:
		GetArtOfType()
			: CGoal(Goals::GET_ART_TYPE)
		{
		}
		GetArtOfType(int type)
			: CGoal(Goals::GET_ART_TYPE)
		{
			aid = type;
		}
		virtual bool operator==(const GetArtOfType & other) const override;
	};
}
