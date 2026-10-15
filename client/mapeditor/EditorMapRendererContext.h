/*
 * EditorMapRendererContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mapView/MapRendererContext.h"

class CMap;

/// Makes the map renderer draw a map that is opened in the editor, without a running game
class EditorMapRendererContext final : public MapRendererBaseContext
{
	const CMap & map;
	std::set<const CGObjectInstance *> lockedObjects;

public:
	EditorMapRendererContext(const MapRendererContextState & viewState, const CMap & map);

	/// Locked objects are drawn faded
	void setLockedObjects(const std::set<const CGObjectInstance *> & locked);

	int3 getMapSize() const override;
	bool isInMap(const int3 & coordinates) const override;
	bool isVisible(const int3 & coordinates) const override;
	bool isActiveHero(const CGObjectInstance * obj) const override;
	int attackedMonsterDirection(const CGObjectInstance * wanderingMonster) const override;
	const TerrainTile & getMapTile(const int3 & coordinates) const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const override;
	bool showBorder() const override;
	bool showInvisible() const override;
	bool debugOverlaysAllowed() const override;
	int3 grailPosition() const override;
};
