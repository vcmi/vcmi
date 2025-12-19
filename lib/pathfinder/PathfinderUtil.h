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

#include "../mapObjects/CGObjectInstance.h"
#include "../mapping/TerrainTile.h"
#include "../mapping/MapTilesStorage.h"
#include "../callback/IGameInfoCallback.h"
#include "CGPathNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace PathfinderUtil
{
	using FoW = MapTilesStorage<uint8_t>;
	using ELayer = EPathfindingLayer;

	template<EPathfindingLayer::Type layer>
	EPathAccessibility evaluateAccessibility(const int3 & pos, const TerrainTile & tinfo, const FoW & fow, const PlayerColor player, const IGameInfoCallback & gameInfo)
	{
		if(!fow[pos])
			return EPathAccessibility::BLOCKED;

		switch(layer)
		{
		case ELayer::LAND:
		case ELayer::SAIL:
			if(tinfo.visitable())
			{
				if (tinfo.visitableObjects.size() > 1)
				{
					auto frontVisitable = gameInfo.getObjInstance(tinfo.visitableObjects.front());
					auto backVisitable = gameInfo.getObjInstance(tinfo.visitableObjects.back());
					if(frontVisitable->ID == Obj::SANCTUARY && backVisitable->ID == Obj::HERO && backVisitable->getOwner() != player)
					{
						return EPathAccessibility::BLOCKED;
					}
				}
				else
				{
					bool hasBlockedVisitable = false;
					bool hasVisitable = false;

					for(const auto objID : tinfo.visitableObjects)
					{
						auto obj = gameInfo.getObjInstance(objID);

						if(obj->isBlockedVisitable())
							hasBlockedVisitable = true;
						else if(!obj->passableFor(player) && obj->ID != Obj::EVENT)
							hasVisitable = true;
					}

					if(hasBlockedVisitable)
						return EPathAccessibility::BLOCKVIS;
					if(hasVisitable)
						return EPathAccessibility::VISITABLE;

					return EPathAccessibility::ACCESSIBLE;
				}
			}
			else if(tinfo.blocked())
			{
				return EPathAccessibility::BLOCKED;
			}
			else if(gameInfo.guardingCreaturePosition(pos).isValid())
			{
				// Monster close by; blocked visit for battle
				return EPathAccessibility::GUARDED;
			}

			break;

		case ELayer::WATER:
			if(tinfo.blocked() || tinfo.isLand())
				return EPathAccessibility::BLOCKED;

			break;

		case ELayer::AIR:
			return EPathAccessibility::FLYABLE;

			break;
		}

		return EPathAccessibility::ACCESSIBLE;
	}
}

VCMI_LIB_NAMESPACE_END
