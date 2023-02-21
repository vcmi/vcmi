/*
 * MapViewController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IMapRendererObserver.h"

VCMI_LIB_NAMESPACE_BEGIN
class Point;
struct ObjectPosInfo;
VCMI_LIB_NAMESPACE_END

class MapViewModel;
class MapRendererContext;

/// Class responsible for updating view state,
/// such as its position and any animations
class MapViewController : public IMapObjectObserver
{
	std::shared_ptr<MapRendererContext> context;
	std::shared_ptr<MapViewModel> model;

private:
	// IMapObjectObserver impl
	bool hasOngoingAnimations() override;
	void onObjectFadeIn(const CGObjectInstance * obj) override;
	void onObjectFadeOut(const CGObjectInstance * obj) override;
	void onObjectInstantAdd(const CGObjectInstance * obj) override;
	void onObjectInstantRemove(const CGObjectInstance * obj) override;
	void onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;

public:
	MapViewController(std::shared_ptr<MapRendererContext> context, std::shared_ptr<MapViewModel> model);

	void setViewCenter(const int3 & position);
	void setViewCenter(const Point & position, int level);
	void setTileSize(const Point & tileSize);
	void update(uint32_t timeDelta);

	void setTerrainVisibility(bool showAllTerrain);
	void setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions);

};
