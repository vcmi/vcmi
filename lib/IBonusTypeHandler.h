/*
 * IBonusTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class IBonusBearer;
struct Bonus;

///High level interface for BonusTypeHandler

class IBonusTypeHandler
{
public:
	virtual ~IBonusTypeHandler(){};

	virtual std::string bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer, bool description) const = 0;
	virtual std::string bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const = 0;
};
