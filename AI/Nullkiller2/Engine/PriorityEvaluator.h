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
#if __has_include(<fuzzylite/Headers.h>)
#  include <fuzzylite/Headers.h>
#else
#  include <fl/Headers.h>
#endif
#include "../Goals/CGoal.h"
#include "../Pathfinding/AIPathfinder.h"

VCMI_LIB_NAMESPACE_BEGIN

VCMI_LIB_NAMESPACE_END

namespace NK2AI
{

class BuildingInfo;
class Nullkiller;
struct HitMapInfo;

class RewardEvaluator
{
public:
	const Nullkiller * aiNk;

	RewardEvaluator(const Nullkiller * aiNk) : aiNk(aiNk) {}

	uint64_t getArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army, bool checkGold) const;
	uint64_t getArmyGrowth(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const;
	int getGoldCost(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const;
	float getEnemyHeroStrategicalValue(const CGHeroInstance * enemy) const;
	float getNowResourceRequirementStrength(GameResID resType) const;
	float getCombinedResourceRequirementStrength(const TResources & res) const;
	float getStrategicalValue(const CGObjectInstance * target, const CGHeroInstance * hero = nullptr) const;
	float getConquestValue(const CGObjectInstance* target) const;
	float getTotalResourceRequirementStrength(GameResID resType) const;
	float evaluateWitchHutSkillScore(const CGObjectInstance * hut, const CGHeroInstance * hero, HeroRole role) const;
	float getSkillReward(const CGObjectInstance * target, const CGHeroInstance * hero, HeroRole role) const;
	int32_t getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero) const;
	uint64_t getUpgradeArmyReward(const CGTownInstance * town, const BuildingInfo & bi) const;
	const HitMapInfo & getEnemyHeroDanger(const int3 & tile, uint8_t turn) const;
	uint64_t townArmyGrowth(const CGTownInstance * town) const;
	float getManaRecoveryArmyReward(const CGHeroInstance * hero) const;
};

struct DLL_EXPORT EvaluationContext
{
	float movementCost;
	std::map<HeroRole, float> movementCostByRole;
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
	int explorePriority; // 1 important, 2 medium, 3 lowest importance
	float powerRatio; // powerRatio = heroPower / totalPower. The ratio of a hero's army strength to the total power of all creatures available to the AI

	EvaluationContext(const Nullkiller * aiNk);

	void addNonCriticalStrategicalValue(float value);
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
	void initVisitTile();

	float evaluate(Goals::TSubgoal task, int priorityTier = BUILDINGS);

	enum PriorityTier : int32_t
	{
		BUILDINGS = 0,
		INSTAKILL,
		INSTADEFEND,
		KILL,
		EXPLORE_AND_GATHER, // Includes guarded resources/artifacts/portals
		ESCAPE,
		DEFEND,
		MAX_PRIORITY_TIER = DEFEND
	};

private:
	const Nullkiller * aiNk;

	fl::Engine * engine;
	fl::InputVariable * armyLossRatioVariable;
	fl::InputVariable * heroRoleVariable;
	fl::InputVariable * mainTurnDistanceVariable;
	fl::InputVariable * scoutTurnDistanceVariable;
	fl::InputVariable * turnVariable;
	fl::InputVariable * goldRewardVsMovementVariable;
	fl::InputVariable * armyRewardVariable;
	fl::InputVariable * armyGrowthVariable;
	fl::InputVariable * dangerVariable;
	fl::InputVariable * skillRewardVariable;
	fl::InputVariable * strategicalValueVariable;
	fl::InputVariable * rewardTypeVariable;
	fl::InputVariable * closestHeroRatioVariable;
	fl::InputVariable * goldPressureVariable;
	fl::InputVariable * goldCostVariable;
	fl::InputVariable * fearVariable;
	fl::OutputVariable * value;
	std::vector<std::shared_ptr<IEvaluationContextBuilder>> evaluationContextBuilders;

	EvaluationContext buildEvaluationContext(const Goals::TSubgoal & goal) const;
	static float evaluateMovement(float score, float movementCost);
	static float evaluateArmyLossRatio(float score, float armyLossRatio, HeroRole heroRole);
	static float evaluateSkillReward(float score, float skillReward, float armyInvolvement, float armyLossRatio);
	static float evaluateConquestValue(float score, float conquestValue, float armyInvolvement);
};

}
