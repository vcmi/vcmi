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

class MapViewController;
class MapViewModel;
class MapViewCache;

/// Main map rendering class that mostly acts as container for component classes
class MapView : public CIntObject
{
	std::shared_ptr<MapViewModel> model;
	std::unique_ptr<MapViewCache> tilesCache;
	std::shared_ptr<MapViewController> controller;

	std::shared_ptr<MapViewModel> createModel(const Point & dimensions) const;

public:
	std::shared_ptr<const MapViewModel> getModel() const;
	std::shared_ptr<MapViewController> getController();

	MapView(const Point & offset, const Point & dimensions);
	~MapView() override;

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};
