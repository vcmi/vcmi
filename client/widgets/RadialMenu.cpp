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

#include "../eventsSDL/InputHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../render/IImage.h"
#include "../CGameInfo.h"

#include "../../lib/CGeneralTextHandler.h"

RadialMenuItem::RadialMenuItem(const std::string & imageName, const std::string & hoverText, const std::function<void()> & callback, bool alternativeLayout)
	: callback(callback)
	, hoverText(hoverText)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	inactiveImage = std::make_shared<CPicture>(ImagePath::builtin(alternativeLayout ? "radialMenu/itemInactiveAlt" : "radialMenu/itemInactive"), Point(0, 0));
	selectedImage = std::make_shared<CPicture>(ImagePath::builtin(alternativeLayout ? "radialMenu/itemEmptyAlt" : "radialMenu/itemEmpty"), Point(0, 0));

	iconImage = std::make_shared<CPicture>(ImagePath::builtin("radialMenu/" + imageName), Point(0, 0));

	pos = selectedImage->pos;
	selectedImage->setEnabled(false);
}

void RadialMenuItem::setSelected(bool selected)
{
	selectedImage->setEnabled(selected);
	inactiveImage->setEnabled(!selected);
}

RadialMenu::RadialMenu(const Point & positionToCenter, const std::vector<RadialMenuConfig> & menuConfig, bool alternativeLayout):
	centerPosition(positionToCenter), alternativeLayout(alternativeLayout)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += positionToCenter;

	Point itemSize = alternativeLayout ? Point(80, 70) : Point(70, 80);
	moveBy(-itemSize / 2);
	pos.w = itemSize.x;
	pos.h = itemSize.y;

	for (auto const & item : menuConfig)
		addItem(item.itemPosition, item.enabled, item.imageName, item.hoverText, item.callback);

	statusBar = CGStatusBar::create(-80, -100, ImagePath::builtin("radialMenu/statusBar"));

	for(const auto & item : items)
		pos = pos.include(item->pos);
	pos = pos.include(statusBar->pos);

	fitToScreen(10);

	addUsedEvents(GESTURE);
}

void RadialMenu::addItem(const Point & offset, bool enabled, const std::string & path, const std::string & hoverText, const std::function<void()>& callback )
{
	if (!enabled)
		return;

	auto item = std::make_shared<RadialMenuItem>(path, CGI->generaltexth->translate(hoverText), callback, alternativeLayout);

	item->moveBy(offset);

	items.push_back(item);
}

std::shared_ptr<RadialMenuItem> RadialMenu::findNearestItem(const Point & cursorPosition) const
{
	static const int requiredDistanceFromCenter = 45;

	// cursor is inside centeral area -> no selection
	if ((centerPosition - cursorPosition).length() < requiredDistanceFromCenter)
		return nullptr;

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
	auto newSelection = findNearestItem(currentPosition);


	if (newSelection != selectedItem)
	{
		if (selectedItem)
			selectedItem->setSelected(false);

		if (newSelection)
		{
			GH.statusbar()->write(newSelection->hoverText);
			newSelection->setSelected(true);
		}
		else
			GH.statusbar()->clear();

		selectedItem = newSelection;

		GH.windows().totalRedraw();
	}
}

void RadialMenu::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (on)
		return;

	auto item = findNearestItem(finalPosition);

	// we need to close this window first so if action spawns a new window it won't be closed instead
	GH.windows().popWindows(1);
	if (item)
	{
		GH.input().hapticFeedback();
		item->callback();
	}
}
