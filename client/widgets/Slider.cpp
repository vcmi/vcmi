/*
 * Slider.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Slider.h"

#include "Buttons.h"

#include "../gui/MouseButton.h"
#include "../gui/Shortcut.h"
#include "../gui/CGuiHandler.h"
#include "../render/Canvas.h"

void CSlider::sliderClicked()
{
	addUsedEvents(MOVE);
}

void CSlider::mouseMoved (const Point & cursorPosition)
{
	double v = 0;
	if(horizontal)
	{
		if(	std::abs(cursorPosition.y-(pos.y+pos.h/2)) > pos.h/2+40  ||  std::abs(cursorPosition.x-(pos.x+pos.w/2)) > pos.w/2  )
			return;
		v = cursorPosition.x - pos.x - 24;
		v *= positions;
		v /= (pos.w - 48);
	}
	else
	{
		if(std::abs(cursorPosition.x-(pos.x+pos.w/2)) > pos.w/2+40  ||  std::abs(cursorPosition.y-(pos.y+pos.h/2)) > pos.h/2  )
			return;
		v = cursorPosition.y - pos.y - 24;
		v *= positions;
		v /= (pos.h - 48);
	}
	v += 0.5;
	if(v!=value)
	{
		moveTo(static_cast<int>(v));
	}
}

void CSlider::setScrollStep(int to)
{
	scrollStep = to;
}

void CSlider::setScrollBounds(const Rect & bounds )
{
	scrollBounds = bounds;
}

void CSlider::clearScrollBounds()
{
	scrollBounds = std::nullopt;
}

int CSlider::getAmount() const
{
	return amount;
}

int CSlider::getValue() const
{
	return value;
}

int CSlider::getCapacity() const
{
	return capacity;
}

void CSlider::moveLeft()
{
	moveTo(value-1);
}

void CSlider::moveRight()
{
	moveTo(value+1);
}

void CSlider::moveBy(int amount)
{
	moveTo(value + amount);
}

void CSlider::updateSliderPos()
{
	if(horizontal)
	{
		if(positions)
		{
			double part = static_cast<double>(value) / positions;
			part*=(pos.w-48);
			int newPos = static_cast<int>(part + pos.x + 16 - slider->pos.x);
			slider->moveBy(Point(newPos, 0));
		}
		else
			slider->moveTo(Point(pos.x+16, pos.y));
	}
	else
	{
		if(positions)
		{
			double part = static_cast<double>(value) / positions;
			part*=(pos.h-48);
			int newPos = static_cast<int>(part + pos.y + 16 - slider->pos.y);
			slider->moveBy(Point(0, newPos));
		}
		else
			slider->moveTo(Point(pos.x, pos.y+16));
	}
}

void CSlider::moveTo(int to)
{
	vstd::amax(to, 0);
	vstd::amin(to, positions);

	//same, old position?
	if(value == to)
		return;
	value = to;

	updateSliderPos();

	moved(to);
}

void CSlider::clickLeft(tribool down, bool previousState)
{
	if(down && !slider->isBlocked())
	{
		double pw = 0;
		double rw = 0;
		if(horizontal)
		{
			pw = GH.getCursorPosition().x-pos.x-25;
			rw = pw / static_cast<double>(pos.w - 48);
		}
		else
		{
			pw = GH.getCursorPosition().y-pos.y-24;
			rw = pw / (pos.h-48);
		}
		if(pw < -8  ||  pw > (horizontal ? pos.w : pos.h) - 40)
			return;
		// 		if (rw>1) return;
		// 		if (rw<0) return;
		slider->clickLeft(true, slider->isMouseButtonPressed(MouseButton::LEFT));
		moveTo((int)(rw * positions  +  0.5));
		return;
	}
	removeUsedEvents(MOVE);
}

bool CSlider::receiveEvent(const Point &position, int eventType) const
{
	if (eventType != WHEEL && eventType != GESTURE_PANNING)
	{
		return CIntObject::receiveEvent(position, eventType);
	}

	if (!scrollBounds)
		return true;

	Rect testTarget = *scrollBounds + pos.topLeft();

	return testTarget.isInside(position);
}

void CSlider::setPanningStep(int to)
{
	panningDistanceSingle = to;
}

void CSlider::panning(bool on)
{
	panningDistanceAccumulated = 0;
}

void CSlider::gesturePanning(const Point & distanceDelta)
{
	if (horizontal)
		panningDistanceAccumulated += -distanceDelta.x;
	else
		panningDistanceAccumulated += distanceDelta.y;

	if (-panningDistanceAccumulated > panningDistanceSingle )
	{
		int scrollAmount = (-panningDistanceAccumulated) / panningDistanceSingle;
		moveBy(-scrollAmount);
		panningDistanceAccumulated += scrollAmount * panningDistanceSingle;
	}

	if (panningDistanceAccumulated > panningDistanceSingle )
	{
		int scrollAmount = panningDistanceAccumulated / panningDistanceSingle;
		moveBy(scrollAmount);
		panningDistanceAccumulated += -scrollAmount * panningDistanceSingle;
	}
}

CSlider::CSlider(Point position, int totalw, std::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, CSlider::EStyle style)
	: CIntObject(LCLICK | RCLICK | WHEEL | GESTURE_PANNING ),
	capacity(Capacity),
	horizontal(Horizontal),
	amount(Amount),
	value(Value),
	scrollStep(1),
	moved(Moved),
	panningDistanceAccumulated(0),
	panningDistanceSingle(32)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	setAmount(amount);
	vstd::amax(value, 0);
	vstd::amin(value, positions);

	pos.x += position.x;
	pos.y += position.y;

	if(style == BROWN)
	{
		std::string name = horizontal ? "IGPCRDIV.DEF" : "OVBUTN2.DEF";
		//NOTE: this images do not have "blocked" frames. They should be implemented somehow (e.g. palette transform or something...)

		left = std::make_shared<CButton>(Point(), name, CButton::tooltip());
		right = std::make_shared<CButton>(Point(), name, CButton::tooltip());
		slider = std::make_shared<CButton>(Point(), name, CButton::tooltip());

		left->setImageOrder(0, 1, 1, 1);
		right->setImageOrder(2, 3, 3, 3);
		slider->setImageOrder(4, 4, 4, 4);
	}
	else
	{
		left = std::make_shared<CButton>(Point(), horizontal ? "SCNRBLF.DEF" : "SCNRBUP.DEF", CButton::tooltip());
		right = std::make_shared<CButton>(Point(), horizontal ? "SCNRBRT.DEF" : "SCNRBDN.DEF", CButton::tooltip());
		slider = std::make_shared<CButton>(Point(), "SCNRBSL.DEF", CButton::tooltip());
	}
	slider->actOnDown = true;
	slider->soundDisabled = true;
	left->soundDisabled = true;
	right->soundDisabled = true;

	if (horizontal)
		right->moveBy(Point(totalw - right->pos.w, 0));
	else
		right->moveBy(Point(0, totalw - right->pos.h));

	left->addCallback(std::bind(&CSlider::moveLeft,this));
	right->addCallback(std::bind(&CSlider::moveRight,this));
	slider->addCallback(std::bind(&CSlider::sliderClicked,this));

	if(horizontal)
	{
		pos.h = slider->pos.h;
		pos.w = totalw;
	}
	else
	{
		pos.w = slider->pos.w;
		pos.h = totalw;
	}

	updateSliderPos();
}

CSlider::~CSlider() = default;

void CSlider::block( bool on )
{
	left->block(on);
	right->block(on);
	slider->block(on);
}

void CSlider::setAmount( int to )
{
	amount = to;
	positions = to - capacity;
	vstd::amax(positions, 0);
}

void CSlider::showAll(Canvas & to)
{
	to.drawColor(pos, Colors::BLACK);
	CIntObject::showAll(to);
}

void CSlider::wheelScrolled(int distance)
{
	// vertical slider -> scrolling up move slider upwards
	// horizontal slider -> scrolling up moves slider towards right
	bool positive = ((distance < 0) != horizontal);

	moveTo(value + 3 * (positive ? +scrollStep : -scrollStep));
}

void CSlider::keyPressed(EShortcut key)
{
	int moveDest = value;
	switch(key)
	{
	case EShortcut::MOVE_UP:
		if (!horizontal)
			moveDest = value - scrollStep;
		break;
	case EShortcut::MOVE_LEFT:
		if (horizontal)
			moveDest = value - scrollStep;
		break;
	case EShortcut::MOVE_DOWN:
		if (!horizontal)
			moveDest = value + scrollStep;
		break;
	case EShortcut::MOVE_RIGHT:
		if (horizontal)
			moveDest = value + scrollStep;
		break;
	case EShortcut::MOVE_PAGE_UP:
		moveDest = value - capacity + scrollStep;
		break;
	case EShortcut::MOVE_PAGE_DOWN:
		moveDest = value + capacity - scrollStep;
		break;
	case EShortcut::MOVE_FIRST:
		moveDest = 0;
		break;
	case EShortcut::MOVE_LAST:
		moveDest = amount - capacity;
		break;
	default:
		return;
	}

	moveTo(moveDest);
}

void CSlider::moveToMin()
{
	moveTo(0);
}

void CSlider::moveToMax()
{
	moveTo(amount);
}
