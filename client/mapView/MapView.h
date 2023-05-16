/*
 * MapView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN
struct ObjectPosInfo;
VCMI_LIB_NAMESPACE_END

class Canvas;
class MapViewActions;
class MapViewController;
class MapViewModel;
class MapViewCache;

/// Internal class that contains logic shared between all map views
class BasicMapView : public CIntObject
{
protected:
	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<MapViewCache> tilesCache;
	std::shared_ptr<MapViewController> controller;

	std::shared_ptr<MapViewModel> createModel(const Point & dimensions) const;

	void render(Canvas & target, bool fullUpdate);

public:
	BasicMapView(const Point & offset, const Point & dimensions);
	~BasicMapView() override;

	void tick(uint32_t msPassed) override;
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

/// Main class that represents visible section of adventure map
/// Contains all public interface of view and translates calls to internal model
class MapView : public BasicMapView
{
	std::shared_ptr<MapViewActions> actions;

	bool isSwiping;

public:
	void show(SDL_Surface * to) override;

	MapView(const Point & offset, const Point & dimensions);

	/// Moves current view to another level, preserving position
	void onMapLevelSwitched();

	/// Moves current view by specified distance in pixels
	void onMapScrolled(const Point & distance);

	/// Moves current view to specified position, in pixels
	void onMapSwiped(const Point & viewPosition);

	/// Ends swiping mode and allows normal map scrolling once again
	void onMapSwipeEnded();

	/// Moves current view to specified tile
	void onCenteredTile(const int3 & tile);

	/// Moves current view to specified object
	void onCenteredObject(const CGObjectInstance * target);

	/// Switches view to "View Earth" / "View Air" mode, displaying downscaled map with overlay
	void onViewSpellActivated(uint32_t tileSize, const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain);

	/// Switches view to downscaled View World
	void onViewWorldActivated(uint32_t tileSize);

	/// Changes zoom level / tile size of current view by specified factor
	void onMapZoomLevelChanged(double zoomFactor);

	/// Switches view from View World mode back to standard view
	void onViewMapActivated();
};

/// Main class that represents map view for puzzle map
class PuzzleMapView : public BasicMapView
{
public:
	PuzzleMapView(const Point & offset, const Point & dimensions, const int3 & tileToCenter);
};
