/*
* BuyArmyBehavior.h, part of VCMI engine
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

class DefenceBehavior : public Behavior
{
public:
	DefenceBehavior()
	{
	}

	virtual Goals::TGoalVec getTasks() override;
	virtual std::string toString() const override;

private:
	void evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town);
};

