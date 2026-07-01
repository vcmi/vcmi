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

VCMI_LIB_NAMESPACE_BEGIN
class int3;
class CGObjectInstance;
class CGHeroInstance;
VCMI_LIB_NAMESPACE_END

namespace NK2AI
{

class Nullkiller;

class DLL_EXPORT FuzzyHelper
{
private:
	const Nullkiller * aiNk;

public:
	FuzzyHelper(const Nullkiller * aiNk): aiNk(aiNk) {}

	ui64 evaluateDanger(const CGObjectInstance * obj);
	ui64 evaluateDanger(const int3 & tile, const CGHeroInstance * visitor, bool checkGuards = true);
};

}
