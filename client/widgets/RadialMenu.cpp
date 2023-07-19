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

RadialMenuItem::RadialMenuItem(const std::string & imageName, const std::string & hoverText, const std::function<void()> & callback)
	: callback(callback)
	, hoverText(hoverText)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = IImage::createFromFile("radialMenu/" + imageName, EImageBlitMode::COLORKEY);
	picture = std::make_shared<CPicture>(image, Point(0, 0));
	pos = picture->pos;
}

RadialMenu::RadialMenu(const Point & positionToCenter, const std::vector<RadialMenuConfig> & menuConfig)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += positionToCenter;

	addItem(Point(0,0), true, "itemEmpty", "", [](){});

	Point itemSize = items.back()->pos.dimensions();
	moveBy(-itemSize / 2);

	for (auto const & item : menuConfig)
		addItem(item.itemPosition, item.enabled, item.imageName, item.hoverText, item.callback);

	statusBar = CGStatusBar::create(-80, -100, "radialMenu/statusBar");

	for(const auto & item : items)
		pos = pos.include(item->pos);

	fitToScreen(10);

	addUsedEvents(GESTURE);
}

void RadialMenu::addItem(const Point & offset, bool enabled, const std::string & path, const std::string & hoverText, const std::function<void()>& callback )
{
	if (!enabled)
		return;

	auto item = std::make_shared<RadialMenuItem>(path, hoverText, callback);

	item->moveBy(offset);

	items.push_back(item);
}

std::shared_ptr<RadialMenuItem> RadialMenu::findNearestItem(const Point & cursorPosition) const
{
	int bestDistance = std::numeric_limits<int>::max();
	std::shared_ptr<RadialMenuItem> bestItem;

	for(const auto & item : items)
	{
		Point vector = item->pos.center() - cursorPosition;

		if (vector.length() < bestDistance)
		{
			bestDistance = vector.length();
			bestItem = item;
		}
	}

	assert(bestItem);
	return bestItem;
}

void RadialMenu::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	auto item = findNearestItem(currentPosition);
	GH.statusbar()->write(item->hoverText);
}

void RadialMenu::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (on)
		return;

	auto item = findNearestItem(finalPosition);

	// we need to close this window first so if action spawns a new window it won't be closed instead
	GH.windows().popWindows(1);
	item->callback();
}
