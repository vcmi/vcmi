/*
* ArmyFormation.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../AIUtility.h"

#include "../../../lib/GameConstants.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/CTownHandler.h"
#include "../../../lib/CBuildingHandler.h"

namespace NKAI
{

struct HeroPtr;
class AIGateway;
class FuzzyHelper;
class Nullkiller;

class DLL_EXPORT ArmyFormation
{
private:
	std::shared_ptr<CCallback> cb; //this is enough, but we downcast from CCallback
	const Nullkiller * ai;

public:
	ArmyFormation(std::shared_ptr<CCallback> CB, const Nullkiller * ai): cb(CB), ai(ai) {}

	void rearrangeArmyForSiege(const CGTownInstance * town, const CGHeroInstance * attacker);
};

}
