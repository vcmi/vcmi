/*
 * INodeStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CDestinationNodeInfo;
struct CGPathNode;
struct PathfinderOptions;
struct PathNodeInfo;

class CGameState;
class CPathfinderHelper;
class PathfinderConfig;

class DLL_LINKAGE INodeStorage
{
public:
	using ELayer = EPathfindingLayer;

	virtual ~INodeStorage() = default;

	virtual std::vector<CGPathNode *> getInitialNodes() = 0;

	virtual std::vector<CGPathNode *> calculateNeighbours(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) = 0;

	virtual std::vector<CGPathNode *> calculateTeleportations(
		const PathNodeInfo & source,
		const PathfinderConfig * pathfinderConfig,
		const CPathfinderHelper * pathfinderHelper) = 0;

	virtual void commit(CDestinationNodeInfo & destination, const PathNodeInfo & source) = 0;

	virtual void initialize(const PathfinderOptions & options, const CGameState * gs) = 0;
};

VCMI_LIB_NAMESPACE_END
