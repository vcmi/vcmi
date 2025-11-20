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
#include "../GameEngine.h"
#include "../render/AssetGenerator.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CConfigHandler.h"

void CSlider::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	bool onControl = pos.isInside(cursorPosition) && !left->pos.isInside(cursorPosition) && !right->pos.isInside(cursorPosition);
	if(!onControl && !slider->isPressed())
		return;

	if(onControl && !slider->isPressed())
		slider->clickPressed(cursorPosition);

	double newPosition = 0;
	if(getOrientation() == Orientation::HORIZONTAL)
	{
		newPosition = cursorPosition.x - pos.x - 16 - (barLength / 2);
		newPosition *= positions;
		newPosition /= (pos.w - 32 - barLength);
	}
	else
	{
		newPosition = cursorPosition.y - pos.y - 16 - (barLength / 2);
		newPosition *= positions;
		newPosition /= (pos.h - 32 - barLength);
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
			part*=(pos.w-32-barLength);
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
			part*=(pos.h-32-barLength);
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

double CSlider::getClickPos(const Point & cursorPosition)
{
	double pw = 0;
	double rw = 0;
	bool inside = cursorPosition.x >= pos.x && cursorPosition.x <= pos.x + pos.w && cursorPosition.y >= pos.y && cursorPosition.y <= pos.y + pos.h;
	if(getOrientation() == Orientation::HORIZONTAL)
	{
		pw = cursorPosition.x-pos.x-16-(barLength/2);
		rw = pw / static_cast<double>(pos.w - 32 - barLength);
	}
	else
	{
		pw = cursorPosition.y-pos.y-16-(barLength/2);
		rw = pw / (pos.h - 32 - barLength);
	}

	if (inside)
		rw = std::clamp(rw, 0.0, 1.0);

	return rw;
}

void CSlider::clickPressed(const Point & cursorPosition)
{
	bool onControl = pos.isInside(cursorPosition) && !left->pos.isInside(cursorPosition) && !right->pos.isInside(cursorPosition);
	if(!onControl)
		return;

	if(slider->isBlocked())
		return;

	// click on area covered by buttons -> ignore, will be handled by left/right buttons
	auto rw = getClickPos(cursorPosition);
	if (!vstd::iswithin(rw, 0, 1))
		return;

	slider->clickPressed(cursorPosition);
	scrollTo((int)(rw * positions + 0.5));
}

void CSlider::clickReleased(const Point & cursorPosition)
{
	if(slider->isBlocked())
		return;

	slider->clickReleased(cursorPosition);
}

bool CSlider::receiveEvent(const Point &position, int eventType) const
{
	if (eventType == LCLICK)
		return true; //capture "clickReleased" also outside of control

	if(eventType != WHEEL && eventType != GESTURE)
		return CIntObject::receiveEvent(position, eventType);

	if (!scrollBounds)
		return true;

	Rect testTarget = *scrollBounds + pos.topLeft();

	return testTarget.isInside(position);
}

CSlider::CSlider(Point position, int totalw, const SliderMovingFunctor & Moved, int Capacity, int Amount, int Value, Orientation orientation, CSlider::EStyle Style)
	: Scrollable(LCLICK | DRAG, position, orientation ),
	capacity(Capacity),
	amount(Amount),
	value(Value),
	moved(Moved),
	length(totalw),
	style(Style)
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

		left->setImageOrder(0, 1, 1, 1);
		right->setImageOrder(2, 3, 3, 3);
	}
	else
	{
		left = std::make_shared<CButton>(Point(), AnimationPath::builtin(getOrientation() == Orientation::HORIZONTAL ? "SCNRBLF.DEF" : "SCNRBUP.DEF"), CButton::tooltip());
		right = std::make_shared<CButton>(Point(), AnimationPath::builtin(getOrientation() == Orientation::HORIZONTAL ? "SCNRBRT.DEF" : "SCNRBDN.DEF"), CButton::tooltip());
	}
	left->setSoundDisabled(true);
	right->setSoundDisabled(true);

	updateSlider();

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

void CSlider::updateSlider()
{
	OBJECT_CONSTRUCTION;

	auto assetGenerator = ENGINE->renderHandler().getAssetGenerator();
	auto layout = assetGenerator->createSliderBar(style == BROWN, getOrientation() == Orientation::HORIZONTAL, barLength);
	std::string sliderName = "Slider-" + std::string(style == BROWN ? "brown" : "blue") + "-" + std::string(getOrientation() == Orientation::HORIZONTAL ? "horizontal" : "vertical") + "-" + std::to_string(barLength);
	assetGenerator->addAnimationFile(AnimationPath::builtin("SPRITES/" + sliderName), layout);
	ENGINE->renderHandler().updateGeneratedAssets();

	slider = std::make_shared<CButton>(Point(), AnimationPath::builtin(sliderName), CButton::tooltip());
	slider->setActOnDown(true);
	slider->setSoundDisabled(true);
}

void CSlider::setAmount( int to )
{
	amount = to;
	positions = to - capacity;
	vstd::amax(positions, 0);

	if(settings["general"]["enableUiEnhancements"].Bool())
	{
		int track = length - 32;
		if(to > 0)
			barLength = (track * capacity) / to;
		else
			barLength = track;
		vstd::amax(barLength, 16);
		vstd::amin(barLength, track);
	}
	else
		barLength = 16;

	updateSlider();
	updateSliderPos();
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
