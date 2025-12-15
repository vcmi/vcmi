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

VCMI_LIB_NAMESPACE_BEGIN
struct PathNodeInfo;
struct CDestinationNodeInfo;
VCMI_LIB_NAMESPACE_END

namespace NK2AI
{

struct AIPathNode;
struct AIPathNodeInfo;
class ChainActor;

class SpecialAction
{
public:
	virtual ~SpecialAction() = default;

	virtual bool canAct(const Nullkiller * aiNk, const AIPathNode * source) const
	{
		return true;
	}

	virtual bool canAct(const Nullkiller * aiNk, const AIPathNodeInfo & source) const
	{
		return true;
	}

	virtual Goals::TSubgoal decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const;

	virtual void execute(AIGateway * aiGw, const CGHeroInstance * hero) const;

	virtual void applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstNode,
		const AIPathNode * srcNode) const
	{
	}

	virtual std::string toString() const = 0;

	virtual const CGObjectInstance * targetObject() const { return nullptr; }

	virtual std::vector<std::shared_ptr<const SpecialAction>> getParts() const
	{
		return {};
	}

	virtual const ChainActor * getActor(const ChainActor * sourceActor) const
	{
		return sourceActor;
	}
};

class CompositeAction : public SpecialAction
{
private:
	std::vector<std::shared_ptr<const SpecialAction>> parts;

public:
	CompositeAction(std::vector<std::shared_ptr<const SpecialAction>> parts) : parts(parts) {}

	bool canAct(const Nullkiller * aiNk, const AIPathNode * source) const override;
	void execute(AIGateway * aiGw, const CGHeroInstance * hero) const override;
	std::string toString() const override;
	const CGObjectInstance * targetObject() const override;
	Goals::TSubgoal decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const override;

	std::vector<std::shared_ptr<const SpecialAction>> getParts() const override
	{
		return parts;
	}

	void applyOnDestination(
		const CGHeroInstance * hero,
		CDestinationNodeInfo & destination,
		const PathNodeInfo & source,
		AIPathNode * dstNode,
		const AIPathNode * srcNode) const override;
};

class ISpecialActionFactory
{
public:
	virtual std::shared_ptr<SpecialAction> create(const Nullkiller * aiNk) = 0;
	virtual ~ISpecialActionFactory() = default;
};

}
