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

bool RadialMenuItem::isInside(const Point & position)
{
	Point localPosition = position - pos.topLeft();

	return !image->isTransparent(localPosition);
}

RadialMenu::RadialMenu(const Point & positionToCenter, const std::vector<RadialMenuConfig> & menuConfig)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += positionToCenter;

	addItem(Point(0,0), "itemEmpty", "", [](){});

	Point itemSize = items.back()->pos.dimensions();
	moveBy(-itemSize / 2);

	for (auto const & item : menuConfig)
		addItem(item.itemPosition, item.imageName, item.hoverText, item.callback);

	statusBar = CGStatusBar::create(-80, -100, "radialMenu/statusBar");

	for(const auto & item : items)
		pos = pos.include(item->pos);

	fitToScreen(10);

	addUsedEvents(GESTURE);
}

void RadialMenu::addItem(const Point & offset, const std::string & path, const std::string & hoverText, const std::function<void()>& callback )
{
	auto item = std::make_shared<RadialMenuItem>(path, hoverText, callback);

	item->moveBy(offset);

	items.push_back(item);
}

void RadialMenu::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{

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
