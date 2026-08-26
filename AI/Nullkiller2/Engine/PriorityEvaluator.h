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

#include "../Goals/CGoal.h"
#include "../Pathfinding/AIPathfinder.h"
#include "mapObjects/CGObjectInstance.h"
#include <cstdint>
#include <utility>

struct RewardableObjectInfo;

namespace NK2AI
{

float evaluateEnemyTownConquestValue(float baseValue, int visibleEnemyTownCount);
float evaluateMaxArmyLossForConquest(float baseMaxArmyLoss, float conquestValue, bool isEnemyTownConquest);

class BuildingInfo;
class Nullkiller;
struct HitMapInfo;

struct RewardEvaluationRequest
{
	const CGObjectInstance * target;
	const CGHeroInstance * hero;
	const CCreatureSet * army;
	HeroRole heroRole;
	bool considerRecruitmentCost;
};

struct CreatureRewardValueInfo
{
	enum class State : uint8_t
	{
		UNKNOWN, //value cannot be determined; pair is ignored
		UNUSABLE, //no possible reward improves the army; pair is {0, 0}
		DEPENDS_ON_TIER, //usefulness depends on the hidden bank tier; pair is {0, N}
		USABLE //every possible reward improves the army; {N, N} is exact, {N, M} is a range
	};
	State state = State::UNKNOWN;
	std::pair<uint64_t, uint64_t> minMaxValue = {0, 0};
};

struct RewardEvaluationResult
{
	int32_t goldReward = 0;
	uint64_t armyReward = 0;
	uint64_t armyGrowth = 0;
	float skillReward = 0;
	float strategicalValue = 0;
	float conquestValue = 0;
	int32_t goldCost = 0;
	CreatureRewardValueInfo creatureReward;
};

class RewardEvaluator
{
public:
	const Nullkiller * aiNk;

	RewardEvaluator(const Nullkiller * aiNk) : aiNk(aiNk) {}

	RewardEvaluationResult evaluateRewards(const RewardEvaluationRequest &) const;
	CreatureRewardValueInfo getCreatureReward(const RewardEvaluationRequest &) const;
	uint64_t getArmyGrowth(const RewardEvaluationRequest &) const;
	int getGoldCost(const RewardEvaluationRequest &) const;
	float getEnemyHeroStrategicalValue(const CGHeroInstance * enemy) const;
	float getNowResourceRequirementStrength(GameResID resType) const;
	float getCombinedResourceRequirementStrength(const TResources & res) const;
	float getStrategicalValue(const CGObjectInstance * target) const;
	float getConquestValue(const CGObjectInstance* target) const;
	float getTotalResourceRequirementStrength(GameResID resType) const;
	uint64_t getUpgradeArmyReward(const CGTownInstance * town, const BuildingInfo & bi) const;
	const HitMapInfo & getEnemyHeroDanger(const int3 & tile, uint8_t turn) const;
	uint64_t townArmyGrowth(const CGTownInstance * town) const;
	float getManaRecoveryArmyReward(const CGHeroInstance * hero) const;

private:
	uint64_t calculateRewardArmyValue(const RewardableObjectInfo *, const CreatureRewardValueInfo & creatureReward) const;
	uint64_t getArmyReward(const RewardEvaluationRequest &, const RewardableObjectInfo * rewardableInfo, const CreatureRewardValueInfo & creatureReward) const;
	CreatureRewardValueInfo getCreatureReward(const RewardEvaluationRequest &, const RewardableObjectInfo * rewardableInfo) const;
	float calculateStrategicalRewardValue(const CGObjectInstance * target, const RewardableObjectInfo * rewardableInfo) const;
	float getStrategicalValue(const CGObjectInstance * target, const RewardableObjectInfo * rewardableInfo) const;
	float evaluateWitchHutSkillScore(const RewardEvaluationRequest &, const RewardableObjectInfo * rewardableInfo) const;
	float getSkillReward(const RewardEvaluationRequest &, const RewardableObjectInfo * rewardableInfo) const;
	int32_t getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero, const RewardableObjectInfo * rewardableInfo) const;
};

struct DLL_EXPORT EvaluationContext
{
	float movementCost;
	std::array<float, 2> movementCostByRole;
	int manaCost;
	uint64_t danger;
	float closestWayRatio;
	float armyLossRatio;
	float armyReward;
	uint64_t armyGrowth;
	int32_t goldReward;
	int32_t goldCost;
	float skillReward;
	float strategicalValue;
	float conquestValue; // capitol towns: 1.5x base value (boosted if owned by another AI) ... towns without fort: 0.8x, special case: 10.0 if no towns are currently owned (high priority to get first town). Strategic importance of capturing or conquering specific objects in the game, primarily enemy-controlled towns and heroes
	HeroRole heroRole;
	uint8_t turn;
	RewardEvaluator evaluator;
	float enemyHeroDangerRatio; // dangerRatio = enemyDanger.danger / (double)ourStrength. A float value between 0 and 1 (or higher) representing the ratio of enemy hero danger to our army strength
	float threat;
	float armyInvolvement;
	int defenseValue; // 0-1: No fortifications (undefended), 2: Fort level (basic defensive structure), 3: Citadel level (stronger defenses), 4: Castle level (maximum defenses)
	bool isDefend;
	int threatTurns;
	TResources buildingCost;
	bool involvesSailing;
	bool isTradeBuilding;
	bool isExchange;
	bool isArmyUpgrade;
	bool isHero;
	bool isEnemy;
	bool isOneWayPortalProbe;
	bool unusableCreatureBankReward;
	bool preparesForCreatureBankReward;
	int explorePriority; // 1 important, 2 medium, 3 lowest importance
	float powerRatio; // Ratio of the committed army's strength to the total power of matching creatures available to the AI

	EvaluationContext(const Nullkiller * aiNk);

	void addNonCriticalStrategicalValue(float value);
	float getMovementCost(HeroRole role) const;
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
	PriorityEvaluator(const Nullkiller * aiNk);
	~PriorityEvaluator();

	float evaluate(Goals::TSubgoal task, int priorityTier = BUILDINGS);
	float evaluate(Goals::TSubgoal task, int priorityTier, const EvaluationContext & evaluationContext);
	EvaluationContext buildEvaluationContext(const Goals::TSubgoal & goal) const;

	enum PriorityTier : int32_t
	{
		BUILDINGS = 0,
		INSTAKILL,
		INSTADEFEND,
		KILL,
		ONE_WAY_PORTAL_PROBE,
		ESCAPE,
		EXPLORE_AND_GATHER, // Includes guarded resources/artifacts/portals
		DEFEND,
		MAX_PRIORITY_TIER = DEFEND
	};

private:
	const Nullkiller * aiNk;

	std::vector<std::shared_ptr<IEvaluationContextBuilder>> evaluationContextBuilders;

	static float evaluateMovement(float score, float movementCost);
	static float evaluateArmyLossRatio(float score, float armyLossRatio, HeroRole heroRole);
	static float evaluateSkillReward(float score, float skillReward, float armyInvolvement, float armyLossRatio);
	static float evaluateConquestValue(float score, float conquestValue, float armyInvolvement);
};

}
