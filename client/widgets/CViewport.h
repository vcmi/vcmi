/*
 * CViewport.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../gui/CIntObject.h"
#include "Slider.h"
#include "Scrollable.h"

/**
 * Generic scrollable viewport widget.
 *
 * Clips rendering of its scrollable content area to the visible viewport rect
 * and shows CSlider scrollbars only when the content is larger than the view.
 *
 * Scrollbars sit **inside** the viewRect (reducing the visible content area
 * when they are shown):
 *   • vertical   – right  edge,  SLIDER_W pixels wide
 *   • horizontal – bottom edge,  SLIDER_W pixels tall
 * They are hidden (disabled) when the content fits without scrolling.
 *
 * Usage:
 *   auto vp = std::make_shared<CViewport>(Rect(x, y, w, h), Point(cw, ch));
 *   OBJECT_CONSTRUCTION_TARGETED(vp->content());
 *   auto btn = std::make_shared<CButton>(...);  // positioned relative to (0,0)
 *   vp->fitContentSize();  // optional: shrink content area to children bounding box
 */
class CViewport : public CIntObject
{
public:
	static constexpr int SLIDER_W = 16; ///< thickness of each scrollbar in px

private:
	Point           contentSize;          ///< total declared content dimensions
	int             scrollX = 0;          ///< current horizontal scroll offset (px)
	int             scrollY = 0;          ///< current vertical scroll offset (px)
	int             curVLen = 0;          ///< current vSlider track length (for change detection)
	int             curHLen = 0;          ///< current hSlider track length (for change detection)

	std::shared_ptr<CIntObject> contentHolder; ///< container for scrollable children
	std::shared_ptr<CSlider>    vSlider;       ///< vertical scrollbar (disabled when unneeded)
	std::shared_ptr<CSlider>    hSlider;       ///< horizontal scrollbar (disabled when unneeded)
	CSlider::EStyle             sliderStyle;

	/// Create/recreate the vertical scrollbar with the given track length.
	void remakeVSlider(int length);
	/// Create/recreate the horizontal scrollbar with the given track length.
	void remakeHSlider(int length);
	void onScrollV(int val);
	void onScrollH(int val);
	void updateSliders();                 ///< (re-)evaluate slider visibility, position & range

	SmoothScrollHelper smoothH; ///< horizontal inertia
	SmoothScrollHelper smoothV; ///< vertical inertia
	double inertialPosX = 0.0; ///< fractional scroll X for sub-pixel accumulation
	double inertialPosY = 0.0; ///< fractional scroll Y for sub-pixel accumulation

	void applyInertialScroll(uint32_t msPassed);

protected:
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	void tick(uint32_t msPassed) override;

	bool receiveEvent(const Point & position, int eventType) const override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;

public:
	CViewport(const Rect & viewRect, const Point & contentSz,
	          CSlider::EStyle style = CSlider::BROWN);

	Rect clipRect() const;               ///< visible content area (shrunk by active sliders)

	/// Returns the scrollable content container.
	CIntObject * content() const;

	/// Updates the total content size and re-evaluates scrollbar visibility.
	/// Resets the scroll position to (0, 0).
	void setContentSize(const Point & sz);

	/// Sets content size to tight bounding box of all children, then
	/// re-evaluates scrollbars.  Call after populating via content().
	void fitContentSize();

	/// Scrolls the viewport so that content at vertical pixel offset @p y
	/// is visible at the top of the viewport.  Does nothing if there is no
	/// vertical slider or the content fits without scrolling.
	void scrollToY(int y);
};
