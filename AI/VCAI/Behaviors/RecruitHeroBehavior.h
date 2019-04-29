/*
* Goals.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/VCMI_Lib.h"
#include "Behavior.h"
#include "../AIUtility.h"

class RecruitHeroBehavior : public Behavior
{
public:
	RecruitHeroBehavior()
	{
	}

	virtual Goals::TGoalVec getTasks() override;
	virtual std::string toString() const override;
};

class StartupBehavior : public Behavior
{
public:
	StartupBehavior()
	{
	}

	virtual Goals::TGoalVec getTasks() override;
	virtual std::string toString() const override;
};

