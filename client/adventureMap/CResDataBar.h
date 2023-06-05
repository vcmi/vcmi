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

VCMI_LIB_NAMESPACE_BEGIN
enum class EGameResID : int8_t;
using GameResID = Identifier<EGameResID>;
VCMI_LIB_NAMESPACE_END

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
	CResDataBar(const std::string & imageName, const Point & position);

	/// For fixed-size UI windows, e.g. CastleInterface
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);

	void setDatePosition(const Point & position);
	void setResourcePosition(const GameResID & resource, const Point & position);

	void colorize(PlayerColor player);
	void showAll(Canvas & to) override;
};

