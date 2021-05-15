/*
* AIPathfinder.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AINodeStorage.h"
#include "../AIUtility.h"
#include "../VCAI.h"

class AIPathfinder
{
private:
	static std::shared_ptr<AINodeStorage> storage;
	CPlayerSpecificInfoCallback * cb;
	VCAI * ai;

public:
	AIPathfinder(CPlayerSpecificInfoCallback * cb, VCAI * ai);
	std::vector<AIPath> getPathInfo(const HeroPtr & hero, const int3 & tile) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const;
	void updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain = false);
	void init();
};
