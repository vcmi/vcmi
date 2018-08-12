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

#include "AINodeStorage.h"

class AIPathfinderConfig : public CPathfinderConfig
{
public:
	AIPathfinderConfig(CPlayerSpecificInfoCallback * cb, std::shared_ptr<AINodeStorage> nodeStorage);
};