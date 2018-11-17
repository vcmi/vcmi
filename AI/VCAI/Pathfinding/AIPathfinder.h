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
	static std::vector<std::shared_ptr<AINodeStorage>> storagePool;
	static std::map<HeroPtr, std::shared_ptr<AINodeStorage>> storageMap;
	static boost::mutex storageMutex;
	CPlayerSpecificInfoCallback * cb;
	VCAI * ai;

public:
	AIPathfinder(CPlayerSpecificInfoCallback * cb, VCAI * ai);
	std::vector<AIPath> getPathInfo(HeroPtr hero, int3 tile);
	void clear();
};
