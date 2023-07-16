/*
 * RadialMenu.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RadialMenu.h"

#include "Images.h"
#include "TextControls.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../render/IImage.h"
#include "CGarrisonInt.h"

RadialMenuItem::RadialMenuItem(const std::string & imageName, const std::function<void()> & callback)
	: callback(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = IImage::createFromFile("radialMenu/" + imageName);
	picture = std::make_shared<CPicture>(image, Point(0, 0));
	pos = picture->pos;
}

bool RadialMenuItem::isInside(const Point & position)
{
	Point localPosition = position - pos.topLeft();

	return !image->isTransparent(localPosition);
}

void RadialMenuItem::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{

}

void RadialMenuItem::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{

}

RadialMenu::RadialMenu(const Point & positionToCenter, CGarrisonInt * army, CGarrisonSlot * slot)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += positionToCenter;

	bool isExchange = army->upperArmy() && army->lowerArmy(); // two armies exist

	addItem(Point(0,0), "stackEmpty", [](){});

	Point itemSize = items.back()->pos.dimensions();
	moveBy(-itemSize / 2);

	addItem(ITEM_NW, "stackMerge", [=](){army->bulkMergeStacks(slot);});
	addItem(ITEM_NE, "stackInfo", [=](){slot->viewInfo();});

	addItem(ITEM_WW, "stackSplitOne", [=](){slot->splitIntoParts(slot->getGarrison(), 1); });
	addItem(ITEM_EE, "stackSplitEqual", [=](){army->bulkSmartSplitStack(slot);});

	if (isExchange)
	{
		addItem(ITEM_SW, "stackMove", [=](){army->moveStackToAnotherArmy(slot);});
		//FIXME: addItem(ITEM_SE, "stackSplitDialog", [=](){slot->split();});
	}

	//statusBarBackground = std::make_shared<CFilledTexture>("DiBoxBck", Rect(-itemSize.x * 2, -100, itemSize.x * 4, 20));
	//statusBar = CGStatusBar::create(statusBarBackground);

	for(const auto & item : items)
		pos = pos.include(item->pos);

	fitToScreen(10);

	addUsedEvents(GESTURE);
}

void RadialMenu::addItem(const Point & offset, const std::string & path, const std::function<void()>& callback )
{
	auto item = std::make_shared<RadialMenuItem>(path, callback);

	item->moveBy(offset);

	items.push_back(item);
}

void RadialMenu::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{

}

void RadialMenu::show(Canvas & to)
{
	showAll(to);
}

void RadialMenu::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (!on)
	{
		// we need to close this window first so if action spawns a new window it won't be closed instead
		GH.windows().popWindows(1);

		for(const auto & item : items)
		{
			if (item->isInside(finalPosition))
			{
				item->callback();
				break;
			}
		}
	}
}
