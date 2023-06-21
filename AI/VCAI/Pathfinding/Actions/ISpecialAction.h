/*
* ISpecialAction.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../../AIUtility.h"
#include "../../Goals/AbstractGoal.h"

struct AIPathNode;
struct PathNodeInfo;
struct CDestinationNodeInfo;

class ISpecialAction
{
public:
	virtual ~ISpecialAction() = default;
	virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const = 0;

	virtual void applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
	}
};
