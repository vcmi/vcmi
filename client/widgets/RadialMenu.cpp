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

#include "../gui/CGuiHandler.h"
#include "../render/IImage.h"
#include "CGarrisonInt.h"

RadialMenuItem::RadialMenuItem(std::string imageName, std::function<void()> callback)
	: callback(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = IImage::createFromFile("radialMenu/" + imageName);
	picture = std::make_shared<CPicture>(image, Point(0,0));
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

RadialMenu::RadialMenu(CGarrisonInt * army, CGarrisonSlot * slot)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	bool isExchange = army->upperArmy() && army->lowerArmy(); // two armies exist

	addItem(ITEM_NW, "stackMerge", [=](){army->bulkMergeStacks(slot);});
	addItem(ITEM_NE, "stackInfo", [=](){slot->viewInfo();});

	addItem(ITEM_WW, "stackSplitOne", [=](){slot->splitIntoParts(slot->getGarrison(), 1); });
	addItem(ITEM_EE, "stackSplitEqual", [=](){army->bulkSmartSplitStack(slot);});

	if (isExchange)
	{
		addItem(ITEM_SW, "stackMove", [=](){army->moveStackToAnotherArmy(slot);});
		//FIXME: addItem(ITEM_SE, "stackSplitDialog", [=](){slot->split();});
	}

	for(const auto & item : items)
		pos = pos.include(item->pos);

}

void RadialMenu::addItem(const Point & offset, const std::string & path, std::function<void()> callback )
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
		for(const auto & item : items)
		{
			if (item->isInside(finalPosition))
			{
				item->callback();
				return;
			}
		}
	}
}
