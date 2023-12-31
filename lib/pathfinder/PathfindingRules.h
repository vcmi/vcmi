/*
 * PathfindingRules.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct CDestinationNodeInfo;
struct PathNodeInfo;

class CPathfinderHelper;
class PathfinderConfig;

class IPathfindingRule
{
public:
	virtual ~IPathfindingRule() = default;
	virtual void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const = 0;
};

class DLL_LINKAGE MovementCostRule : public IPathfindingRule
{
public:
	void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE LayerTransitionRule : public IPathfindingRule
{
public:
	void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE DestinationActionRule : public IPathfindingRule
{
public:
	void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE PathfinderBlockingRule : public IPathfindingRule
{
public:
	void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;

protected:
	enum class BlockingReason
	{
		NONE = 0,
		SOURCE_GUARDED = 1,
		DESTINATION_GUARDED = 2,
		SOURCE_BLOCKED = 3,
		DESTINATION_BLOCKED = 4,
		DESTINATION_BLOCKVIS = 5,
		DESTINATION_VISIT = 6
	};

	virtual BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const = 0;
};

class DLL_LINKAGE MovementAfterDestinationRule : public PathfinderBlockingRule
{
public:
	void process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const override;

protected:
	BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const override;
};

class DLL_LINKAGE MovementToDestinationRule : public PathfinderBlockingRule
{
protected:
	BlockingReason getBlockingReason(
		const PathNodeInfo & source,
		const CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) const override;
};

VCMI_LIB_NAMESPACE_END
