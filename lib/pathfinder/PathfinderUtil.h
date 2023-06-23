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

#include "../TerrainHandler.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapping/CMapDefines.h"
#include "../gameState/CGameState.h"
#include "CGPathNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace PathfinderUtil
{
	using FoW = std::shared_ptr<const boost::multi_array<ui8, 3>>;
	using ELayer = EPathfindingLayer;

	template<EPathfindingLayer::EEPathfindingLayer layer>
	EPathAccessibility evaluateAccessibility(const int3 & pos, const TerrainTile & tinfo, FoW fow, const PlayerColor player, const CGameState * gs)
	{
		if(!(*fow)[pos.z][pos.x][pos.y])
			return EPathAccessibility::BLOCKED;

		switch(layer)
		{
		case ELayer::LAND:
		case ELayer::SAIL:
			if(tinfo.visitable)
			{
				if(tinfo.visitableObjects.front()->ID == Obj::SANCTUARY && tinfo.visitableObjects.back()->ID == Obj::HERO && tinfo.visitableObjects.back()->tempOwner != player) //non-owned hero stands on Sanctuary
				{
					return EPathAccessibility::BLOCKED;
				}
				else
				{
					for(const CGObjectInstance * obj : tinfo.visitableObjects)
					{
						if(obj->blockVisit)
							return EPathAccessibility::BLOCKVIS;
						else if(obj->passableFor(player))
							return EPathAccessibility::ACCESSIBLE;
						else if(obj->ID != Obj::EVENT)
							return EPathAccessibility::VISITABLE;
					}
				}
			}
			else if(tinfo.blocked)
			{
				return EPathAccessibility::BLOCKED;
			}
			else if(gs->guardingCreaturePosition(pos).valid())
			{
				// Monster close by; blocked visit for battle
				return EPathAccessibility::BLOCKVIS;
			}

			break;

		case ELayer::WATER:
			if(tinfo.blocked || tinfo.terType->isLand())
				return EPathAccessibility::BLOCKED;

			break;

		case ELayer::AIR:
			if(tinfo.blocked || tinfo.terType->isLand())
				return EPathAccessibility::FLYABLE;

			break;
		}

		return EPathAccessibility::ACCESSIBLE;
	}
}

VCMI_LIB_NAMESPACE_END
