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
#include "../Goals/CGoal.h"
#include "../Pathfinding/AIPathfinder.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGWitchHut;

VCMI_LIB_NAMESPACE_END

namespace NKAI
{

class BuildingInfo;
class Nullkiller;
struct HitMapInfo;

class RewardEvaluator
{
public:
	const Nullkiller * ai;

	RewardEvaluator(const Nullkiller * ai) : ai(ai) {}

	uint64_t getArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army, bool checkGold) const;
	uint64_t getArmyGrowth(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const;
	int getGoldCost(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const;
	float getEnemyHeroStrategicalValue(const CGHeroInstance * enemy) const;
	float getResourceRequirementStrength(int resType) const;
	float getStrategicalValue(const CGObjectInstance * target) const;
	float getTotalResourceRequirementStrength(int resType) const;
	float evaluateWitchHutSkillScore(const CGWitchHut * hut, const CGHeroInstance * hero, HeroRole role) const;
	float getSkillReward(const CGObjectInstance * target, const CGHeroInstance * hero, HeroRole role) const;
	int32_t getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero) const;
	uint64_t getUpgradeArmyReward(const CGTownInstance * town, const BuildingInfo & bi) const;
	const HitMapInfo & getEnemyHeroDanger(const int3 & tile, uint8_t turn) const;
};

struct DLL_EXPORT EvaluationContext
{
	float movementCost;
	std::map<HeroRole, float> movementCostByRole;
	int manaCost;
	uint64_t danger;
	float closestWayRatio;
	float armyLossPersentage;
	float armyReward;
	uint64_t armyGrowth;
	int32_t goldReward;
	int32_t goldCost;
	float skillReward;
	float strategicalValue;
	HeroRole heroRole;
	uint8_t turn;
	RewardEvaluator evaluator;
	float enemyHeroDangerRatio;

	EvaluationContext(const Nullkiller * ai);
};

class IEvaluationContextBuilder
{
public:
	virtual ~IEvaluationContextBuilder() = default;
	virtual void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal goal) const = 0;
};

class Nullkiller;

class PriorityEvaluator
{
public:
	PriorityEvaluator(const Nullkiller * ai);
	~PriorityEvaluator();
	void initVisitTile();

	float evaluate(Goals::TSubgoal task);

private:
	const Nullkiller * ai;

	fl::Engine * engine;
	fl::InputVariable * armyLossPersentageVariable;
	fl::InputVariable * heroRoleVariable;
	fl::InputVariable * mainTurnDistanceVariable;
	fl::InputVariable * scoutTurnDistanceVariable;
	fl::InputVariable * turnVariable;
	fl::InputVariable * goldRewardVariable;
	fl::InputVariable * armyRewardVariable;
	fl::InputVariable * armyGrowthVariable;
	fl::InputVariable * dangerVariable;
	fl::InputVariable * skillRewardVariable;
	fl::InputVariable * strategicalValueVariable;
	fl::InputVariable * rewardTypeVariable;
	fl::InputVariable * closestHeroRatioVariable;
	fl::InputVariable * goldPreasureVariable;
	fl::InputVariable * goldCostVariable;
	fl::InputVariable * fearVariable;
	fl::OutputVariable * value;
	std::vector<std::shared_ptr<IEvaluationContextBuilder>> evaluationContextBuilders;

	EvaluationContext buildEvaluationContext(Goals::TSubgoal goal) const;
};

}
