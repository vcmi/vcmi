/*
 * FuzzyHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#pragma once
#include "FuzzyEngines.h"

namespace NKAI
{

class Nullkiller2;

class DLL_EXPORT FuzzyHelper
{
private:
	const Nullkiller2 * ai;
	TacticalAdvantageEngine tacticalAdvantageEngine;

public:
	FuzzyHelper(const Nullkiller2 * ai): ai(ai) {}

	ui64 evaluateDanger(const CGObjectInstance * obj);
	ui64 evaluateDanger(const int3 & tile, const CGHeroInstance * visitor, bool checkGuards = true);
};

}
