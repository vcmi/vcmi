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
	bool isSwiping;
	bool isPostSwiping;
	int32_t postSwipeCounter;

	Point swipeInitialViewPos;
	Point swipeInitialRealPos;
	Point swipeDelta;
	Point cursorPositionLast[5];

	MapView & owner;
	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<IMapRendererContext> context;
	
	uint32_t timerCounter;
	bool timerRunning;

	void handleHover(const Point & cursorPosition);
	void handleSwipeMove(const Point & cursorPosition);
	bool handleSwipeStateChange(bool btnPressed);
	bool swipeEnabled() const;
	void tick(uint32_t msPassed) override;
	void startTimer();
	void trackCursor(bool init);
	void handlePostSwipe();

public:
	MapViewActions(MapView & owner, const std::shared_ptr<MapViewModel> & model);

	void setContext(const std::shared_ptr<IMapRendererContext> & context);

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void clickMiddle(tribool down, bool previousState) override;
	void hover(bool on) override;
	void mouseMoved(const Point & cursorPosition) override;
	void wheelScrolled(bool down, bool in) override;
};
