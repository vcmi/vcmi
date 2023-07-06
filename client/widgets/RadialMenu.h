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

class RadialMenuItem : public CIntObject
{
	std::shared_ptr<IImage> image;
	std::shared_ptr<CPicture> picture;
public:
	std::function<void()> callback;

	RadialMenuItem(std::string imageName, std::function<void()> callback);

	bool isInside(const Point & position);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
};

class RadialMenu : public CIntObject
{
	static constexpr Point ITEM_NW = Point( -35, -85);
	static constexpr Point ITEM_NE = Point( +35, -85);
	static constexpr Point ITEM_WW = Point( -85, 0);
	static constexpr Point ITEM_EE = Point( +85, 0);
	static constexpr Point ITEM_SW = Point( -35, +85);
	static constexpr Point ITEM_SE = Point( +35, +85);

	std::vector<std::shared_ptr<RadialMenuItem>> items;

	void addItem(const Point & offset, const std::string & path, std::function<void()> callback );
public:
	RadialMenu(CGarrisonInt * army, CGarrisonSlot * slot);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void show(Canvas & to) override;
};
