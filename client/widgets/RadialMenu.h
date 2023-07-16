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
class CFilledTexture;
class CGStatusBar;


class RadialMenuItem : public CIntObject
{
	std::shared_ptr<IImage> image;
	std::shared_ptr<CPicture> picture;
public:
	std::function<void()> callback;

	RadialMenuItem(const std::string& imageName, const std::function<void()>& callback);

	bool isInside(const Point & position);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
};

class RadialMenu : public CIntObject
{
	static constexpr Point ITEM_NW = Point( -40, -70);
	static constexpr Point ITEM_NE = Point( +40, -70);
	static constexpr Point ITEM_WW = Point( -80, 0);
	static constexpr Point ITEM_EE = Point( +80, 0);
	static constexpr Point ITEM_SW = Point( -40, +70);
	static constexpr Point ITEM_SE = Point( +40, +70);

	std::vector<std::shared_ptr<RadialMenuItem>> items;

	std::shared_ptr<CFilledTexture> statusBarBackground;
	std::shared_ptr<CGStatusBar> statusBar;

	void addItem(const Point & offset, const std::string & path, const std::function<void()>& callback );
public:
	RadialMenu(const Point & positionToCenter, CGarrisonInt * army, CGarrisonSlot * slot);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void show(Canvas & to) override;
};
