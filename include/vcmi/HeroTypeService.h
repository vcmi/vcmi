/*
 * HeroTypeService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class HeroTypeID;
class HeroType;

class DLL_LINKAGE HeroTypeService
{
public:
	virtual ~HeroTypeService() = default;
	virtual const HeroType * getHeroType(const HeroTypeID & heroTypeID) const = 0;
};
