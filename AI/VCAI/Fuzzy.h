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

class FuzzyHelper
{
	friend class VCAI;

	fl::FuzzyEngine engine;

	fl::InputLVar* bankInput;
	fl::OutputLVar* bankDanger;
	fl::RuleBlock bankBlock;

	class TacticalAdvantage
	{
	public:
		fl::InputLVar * ourWalkers, * ourShooters, * ourFlyers, * enemyWalkers, * enemyShooters, * enemyFlyers;
		fl::InputLVar * ourSpeed, * enemySpeed;
		fl::InputLVar * bankPresent;
		fl::InputLVar * castleWalls;
		fl::OutputLVar * threat;
		fl::RuleBlock tacticalAdvantage;
		~TacticalAdvantage();
	} ta;

	class EvalVisitTile
	{
	public:
		fl::InputLVar * strengthRatio;
		fl::InputLVar * heroStrength;
		fl::InputLVar * turnDistance;
		fl::InputLVar * missionImportance;
		fl::OutputLVar * value;
		fl::RuleBlock rules;
		~EvalVisitTile();
	} vt;

public:
	enum RuleBlocks {BANK_DANGER, TACTICAL_ADVANTAGE, VISIT_TILE};
	//blocks should be initialized in this order, which may be confusing :/

	FuzzyHelper();
	void initBank();
	void initTacticalAdvantage();
	void initVisitTile();

	float evaluate (Goals::Explore & g);
	float evaluate (Goals::RecruitHero & g);
	float evaluate (Goals::VisitTile & g);
	float evaluate (Goals::VisitHero & g);
	float evaluate (Goals::BuildThis & g);
	float evaluate (Goals::DigAtTile & g);
	float evaluate (Goals::CollectRes & g);
	float evaluate (Goals::Build & g);
	float evaluate (Goals::Invalid & g);
	float evaluate (Goals::AbstractGoal & g);
	void setPriority (Goals::TSubgoal & g);

	ui64 estimateBankDanger (int ID);
	float getTacticalAdvantage (const CArmedInstance *we, const CArmedInstance *enemy); //returns factor how many times enemy is stronger than us

	Goals::TSubgoal chooseSolution (Goals::TGoalVec vec);
	//shared_ptr<AbstractGoal> chooseSolution (std::vector<shared_ptr<AbstractGoal>> & vec);
};
