/*
 * CScrollPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CScrollPanel.h"

#include "Slider.h"
#include "UiTheme.h"

#include "render/Canvas.h"

CScrollPanel::CScrollPanel(const Rect & area)
	: CIntObject(WHEEL | GESTURE)
{
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;

	contentHolder = std::make_shared<CIntObject>();
	contentHolder->pos = Rect(pos.x, pos.y, contentWidth(), pos.h);
	contentHolder->setRedrawParent(true);

	slider = std::make_shared<CSlider>(Point(pos.w - UiTheme::SLIDER_WIDTH, 0), pos.h, [this](int value) { scrollTo(value); },
		pos.h, pos.h, 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setScrollingEnabled(false);
	slider->disable();

	// both are drawn by the panel itself, because the content has to be clipped
	contentHolder->recActions &= static_cast<ui8>(~(SHOWALL | UPDATE));
	slider->recActions &= static_cast<ui8>(~(SHOWALL | UPDATE));
}

CScrollPanel::~CScrollPanel() = default;

CIntObject * CScrollPanel::content() const
{
	return contentHolder.get();
}

int CScrollPanel::contentWidth() const
{
	return pos.w - UiTheme::SLIDER_WIDTH - UiTheme::SPACING;
}

int CScrollPanel::maxScroll() const
{
	return std::max(0, contentHeight - pos.h);
}

void CScrollPanel::setContentHeight(int height)
{
	contentHeight = height;
	contentHolder->pos.h = height;

	if(maxScroll() > 0)
	{
		slider->setAmount(contentHeight);
		slider->enable();
		slider->recActions &= static_cast<ui8>(~(SHOWALL | UPDATE));
		slider->setScrollStep(UiTheme::ROW_HEIGHT);
		slider->setPanningStep(1);
	}
	else
	{
		slider->disable();
	}

	scrollY = std::clamp(scrollY, 0, maxScroll());
	if(maxScroll() > 0)
		slider->scrollTo(scrollY, false);

	applyScroll();
	redraw();
}

int CScrollPanel::getContentHeight() const
{
	return contentHeight;
}

void CScrollPanel::scrollToTop()
{
	scrollTo(0);
	if(maxScroll() > 0)
		slider->scrollTo(0, false);
}

void CScrollPanel::ensureVisible(int top, int bottom)
{
	if(top < scrollY)
		scrollTo(top);
	else if(bottom > scrollY + pos.h)
		scrollTo(bottom - pos.h);

	if(maxScroll() > 0)
		slider->scrollTo(scrollY, false);
}

void CScrollPanel::scrollTo(int y)
{
	scrollY = std::clamp(y, 0, maxScroll());
	applyScroll();
	redraw();
}

void CScrollPanel::applyScroll()
{
	contentHolder->moveTo(Point(pos.x, pos.y - scrollY), true);

	// widgets that stick out of the panel would otherwise react to clicks on whatever is below the panel
	std::set<CIntObject *> alive;
	for(auto * child : contentHolder->children)
	{
		alive.insert(child);

		const bool inside = child->pos.y >= pos.y && child->pos.y + child->pos.h <= pos.y + pos.h;
		const bool wasSuppressed = suppressed.count(child) != 0;

		if(!inside && !wasSuppressed)
		{
			child->setInputEnabled(false);
			suppressed.insert(child);
		}
		else if(inside && wasSuppressed)
		{
			child->setInputEnabled(true);
			suppressed.erase(child);
		}
	}

	vstd::erase_if(suppressed, [&](CIntObject * child) { return alive.count(child) == 0; });
}

void CScrollPanel::wheelScrolled(int distance)
{
	if(maxScroll() == 0)
		return;

	scrollTo(scrollY - distance * UiTheme::ROW_HEIGHT);
	slider->scrollTo(scrollY, false);
}

void CScrollPanel::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	panningAccumulated = 0;
}

void CScrollPanel::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	if(maxScroll() == 0)
		return;

	scrollTo(scrollY - lastUpdateDistance.y);
	slider->scrollTo(scrollY, false);
}

void CScrollPanel::showAll(Canvas & to)
{
	{
		CanvasClipRectGuard guard(to, pos);
		contentHolder->showAll(to);
	}

	if(!slider->isDisabled())
		slider->showAll(to);
}

void CScrollPanel::show(Canvas & to)
{
	{
		CanvasClipRectGuard guard(to, pos);
		contentHolder->show(to);
	}

	if(!slider->isDisabled())
		slider->show(to);
}
