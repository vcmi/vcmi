#ifndef NAN
    static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
    #define NAN (*(const float *) __nan)
#endif

#ifdef _MSC_VER
	#ifndef INFINITY
	union MSVC_FL
	{
	   unsigned __int8 Bytes[4];
	   float Value;
	};
	static union MSVC_FL INFINITY_HACK = {{0x00, 0x00, 0x80, 0x7F}};
	#define INFINITY (INFINITY_HACK.Value)
	#endif
#endif

#include "..\FuzzyLite\FuzzyLite.h"

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
class CArmedInstance;

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
	fl::RuleBlock bankBlock;

	fl::InputLVar * ourWalkers, * ourShooters, * ourFlyers, * enemyWalkers, * enemyShooters, * enemyFlyers;
	fl::InputLVar * ourSpeed, * enemySpeed;
	fl::InputLVar * bankPresent;
	fl::InputLVar * castleWalls;
	fl::OutputLVar * threat;
	fl::RuleBlock tacticalAdvantage;

public:
	enum RuleBlocks {BANK_DANGER, TACTICAL_ADVANTAGE};

	FuzzyHelper();
	void initBank();
	void initTacticalAdvantage();

	ui64 estimateBankDanger (int ID);
	float getTacticalAdvantage (const CArmedInstance *we, const CArmedInstance *enemy); //returns factor how many times enemy is stronger than us
};