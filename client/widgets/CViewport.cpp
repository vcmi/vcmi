/*
 * CViewport.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CViewport.h"

#include "../render/Canvas.h"
#include "../GameEngine.h"
#include "../eventsSDL/InputHandler.h"

CViewport::CViewport(const Rect & viewRect, const Point & contentSz,
                     CSlider::EStyle style)
	: contentSize(contentSz)
	, sliderStyle(style)
{
	addUsedEvents(TIME | GESTURE);
	OBJECT_CONSTRUCTION;

	pos.x += viewRect.x;
	pos.y += viewRect.y;
	pos.w  = viewRect.w;
	pos.h  = viewRect.h;

	// Scroll-triggered redraw() must repaint the parent first so that the
	// background behind the viewport is cleared before we blit new content.
	setRedrawParent(true);

	// Content container.
	contentHolder = std::make_shared<CIntObject>();
	contentHolder->pos.w = contentSz.x;
	contentHolder->pos.h = contentSz.y;

	// Evaluate which sliders are needed and place them correctly.
	// remakeXSlider() is called here for first-time creation too.
	updateSliders();

	// Prevent double-rendering via the default CIntObject child-forwarding loop.
	auto stripRender = [](CIntObject * obj){
		obj->recActions &= static_cast<uint8_t>(~(CIntObject::SHOWALL | CIntObject::UPDATE));
	};
	stripRender(contentHolder.get());
	if(vSlider) stripRender(vSlider.get());
	if(hSlider) stripRender(hSlider.get());
}

CIntObject * CViewport::content() const
{
	return contentHolder.get();
}

// ---------------------------------------------------------------------------
// Slider visibility, sizing & positioning
// ---------------------------------------------------------------------------

void CViewport::remakeVSlider(int length)
{
	const int curVal = vSlider ? vSlider->getValue() : 0;
	// Destroy old slider (its dtor removes it from children).
	vSlider.reset();
	{
		OBJECT_CONSTRUCTION_TARGETED(this);
		vSlider = std::make_shared<CSlider>(
			Point(0, 0),   // positioned by updateSliders
			length,
			[this](int val){ onScrollV(val); },
			length,
			std::max(length + 1, contentSize.y),
			curVal,
			Orientation::VERTICAL,
			sliderStyle);
	}
	vSlider->setScrollStep(20);
	vSlider->setPanningStep(1);
	// This slider's GESTURE events are handled exclusively by CViewport::gesturePanning.
	// Without this, both the slider (via Scrollable::gesturePanning) and the viewport
	// would scroll on every touch-panning event, causing double (over-)scroll speed.
	vSlider->removeUsedEvents(GESTURE);
}

void CViewport::remakeHSlider(int length)
{
	const int curVal = hSlider ? hSlider->getValue() : 0;
	hSlider.reset();
	{
		OBJECT_CONSTRUCTION_TARGETED(this);
		hSlider = std::make_shared<CSlider>(
			Point(0, 0),
			length,
			[this](int val){ onScrollH(val); },
			length,
			std::max(length + 1, contentSize.x),
			curVal,
			Orientation::HORIZONTAL,
			sliderStyle);
	}
	hSlider->setScrollStep(20);
	hSlider->setPanningStep(1);
	// Same reason as vSlider: gesture events are handled by CViewport, not the slider.
	hSlider->removeUsedEvents(GESTURE);
}

void CViewport::updateSliders()
{
	const bool needV = (contentSize.y > pos.h);
	const bool needH = (contentSize.x > pos.w);

	// When both bars are needed the corner (SLIDER_W × SLIDER_W) must stay
	// empty so the two end-buttons don't overlap.  Shorten each bar by
	// SLIDER_W to leave that corner free.
	const int vLen = pos.h - (needH ? SLIDER_W : 0);
	const int hLen = pos.w - (needV ? SLIDER_W : 0);

	// Visible content area after sliders are placed.
	const int clipW = pos.w - (needV ? SLIDER_W : 0);
	const int clipH = pos.h - (needH ? SLIDER_W : 0);

	// --- Vertical ---
	if(needV)
	{
		// Recreate if the required length changed.
		if(vLen != curVLen)
		{
			curVLen = vLen;
			remakeVSlider(vLen);
		}
		vSlider->moveTo(Point(pos.x + clipW, pos.y), true);
		vSlider->setAmount(contentSize.y);
		vSlider->enable();
	}
	else
	{
		if(vSlider) vSlider->disable();
	}

	// --- Horizontal ---
	if(needH)
	{
		if(hLen != curHLen)
		{
			curHLen = hLen;
			remakeHSlider(hLen);
		}
		hSlider->moveTo(Point(pos.x, pos.y + clipH), true);
		hSlider->setAmount(contentSize.x);
		hSlider->enable();
	}
	else
	{
		if(hSlider) hSlider->disable();
	}

	// Allow wheel-scroll over the whole viewport body – but only for the
	// vertical slider.  When both are active the horizontal slider gets only
	// its own strip as scrollBounds so the mouse wheel exclusively scrolls
	// vertically when the cursor is over the content area.
	if(vSlider && !vSlider->isDisabled())
	{
		// Exclude the hSlider strip: use content clip area, not full pos.
		const Rect vScrollArea(pos.x, pos.y, clipW, clipH);
		vSlider->setScrollBounds(vScrollArea - vSlider->pos.topLeft());
	}
	if(hSlider && !hSlider->isDisabled())
	{
		if(needV)
			// Restrict hSlider wheel/gesture to its own strip only.
			hSlider->setScrollBounds(Rect(0, 0, hSlider->pos.w, hSlider->pos.h));
		else
			// Alone: respond everywhere over the viewport.
			hSlider->setScrollBounds(pos - hSlider->pos.topLeft());
	}

	// Strip render bits (enable() resets recActions to ALL_ACTIONS).
	auto stripRender = [](CIntObject * obj){
		obj->recActions &= static_cast<uint8_t>(~(CIntObject::SHOWALL | CIntObject::UPDATE));
	};
	if(vSlider) stripRender(vSlider.get());
	if(hSlider) stripRender(hSlider.get());
}

Rect CViewport::clipRect() const
{
	const bool needV = vSlider && !vSlider->isDisabled();
	const bool needH = hSlider && !hSlider->isDisabled();
	return Rect(pos.x, pos.y,
	            pos.w - (needV ? SLIDER_W : 0),
	            pos.h - (needH ? SLIDER_W : 0));
}

// ---------------------------------------------------------------------------
void CViewport::setContentSize(const Point & sz)
{
	contentSize = sz;
	contentHolder->pos.w = sz.x;
	contentHolder->pos.h = sz.y;

	// Reset scroll to origin.
	scrollX = 0;
	scrollY = 0;
	contentHolder->moveTo(pos.topLeft(), true);

	if(vSlider) vSlider->scrollTo(0, false);
	if(hSlider) hSlider->scrollTo(0, false);

	updateSliders();
	redraw();
}

void CViewport::fitContentSize()
{
	// Compute tight bounding box of all children relative to (0,0) inside
	// the content holder.  Children have absolute screen coordinates after
	// addChild(adjustPosition=true), so subtract contentHolder->pos.topLeft().
	int maxX = 0;
	int maxY = 0;
	const Point origin = contentHolder->pos.topLeft();
	for(auto * child : contentHolder->children)
	{
		const int right  = (child->pos.x - origin.x) + child->pos.w;
		const int bottom = (child->pos.y - origin.y) + child->pos.h;
		if(right  > maxX) maxX = right;
		if(bottom > maxY) maxY = bottom;

		// Ensure every child's redraw() propagates up through the viewport
		// clipping chain instead of drawing directly to the screen canvas.
		// Without this, hover/click state changes and animation ticks would
		// bypass the CanvasClipRectGuard and overdraw outside the viewport.
		child->setRedrawParent(true);
	}
	contentHolder->setRedrawParent(true);

	if(maxX <= 0) maxX = 1;
	if(maxY <= 0) maxY = 1;
	setContentSize(Point(maxX, maxY));
}

void CViewport::scrollToY(int y)
{
	if(vSlider && !vSlider->isDisabled())
		vSlider->scrollTo(std::max(0, y));
}

void CViewport::onScrollV(int val)
{
	const int delta = val - scrollY;
	scrollY = val;
	inertialPosY = scrollY;
	contentHolder->moveBy(Point(0, -delta), true);
	redraw();
}

void CViewport::onScrollH(int val)
{
	const int delta = val - scrollX;
	scrollX = val;
	inertialPosX = scrollX;
	contentHolder->moveBy(Point(-delta, 0), true);
	redraw();
}

// ---------------------------------------------------------------------------
void CViewport::show(Canvas & to)
{
	if(vSlider && !vSlider->isDisabled()) vSlider->show(to);
	if(hSlider && !hSlider->isDisabled()) hSlider->show(to);
	{
		CanvasClipRectGuard clip(to, clipRect());
		contentHolder->show(to);
	}
}

void CViewport::showAll(Canvas & to)
{
	if(vSlider && !vSlider->isDisabled()) vSlider->showAll(to);
	if(hSlider && !hSlider->isDisabled()) hSlider->showAll(to);
	{
		CanvasClipRectGuard clip(to, clipRect());
		contentHolder->showAll(to);
	}
}

bool CViewport::receiveEvent(const Point & position, int eventType) const
{
	if(eventType == GESTURE)
		return clipRect().isInside(position);
	return CIntObject::receiveEvent(position, eventType);
}

void CViewport::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(on)
	{
		smoothH.start();
		smoothV.start();
		inertialPosX = scrollX;
		inertialPosY = scrollY;
	}
	else
	{
		const uint32_t now = ENGINE->input().getTicks();
		smoothH.finish(now);
		smoothV.finish(now);
	}
}

void CViewport::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	const uint32_t ts = ENGINE->input().getTicks();
	const int deltaX = lastUpdateDistance.x;
	const int deltaY = lastUpdateDistance.y;

	smoothH.addStep(deltaX, ts);
	smoothV.addStep(deltaY, ts);

	// Apply live scroll so content actually moves with the finger.
	// Keep inertialPos in sync so post-lift inertia continues from here.
	if(hSlider && !hSlider->isDisabled())
		hSlider->scrollTo(std::max(0, scrollX + deltaX));
	if(vSlider && !vSlider->isDisabled())
		vSlider->scrollTo(std::max(0, scrollY + deltaY));

	inertialPosX = scrollX; // updated by onScrollH callback above
	inertialPosY = scrollY; // updated by onScrollV callback above
}

void CViewport::applyInertialScroll(uint32_t msPassed)
{
	if(isGesturing())
		return;

	inertialPosX += smoothH.tick(msPassed);
	inertialPosY += smoothV.tick(msPassed);

	const int targetX = static_cast<int>(std::round(inertialPosX));
	const int targetY = static_cast<int>(std::round(inertialPosY));

	if(hSlider && !hSlider->isDisabled() && targetX != scrollX)
		hSlider->scrollTo(std::max(0, targetX));

	if(vSlider && !vSlider->isDisabled() && targetY != scrollY)
		vSlider->scrollTo(std::max(0, targetY));
}

void CViewport::tick(uint32_t msPassed)
{
	applyInertialScroll(msPassed);
}
