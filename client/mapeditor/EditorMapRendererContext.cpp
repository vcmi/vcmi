/*
 * EditorMapRendererContext.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMapRendererContext.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/TerrainTile.h"

namespace
{
constexpr double LOCKED_OBJECT_OPACITY = 0.4;
}

EditorMapRendererContext::EditorMapRendererContext(const MapRendererContextState & viewState, const CMap & map)
	: MapRendererBaseContext(viewState)
	, map(map)
{
}

void EditorMapRendererContext::setLockedObjects(const std::set<const CGObjectInstance *> & locked)
{
	lockedObjects = locked;
}

int3 EditorMapRendererContext::getMapSize() const
{
	return int3(map.width, map.height, map.levels());
}

bool EditorMapRendererContext::isInMap(const int3 & coordinates) const
{
	return map.isInTheMap(coordinates);
}

bool EditorMapRendererContext::isVisible(const int3 & coordinates) const
{
	return map.isInTheMap(coordinates);
}

bool EditorMapRendererContext::isActiveHero(const CGObjectInstance * obj) const
{
	return false;
}

int EditorMapRendererContext::attackedMonsterDirection(const CGObjectInstance * wanderingMonster) const
{
	return -1;
}

const TerrainTile & EditorMapRendererContext::getMapTile(const int3 & coordinates) const
{
	return map.getTile(coordinates);
}

const CGObjectInstance * EditorMapRendererContext::getObject(ObjectInstanceID objectID) const
{
	return map.getObject(objectID);
}

double EditorMapRendererContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
{
	const double base = MapRendererBaseContext::objectTransparency(objectID, coordinates);

	if(lockedObjects.count(map.getObject(objectID)))
		return base * LOCKED_OBJECT_OPACITY;

	return base;
}

bool EditorMapRendererContext::showBorder() const
{
	return true;
}

bool EditorMapRendererContext::showInvisible() const
{
	return true;
}

bool EditorMapRendererContext::debugOverlaysAllowed() const
{
	return true;
}

int3 EditorMapRendererContext::grailPosition() const
{
	return int3(-1, -1, -1);
}
