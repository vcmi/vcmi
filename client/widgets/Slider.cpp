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
#include "../render/Colors.h"

void CSlider::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	double newPosition = 0;
	if(getOrientation() == Orientation::HORIZONTAL)
	{
		newPosition = cursorPosition.x - pos.x - 24;
		newPosition *= positions;
		newPosition /= (pos.w - 48);
	}
	else
	{
		newPosition = cursorPosition.y - pos.y - 24;
		newPosition *= positions;
		newPosition /= (pos.h - 48);
	}

	int positionInteger = std::round(newPosition);
	if(positionInteger != value)
	{
		scrollTo(static_cast<int>(newPosition));
	}
}

void CSlider::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	if (getOrientation() == Orientation::VERTICAL)
		Scrollable::gesturePanning(initialPosition, currentPosition, lastUpdateDistance);
	else
		mouseDragged(currentPosition, lastUpdateDistance);
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

void CSlider::setValue(int to, bool callCallbacks)
{
	scrollTo(value);
}

int CSlider::getCapacity() const
{
	return capacity;
}

void CSlider::scrollBy(int amount)
{
	scrollTo(value + amount);
}

void CSlider::updateSliderPos()
{
	if(getOrientation() == Orientation::HORIZONTAL)
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

void CSlider::scrollTo(int to, bool callCallbacks)
{
	vstd::amax(to, 0);
	vstd::amin(to, positions);

	//same, old position?
	if(value == to)
		return;
	value = to;

	updateSliderPos();

	if (callCallbacks)
		moved(getValue());
}

void CSlider::clickPressed(const Point & cursorPosition)
{
	if(!slider->isBlocked())
	{
		double pw = 0;
		double rw = 0;
		if(getOrientation() == Orientation::HORIZONTAL)
		{
			pw = cursorPosition.x-pos.x-25;
			rw = pw / static_cast<double>(pos.w - 48);
		}
		else
		{
			pw = cursorPosition.y-pos.y-24;
			rw = pw / (pos.h-48);
		}

		// click on area covered by buttons -> ignore, will be handled by left/right buttons
		if (!vstd::iswithin(rw, 0, 1))
			return;

		slider->clickPressed(cursorPosition);
		scrollTo((int)(rw * positions  +  0.5));
		return;
	}
}

bool CSlider::receiveEvent(const Point &position, int eventType) const
{
	if (eventType == LCLICK)
	{
		return pos.isInside(position) && !left->pos.isInside(position) && !right->pos.isInside(position);
	}

	if(eventType != WHEEL && eventType != GESTURE)
	{
		return CIntObject::receiveEvent(position, eventType);
	}

	if (!scrollBounds)
		return true;

	Rect testTarget = *scrollBounds + pos.topLeft();

	return testTarget.isInside(position);
}

CSlider::CSlider(Point position, int totalw, const SliderMovingFunctor & Moved, int Capacity, int Amount, int Value, Orientation orientation, CSlider::EStyle style)
	: Scrollable(LCLICK | DRAG, position, orientation ),
	capacity(Capacity),
	amount(Amount),
	value(Value),
	moved(Moved)
{
	OBJECT_CONSTRUCTION;
	setAmount(amount);
	vstd::amax(value, 0);
	vstd::amin(value, positions);

	if(style == BROWN)
	{
		AnimationPath name = AnimationPath::builtin(getOrientation() == Orientation::HORIZONTAL ? "IGPCRDIV.DEF" : "OVBUTN2.DEF");
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
		left = std::make_shared<CButton>(Point(), AnimationPath::builtin(getOrientation() == Orientation::HORIZONTAL ? "SCNRBLF.DEF" : "SCNRBUP.DEF"), CButton::tooltip());
		right = std::make_shared<CButton>(Point(), AnimationPath::builtin(getOrientation() == Orientation::HORIZONTAL ? "SCNRBRT.DEF" : "SCNRBDN.DEF"), CButton::tooltip());
		slider = std::make_shared<CButton>(Point(), AnimationPath::builtin("SCNRBSL.DEF"), CButton::tooltip());
	}
	slider->setActOnDown(true);
	slider->setSoundDisabled(true);
	left->setSoundDisabled(true);
	right->setSoundDisabled(true);

	if (getOrientation() == Orientation::HORIZONTAL)
		right->moveBy(Point(totalw - right->pos.w, 0));
	else
		right->moveBy(Point(0, totalw - right->pos.h));

	left->addCallback(std::bind(&CSlider::scrollPrev,this));
	right->addCallback(std::bind(&CSlider::scrollNext,this));

	if(getOrientation() == Orientation::HORIZONTAL)
	{
		pos.h = slider->pos.h;
		pos.w = totalw;
	}
	else
	{
		pos.w = slider->pos.w;
		pos.h = totalw;
	}

	// for horizontal sliders that act as values selection - add keyboard event to receive left/right click
	if (getOrientation() == Orientation::HORIZONTAL)
		addUsedEvents(KEYBOARD);

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

void CSlider::keyPressed(EShortcut key)
{
	int moveDest = value;
	switch(key)
	{
	case EShortcut::MOVE_UP:
		if (getOrientation() == Orientation::VERTICAL)
			moveDest = value - getScrollStep();
		break;
	case EShortcut::MOVE_LEFT:
		if (getOrientation() == Orientation::HORIZONTAL)
			moveDest = value - getScrollStep();
		break;
	case EShortcut::MOVE_DOWN:
		if (getOrientation() == Orientation::VERTICAL)
			moveDest = value + getScrollStep();
		break;
	case EShortcut::MOVE_RIGHT:
		if (getOrientation() == Orientation::HORIZONTAL)
			moveDest = value + getScrollStep();
		break;
	case EShortcut::MOVE_PAGE_UP:
		moveDest = value - capacity + getScrollStep();
		break;
	case EShortcut::MOVE_PAGE_DOWN:
		moveDest = value + capacity - getScrollStep();
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

	scrollTo(moveDest);
}

void CSlider::scrollToMin()
{
	scrollTo(0);
}

void CSlider::scrollToMax()
{
	scrollTo(amount);
}

SliderNonlinear::SliderNonlinear(Point position, int length, const std::function<void(int)> & Moved, const std::vector<int> & values, int Value, Orientation orientation, EStyle style)
	: CSlider(position, length, Moved, 1, values.size(), Value, orientation, style)
	, scaledValues(values)
{

}

int SliderNonlinear::getValue() const
{
	return scaledValues.at(CSlider::getValue());
}

void SliderNonlinear::setValue(int to, bool callCallbacks)
{
	size_t nearest = 0;

	for(size_t i = 0; i < scaledValues.size(); ++i)
	{
		int nearestDistance = std::abs(to - scaledValues[nearest]);
		int currentDistance = std::abs(to - scaledValues[i]);

		if(currentDistance < nearestDistance)
			nearest = i;
	}

	scrollTo(nearest, callCallbacks);
}
