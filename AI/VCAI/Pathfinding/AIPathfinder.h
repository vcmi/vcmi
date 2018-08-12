/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AIUtility.h"
#include "AINodeStorage.h"

class AIPathfinder
{
private:
	static std::vector<std::shared_ptr<AINodeStorage>> storagePool;
	static std::map<HeroPtr, std::shared_ptr<AINodeStorage>> storageMap;
	static boost::mutex storageMutex;
	CPlayerSpecificInfoCallback * cb;

public:
	AIPathfinder(CPlayerSpecificInfoCallback * cb);
	std::vector<AIPath> getPathInfo(HeroPtr hero, int3 tile);
	void clear();
};