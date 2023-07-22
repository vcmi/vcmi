/*
 * RadialMenu.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class IImage;

class CGarrisonInt;
class CGarrisonSlot;
class CGStatusBar;

struct RadialMenuConfig
{
	static constexpr Point ITEM_NW = Point(-40, -70);
	static constexpr Point ITEM_NE = Point(+40, -70);
	static constexpr Point ITEM_WW = Point(-80, 0);
	static constexpr Point ITEM_EE = Point(+80, 0);
	static constexpr Point ITEM_SW = Point(-40, +70);
	static constexpr Point ITEM_SE = Point(+40, +70);

	Point itemPosition;
	bool enabled;
	std::string imageName;
	std::string hoverText;
	std::function<void()> callback;
};

class RadialMenuItem : public CIntObject
{
	friend class RadialMenu;

	std::shared_ptr<CPicture> iconImage;
	std::shared_ptr<CPicture> selectedImage;
	std::shared_ptr<CPicture> inactiveImage;
	std::function<void()> callback;
	std::string hoverText;

public:
	RadialMenuItem(const std::string & imageName, const std::string & hoverText, const std::function<void()> & callback);

	void setSelected(bool selected);
};

class RadialMenu : public CIntObject
{
	std::vector<std::shared_ptr<RadialMenuItem>> items;
	std::shared_ptr<CGStatusBar> statusBar;
	std::shared_ptr<RadialMenuItem> selectedItem;

	Point centerPosition;

	void addItem(const Point & offset, bool enabled, const std::string & path, const std::string & hoverText, const std::function<void()> & callback);

	std::shared_ptr<RadialMenuItem> findNearestItem(const Point & cursorPosition) const;
public:
	RadialMenu(const Point & positionToCenter, const std::vector<RadialMenuConfig> & menuConfig);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
};
