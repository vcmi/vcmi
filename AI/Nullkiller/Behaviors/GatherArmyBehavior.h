/*
* GatherArmyBehavior.h, part of VCMI engine
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

class GatherArmyBehavior : public Behavior {
private:
	std::vector<int> objectTypes;
	std::vector<int> objectSubTypes;
	std::vector<const CGObjectInstance *> objectsToCapture;
	bool specificObjects;
public:
	GatherArmyBehavior()
	{
		objectTypes = std::vector<int>();
		specificObjects = false;
	}

	virtual Goals::TGoalVec getTasks() override;
	virtual std::string toString() const override;

private:
	Goals::TGoalVec deliverArmyToHero(const CGHeroInstance * hero) const;
	Goals::TGoalVec upgradeArmy(const CGTownInstance * upgrader) const;
};

