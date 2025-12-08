/*
* SpecialAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "SpecialAction.h"
#include "../../AIGateway.h"
#include "../../Goals/CGoal.h"
#include "../../Goals/Invalid.h"

namespace NK2AI
{

Goals::TSubgoal SpecialAction::decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const
{
	return Goals::sptr(Goals::Invalid());
}

void SpecialAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
{
	throw cannotFulfillGoalException("Can not execute " + toString());
}

bool CompositeAction::canAct(const Nullkiller * aiNk, const AIPathNode * source) const
{
	for(auto part : parts)
	{
		if(!part->canAct(aiNk, source)) return false;
	}

	return true;
}

Goals::TSubgoal CompositeAction::decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const
{
	for(auto part : parts)
	{
		auto goal = part->decompose(aiNk, hero);

		if(!goal->invalid()) return goal;
	}

	return SpecialAction::decompose(aiNk, hero);
}

void CompositeAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
{
	for(auto part : parts)
	{
		part->execute(aiGw, hero);
	}
}

void CompositeAction::applyOnDestination(
	const CGHeroInstance * hero,
	CDestinationNodeInfo & destination,
	const PathNodeInfo & source,
	AIPathNode * dstNode,
	const AIPathNode * srcNode) const
{
	for(auto part : parts)
	{
		part->applyOnDestination(hero, destination, source, dstNode, srcNode);
	}
}

std::string CompositeAction::toString() const
{
	std::string result = "";

	for(auto part : parts)
	{
		result += ", " + part->toString();
	}

	return result;
}

const CGObjectInstance * CompositeAction::targetObject() const
{
	if(parts.empty())
		return nullptr;

	return parts.front()->targetObject();
}

}
