/*
 * Fuzzy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#pragma once
#include "fl/Headers.h"
#include "Goals.h"

class VCAI;
class CArmedInstance;
class CBank;
struct SectorMap;

class engineBase
{
public:
	fl::Engine engine;
	fl::RuleBlock rules;

	engineBase();
	void configure();
	void addRule(const std::string & txt);
};

class TacticalAdvantage : public engineBase
{
public:
	TacticalAdvantage();
	fl::InputVariable * ourWalkers, *ourShooters, *ourFlyers, *enemyWalkers, *enemyShooters, *enemyFlyers;
	fl::InputVariable * ourSpeed, *enemySpeed;
	fl::InputVariable * bankPresent;
	fl::InputVariable * castleWalls;
	fl::OutputVariable * threat;
	~TacticalAdvantage();
};

class HeroMovementGoalEngineBase : public engineBase //abstract class for hero visiting goals, allows evaluating newly added elementar objectives with appropiately broad context data
{
public:
	HeroMovementGoalEngineBase();
	fl::InputVariable * strengthRatio;
	fl::InputVariable * heroStrength;
	fl::InputVariable * turnDistance;
	fl::InputVariable * missionImportance;
	fl::InputVariable * estimatedReward;
	fl::OutputVariable * value;
	~HeroMovementGoalEngineBase();

private:
	float calculateTurnDistanceInputValue(const CGHeroInstance * h, int3 tile) const;
};

class EvalVisitTile : public HeroMovementGoalEngineBase
{
public:
	EvalVisitTile();
	~EvalVisitTile();
};

class EvalWanderTargetObject : public HeroMovementGoalEngineBase //designed for use with VCAI::wander()
{
public:
	EvalWanderTargetObject();
	fl::InputVariable * objectValue;
	~EvalWanderTargetObject();
};

class FuzzyHelper
{
	friend class VCAI;

	TacticalAdvantage ta;

	EvalVisitTile vt;

	EvalWanderTargetObject wanderTarget;

public:
	enum RuleBlocks {BANK_DANGER, TACTICAL_ADVANTAGE, VISIT_TILE};
	//blocks should be initialized in this order, which may be confusing :/

	FuzzyHelper();
	void initTacticalAdvantage();

	float evaluate(Goals::Explore & g);
	float evaluate(Goals::RecruitHero & g);
	float evaluate(Goals::VisitTile & g);
	float evaluate(Goals::VisitHero & g);
	float evaluate(Goals::BuildThis & g);
	float evaluate(Goals::DigAtTile & g);
	float evaluate(Goals::CollectRes & g);
	float evaluate(Goals::Build & g);
	float evaluate(Goals::BuyArmy & g);
	float evaluate(Goals::GatherArmy & g);
	float evaluate(Goals::ClearWayTo & g);
	float evaluate(Goals::Invalid & g);
	float evaluate(Goals::AbstractGoal & g);
	void setPriority(Goals::TSubgoal & g);

	ui64 estimateBankDanger(const CBank * bank);
	float getTacticalAdvantage(const CArmedInstance * we, const CArmedInstance * enemy); //returns factor how many times enemy is stronger than us
	float getWanderTargetObjectValue(const CGHeroInstance & h, const ObjectIdRef & obj);

	Goals::TSubgoal chooseSolution(Goals::TGoalVec vec);
	//std::shared_ptr<AbstractGoal> chooseSolution (std::vector<std::shared_ptr<AbstractGoal>> & vec);
};
