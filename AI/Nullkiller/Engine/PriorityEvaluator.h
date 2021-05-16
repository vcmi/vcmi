/*
* PriorityEvaluator.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once
#include "fl/Headers.h"
#include "../Goals/Goals.h"

struct DLL_EXPORT EvaluationContext
{
	float movementCost;
	std::map<HeroRole, float> movementCostByRole;
	int manaCost;
	uint64_t danger;
	float closestWayRatio;
	float armyLossPersentage;
	float armyReward;
	int32_t goldReward;
	int32_t goldCost;
	float skillReward;
	float strategicalValue;
	HeroRole heroRole;
	uint8_t turn;

	EvaluationContext();
};

class IEvaluationContextBuilder
{
public:
	virtual void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal goal) const = 0;
};

class PriorityEvaluator
{
public:
	PriorityEvaluator();
	~PriorityEvaluator();
	void initVisitTile();

	float evaluate(Goals::TSubgoal task);

private:
	fl::Engine * engine;
	fl::InputVariable * armyLossPersentageVariable;
	fl::InputVariable * heroRoleVariable;
	fl::InputVariable * mainTurnDistanceVariable;
	fl::InputVariable * scoutTurnDistanceVariable;
	fl::InputVariable * turnVariable;
	fl::InputVariable * goldRewardVariable;
	fl::InputVariable * armyRewardVariable;
	fl::InputVariable * dangerVariable;
	fl::InputVariable * skillRewardVariable;
	fl::InputVariable * strategicalValueVariable;
	fl::InputVariable * rewardTypeVariable;
	fl::InputVariable * closestHeroRatioVariable;
	fl::InputVariable * goldPreasureVariable;
	fl::InputVariable * goldCostVariable;
	fl::OutputVariable * value;
	std::vector<std::shared_ptr<IEvaluationContextBuilder>> evaluationContextBuilders;

	EvaluationContext buildEvaluationContext(Goals::TSubgoal goal) const;
};
