/*
 * CResDataBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../../lib/filesystem/ResourcePath.h"

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar : public CIntObject
{
	std::string buildDateString();

	std::shared_ptr<CPicture> background;

	std::map<GameResID, Point> resourcePositions;
	std::optional<Point> datePosition;

public:

	/// For dynamically-sized UI windows, e.g. adventure map interface
	CResDataBar(const ImagePath & imageName, const Point & position);

	/// For fixed-size UI windows, e.g. CastleInterface
	CResDataBar(const ImagePath & defname, int x, int y, int offx, int offy, int resdist, int datedist);

	void setDatePosition(const Point & position);
	void setResourcePosition(const GameResID & resource, const Point & position);

	void colorize(PlayerColor player);
	void showAll(Canvas & to) override;
};

