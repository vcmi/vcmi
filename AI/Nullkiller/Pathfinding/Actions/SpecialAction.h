/*
* SpecialAction.h, part of VCMI engine
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

class SpecialAction
{
public:
	virtual bool canAct(const AIPathNode * source) const
	{
		return true;
	}

	virtual Goals::TSubgoal decompose(const CGHeroInstance * hero) const;

	virtual void execute(const CGHeroInstance * hero) const;

	virtual void applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstMode,
		const AIPathNode * srcNode) const
	{
	}

	virtual std::string toString() const = 0;

	virtual const CGObjectInstance * targetObject() const { return nullptr; }
};