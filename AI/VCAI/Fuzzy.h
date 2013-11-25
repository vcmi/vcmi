#pragma once
#include "../FuzzyLite/FuzzyLite.h"
#include "Goals.h"

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
//class TSubgoal;
//class TGoalVec;
//class AbstractGoal;

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

	Goals::TSubgoal chooseSolution (Goals::TGoalVec & vec);
	//shared_ptr<AbstractGoal> chooseSolution (std::vector<shared_ptr<AbstractGoal>> & vec);
};
