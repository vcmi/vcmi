/*
* VisitTile.h, part of VCMI engine
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
	class DLL_EXPORT VisitTile : public CGoal<VisitTile>
		//tile, in conjunction with hero elementar; assumes tile is reachable
	{
	public:
		VisitTile() {} // empty constructor not allowed

		VisitTile(int3 Tile)
			: CGoal(Goals::VISIT_TILE)
		{
			tile = Tile;
			priority = 5;
		}
		virtual bool operator==(const VisitTile & other) const override;
	};
}
