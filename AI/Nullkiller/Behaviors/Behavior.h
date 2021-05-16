/*
* Behavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../VCAI.h"
#error REMOVE THIS FILE

class Behavior : public Goals::AbstractGoal
{
public:
	virtual std::string toString() const = 0;
};
