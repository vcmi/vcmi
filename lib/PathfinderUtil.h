/*
 * PathfinderUtil.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "mapping/CMapDefines.h"
#include "CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace PathfinderUtil
{
	using FoW = std::shared_ptr<const boost::multi_array<ui8, 3>>;
	using ELayer = EPathfindingLayer;

	template<EPathfindingLayer::EEPathfindingLayer layer>
	CGPathNode::EAccessibility evaluateAccessibility(const int3 & pos, const TerrainTile * tinfo, FoW fow, const PlayerColor player, const CGameState * gs)
	{
		if(!(*fow)[pos.z][pos.x][pos.y])
			return CGPathNode::BLOCKED;

		switch(layer)
		{
		case ELayer::LAND:
		case ELayer::SAIL:
			if(tinfo->visitable)
			{
				if(tinfo->visitableObjects.front()->ID == Obj::SANCTUARY && tinfo->visitableObjects.back()->ID == Obj::HERO && tinfo->visitableObjects.back()->tempOwner != player) //non-owned hero stands on Sanctuary
				{
					return CGPathNode::BLOCKED;
				}
				else
				{
					for(const CGObjectInstance * obj : tinfo->visitableObjects)
					{
						if(obj->blockVisit)
							return CGPathNode::BLOCKVIS;
						else if(obj->passableFor(player))
							return CGPathNode::ACCESSIBLE;
						else if(obj->ID != Obj::EVENT)
							return CGPathNode::VISITABLE;
					}
				}
			}
			else if(tinfo->blocked)
			{
				return CGPathNode::BLOCKED;
			}
			else if(gs->guardingCreaturePosition(pos).valid())
			{
				// Monster close by; blocked visit for battle
				return CGPathNode::BLOCKVIS;
			}

			break;

		case ELayer::WATER:
			if(tinfo->blocked || tinfo->terType.isLand())
				return CGPathNode::BLOCKED;

			break;

		case ELayer::AIR:
			if(tinfo->blocked || tinfo->terType.isLand())
				return CGPathNode::FLYABLE;

			break;
		}

		return CGPathNode::ACCESSIBLE;
	}
}

VCMI_LIB_NAMESPACE_END
