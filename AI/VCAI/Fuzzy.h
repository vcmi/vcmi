#define INFINITY 1000000000 //definition required by FuzzyLite (?)
#define NAN 1000000001
#include "FuzzyLite.h"

/*
 * Fuzzy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/

class VCAI;

template <typename T> bool isinf (T val)
{
	return val == INFINITY || val == -INFINITY;
}

class FuzzyHelper
{
	friend class VCAI;

	fl::FuzzyEngine engine;
	fl::InputLVar* bankInput;
	fl::OutputLVar* bankDanger;
	fl::RuleBlock ruleBlock;

public:
	FuzzyHelper();
	ui64 estimateBankDanger (int ID);
};