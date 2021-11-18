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

class CBank;
class Nullkiller;

class DLL_EXPORT FuzzyHelper
{
private:
	const Nullkiller * ai;
	TacticalAdvantageEngine tacticalAdvantageEngine;

public:
	FuzzyHelper(const Nullkiller * ai) : ai(ai), tacticalAdvantageEngine() {}

	ui64 estimateBankDanger(const CBank * bank); //TODO: move to another class?

	ui64 evaluateDanger(const CGObjectInstance * obj);
	ui64 evaluateDanger(crint3 tile, const CGHeroInstance * visitor, bool checkGuards = true);
};
