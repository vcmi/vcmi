/*
 * MapViewActions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/int3.h"
#include "../gui/CIntObject.h"

class IMapRendererContext;
class MapViewModel;
class MapView;

class MapViewActions : public CIntObject
{
	MapView & owner;
	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<IMapRendererContext> context;

	Point dragDistance;
	double pinchZoomFactor;

	void handleHover(const Point & cursorPosition);

public:
	MapViewActions(MapView & owner, const std::shared_ptr<MapViewModel> & model);

	void setContext(const std::shared_ptr<IMapRendererContext> & context);

	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesturePinch(const Point & centerPosition, double lastUpdateFactor) override;
	void hover(bool on) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void wheelScrolled(int distance) override;
	
	bool dragActive;
};
