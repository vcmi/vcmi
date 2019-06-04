/*
 * HeroClassService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class HeroClassID;
class HeroClass;

class DLL_LINKAGE HeroClassService
{
public:
	virtual ~HeroClassService() = default;
	virtual const HeroClass * getHeroClass(const HeroClassID & heroClassID) const = 0;
};
