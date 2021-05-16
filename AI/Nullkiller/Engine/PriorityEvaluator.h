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

class IEvaluationContextBuilder
{
public:
	virtual Goals::EvaluationContext buildEvaluationContext(Goals::TSubgoal goal) const = 0;
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
	fl::InputVariable * goldRewardVariable;
	fl::InputVariable * armyRewardVariable;
	fl::InputVariable * dangerVariable;
	fl::InputVariable * skillRewardVariable;
	fl::InputVariable * strategicalValueVariable;
	fl::InputVariable * rewardTypeVariable;
	fl::InputVariable * closestHeroRatioVariable;
	fl::OutputVariable * value;
	std::map<Goals::EGoals, std::shared_ptr<IEvaluationContextBuilder>> evaluationContextBuilders;

	Goals::EvaluationContext buildEvaluationContext(Goals::TSubgoal goal) const;
};
