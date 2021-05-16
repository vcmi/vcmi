/*
* PriorityEvaluator.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "PriorityEvaluator.h"
#include <limits>

#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/mapObjects/CommonConstructors.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CPathfinder.h"
#include "../../../lib/CGameStateFwd.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../CCallback.h"
#include "../../../lib/filesystem/Filesystem.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/UnlockCluster.h"
#include "../Goals/BuildThis.h"

#define MIN_AI_STRENGHT (0.5f) //lower when combat AI gets smarter
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 100 times weaker than us

extern boost::thread_specific_ptr<CCallback> cb;

EvaluationContext::EvaluationContext(const Nullkiller * ai)
	: movementCost(0.0),
	manaCost(0),
	danger(0),
	closestWayRatio(1),
	movementCostByRole(),
	skillReward(0),
	goldReward(0),
	goldCost(0),
	armyReward(0),
	armyLossPersentage(0),
	heroRole(HeroRole::SCOUT),
	turn(0),
	strategicalValue(0),
	evaluator(ai),
	enemyHeroDangerRatio(0)
{
}

PriorityEvaluator::~PriorityEvaluator()
{
	delete engine;
}

void PriorityEvaluator::initVisitTile()
{
	auto file = CResourceHandler::get()->load(ResourceID("config/ai/object-priorities.txt"))->readAll();
	std::string str = std::string((char *)file.first.get(), file.second);
	engine = fl::FllImporter().fromString(str);
	armyLossPersentageVariable = engine->getInputVariable("armyLoss");
	heroRoleVariable = engine->getInputVariable("heroRole");
	dangerVariable = engine->getInputVariable("danger");
	turnVariable = engine->getInputVariable("turn");
	mainTurnDistanceVariable = engine->getInputVariable("mainTurnDistance");
	scoutTurnDistanceVariable = engine->getInputVariable("scoutTurnDistance");
	goldRewardVariable = engine->getInputVariable("goldReward");
	armyRewardVariable = engine->getInputVariable("armyReward");
	skillRewardVariable = engine->getInputVariable("skillReward");
	rewardTypeVariable = engine->getInputVariable("rewardType");
	closestHeroRatioVariable = engine->getInputVariable("closestHeroRatio");
	strategicalValueVariable = engine->getInputVariable("strategicalValue");
	goldPreasureVariable = engine->getInputVariable("goldPreasure");
	goldCostVariable = engine->getInputVariable("goldCost");
	fearVariable = engine->getInputVariable("fear");
	value = engine->getOutputVariable("Value");
}

int32_t estimateTownIncome(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	auto relations = cb->getPlayerRelations(hero->tempOwner, target->tempOwner);

	if(relations != PlayerRelations::ENEMIES)
		return 0; // if we already own it, no additional reward will be received by just visiting it

	auto town = cb->getTown(target->id);
	auto isNeutral = target->tempOwner == PlayerColor::NEUTRAL;
	auto isProbablyDeveloped = !isNeutral && town->hasFort();

	return isProbablyDeveloped ? 1500 : 500;
}

TResources getCreatureBankResources(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	auto objectInfo = VLC->objtypeh->getHandlerFor(target->ID, target->subID)->getObjectInfo(target->appearance);
	CBankInfo * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());
	auto resources = bankInfo->getPossibleResourcesReward();

	return resources;
}

uint64_t getCreatureBankArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	auto objectInfo = VLC->objtypeh->getHandlerFor(target->ID, target->subID)->getObjectInfo(target->appearance);
	CBankInfo * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());
	auto creatures = bankInfo->getPossibleCreaturesReward();
	uint64_t result = 0;

	for(auto c : creatures)
	{
		result += c.type->AIValue * c.count;
	}

	return result;
}

uint64_t getDwellingScore(const CGObjectInstance * target, bool checkGold)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	uint64_t score = 0;
	
	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.first && creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();
			auto creaturesAreFree = creature->level == 1;
			if(!creaturesAreFree && checkGold && !cb->getResourceAmount().canAfford(creature->cost * creLevel.first))
				continue;

			score += creature->AIValue * creLevel.first;
		}
	}

	return score;
}

int getDwellingArmyCost(const CGObjectInstance * target)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	int cost = 0;

	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.first && creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();
			auto creaturesAreFree = creature->level == 1;
			if(!creaturesAreFree)
				cost += creature->cost[Res::GOLD] * creLevel.first;
		}
	}

	return cost;
}

uint64_t evaluateArtifactArmyValue(CArtifactInstance * art)
{
	if(art->artType->id == ArtifactID::SPELL_SCROLL)
		return 1500;

	auto statsValue =
		4 * art->valOfBonuses(Bonus::LAND_MOVEMENT)
		+ 700 * art->valOfBonuses(Bonus::MORALE)
		+ 700 * art->getAttack(false)
		+ 700 * art->getDefence(false)
		+ 700 * art->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::KNOWLEDGE)
		+ 700 * art->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::SPELL_POWER)
		+ 700 * art->getDefence(false)
		+ 500 * art->valOfBonuses(Bonus::LUCK);

	auto classValue = 0;

	switch(art->artType->aClass)
	{
	case CArtifact::EartClass::ART_MINOR:
		classValue = 1000;
		break;

	case CArtifact::EartClass::ART_MAJOR:
		classValue = 3000;
		break;
	case CArtifact::EartClass::ART_RELIC:
	case CArtifact::EartClass::ART_SPECIAL:
		classValue = 8000;
		break;
	}

	return statsValue > classValue ? statsValue : classValue;
}

uint64_t RewardEvaluator::getArmyReward(
	const CGObjectInstance * target,
	const CGHeroInstance * hero,
	const CCreatureSet * army,
	bool checkGold) const
{
	const float enemyArmyEliminationRewardRatio = 0.5f;

	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::TOWN:
		return target->tempOwner == PlayerColor::NEUTRAL ? 1000 : 10000;
	case Obj::HILL_FORT:
		return ai->armyManager->calculateCreateresUpgrade(army, target, cb->getResourceAmount()).upgradeValue;
	case Obj::CREATURE_BANK:
		return getCreatureBankArmyReward(target, hero);
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR2:
	case Obj::CREATURE_GENERATOR3:
	case Obj::CREATURE_GENERATOR4:
		return getDwellingScore(target, checkGold);
	case Obj::CRYPT:
	case Obj::SHIPWRECK:
	case Obj::SHIPWRECK_SURVIVOR:
	case Obj::WARRIORS_TOMB:
		return 1000;
	case Obj::ARTIFACT:
		return evaluateArtifactArmyValue(dynamic_cast<const CGArtifact *>(target)->storedArtifact);
	case Obj::DRAGON_UTOPIA:
		return 10000;
	case Obj::HERO:
		return cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? enemyArmyEliminationRewardRatio * dynamic_cast<const CGHeroInstance *>(target)->getArmyStrength()
			: 0;
	default:
		return 0;
	}
}

int RewardEvaluator::getGoldCost(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const
{
	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::HILL_FORT:
		return ai->armyManager->calculateCreateresUpgrade(army, target, cb->getResourceAmount()).upgradeCost[Res::GOLD];
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
		return 1000;
	case Obj::UNIVERSITY:
		return 2000;
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR2:
	case Obj::CREATURE_GENERATOR3:
	case Obj::CREATURE_GENERATOR4:
		return getDwellingArmyCost(target);
	default:
		return 0;
	}
}

float RewardEvaluator::getEnemyHeroStrategicalValue(const CGHeroInstance * enemy) const
{
	auto objectsUnderTreat = ai->dangerHitMap->getOneTurnAccessibleObjects(enemy);
	float objectValue = 0;

	for(auto obj : objectsUnderTreat)
	{
		vstd::amax(objectValue, getStrategicalValue(obj));
	}

	return objectValue / 2.0f + enemy->level / 15.0f;
}

float RewardEvaluator::getResourceRequirementStrength(int resType) const
{
	TResources requiredResources = ai->buildAnalyzer->getResourcesRequiredNow();
	TResources dailyIncome = ai->buildAnalyzer->getDailyIncome();

	if(requiredResources[resType] == 0)
		return 0;

	if(dailyIncome[resType] == 0)
		return 1.0f;

	float ratio = (float)requiredResources[resType] / dailyIncome[resType] / 2;

	return std::min(ratio, 1.0f);
}

float RewardEvaluator::getTotalResourceRequirementStrength(int resType) const
{
	TResources requiredResources = ai->buildAnalyzer->getTotalResourcesRequired();
	TResources dailyIncome = ai->buildAnalyzer->getDailyIncome();

	if(requiredResources[resType] == 0)
		return 0;

	float ratio = dailyIncome[resType] == 0
		? requiredResources[resType] / 50
		: (float)requiredResources[resType] / dailyIncome[resType] / 50;

	return std::min(ratio, 1.0f);
}

float RewardEvaluator::getStrategicalValue(const CGObjectInstance * target) const
{
	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::MINE:
		return target->subID == Res::GOLD 
			? 0.5f 
			: 0.02f * getTotalResourceRequirementStrength(target->subID) + 0.02f * getResourceRequirementStrength(target->subID);

	case Obj::RESOURCE:
		return target->subID == Res::GOLD ? 0 : 0.1f * getResourceRequirementStrength(target->subID);

	case Obj::TOWN:
		return dynamic_cast<const CGTownInstance *>(target)->hasFort()
			? (target->tempOwner == PlayerColor::NEUTRAL ? 0.8f : 1.0f)
			: 0.5f;

	case Obj::HERO:
		return cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? getEnemyHeroStrategicalValue(dynamic_cast<const CGHeroInstance *>(target))
			: 0;

	default:
		return 0;
	}
}

float RewardEvaluator::evaluateWitchHutSkillScore(const CGWitchHut * hut, const CGHeroInstance * hero, HeroRole role) const
{
	if(!hut->wasVisited(hero->tempOwner))
		return role == HeroRole::SCOUT ? 2 : 0;

	auto skill = SecondarySkill(hut->ability);

	if(hero->getSecSkillLevel(skill) != SecSkillLevel::NONE
		|| hero->secSkills.size() >= GameConstants::SKILL_PER_HERO)
		return 0;

	auto score = ai->heroManager->evaluateSecSkill(skill, hero);

	return score >= 2 ? (role == HeroRole::MAIN ? 10 : 4) : score;
}

float RewardEvaluator::getSkillReward(const CGObjectInstance * target, const CGHeroInstance * hero, HeroRole role) const
{
	const float enemyHeroEliminationSkillRewardRatio = 0.5f;

	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::STAR_AXIS:
	case Obj::SCHOLAR:
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
	case Obj::GARDEN_OF_REVELATION:
	case Obj::MARLETTO_TOWER:
	case Obj::MERCENARY_CAMP:
	case Obj::SHRINE_OF_MAGIC_GESTURE:
	case Obj::SHRINE_OF_MAGIC_INCANTATION:
	case Obj::TREE_OF_KNOWLEDGE:
		return 1;
	case Obj::LEARNING_STONE:
		return 1.0f / std::sqrt(hero->level);
	case Obj::ARENA:
	case Obj::SHRINE_OF_MAGIC_THOUGHT:
		return 2;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		return 8;
	case Obj::WITCH_HUT:
		return evaluateWitchHutSkillScore(dynamic_cast<const CGWitchHut *>(target), hero, role);
	case Obj::HERO:
		return cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? enemyHeroEliminationSkillRewardRatio * dynamic_cast<const CGHeroInstance *>(target)->level
			: 0;
	default:
		return 0;
	}
}

uint64_t RewardEvaluator::getEnemyHeroDanger(const AIPath & path) const
{
	auto & treatNode = ai->dangerHitMap->getTileTreat(path.targetTile());

	if(treatNode.maximumDanger.danger == 0)
		return 0;
	
	if(treatNode.maximumDanger.turn <= path.turn())
		return treatNode.maximumDanger.danger;

	return treatNode.fastestDanger.turn <= path.turn() ? treatNode.fastestDanger.danger : 0;
}

int32_t getArmyCost(const CArmedInstance * army)
{
	int32_t value = 0;

	for(auto stack : army->Slots())
	{
		value += stack.second->getCreatureID().toCreature()->cost[Res::GOLD] * stack.second->count;
	}

	return value;
}

/// Gets aproximated reward in gold. Daily income is multiplied by 5
int32_t RewardEvaluator::getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero) const
{
	if(!target)
		return 0;

	const int dailyIncomeMultiplier = 5;
	const float enemyArmyEliminationGoldRewardRatio = 0.2f;
	const int32_t heroEliminationBonus = GameConstants::HERO_GOLD_COST / 2;
	auto isGold = target->subID == Res::GOLD; // TODO: other resorces could be sold but need to evaluate market power

	switch(target->ID)
	{
	case Obj::RESOURCE:
		return isGold ? 600 : 100;
	case Obj::TREASURE_CHEST:
		return 1500;
	case Obj::WATER_WHEEL:
		return 1000;
	case Obj::TOWN:
		return dailyIncomeMultiplier * estimateTownIncome(target, hero);
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
		return dailyIncomeMultiplier * (isGold ? 1000 : 75);
	case Obj::MYSTICAL_GARDEN:
	case Obj::WINDMILL:
		return 100;
	case Obj::CAMPFIRE:
		return 800;
	case Obj::WAGON:
		return 100;
	case Obj::CREATURE_BANK:
		return getCreatureBankResources(target, hero)[Res::GOLD];
	case Obj::CRYPT:
	case Obj::DERELICT_SHIP:
		return 3000;
	case Obj::DRAGON_UTOPIA:
		return 10000;
	case Obj::SEA_CHEST:
		return 1500;
	case Obj::HERO:
		return cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? heroEliminationBonus + enemyArmyEliminationGoldRewardRatio * getArmyCost(dynamic_cast<const CGHeroInstance *>(target))
			: 0;
	default:
		return 0;
	}
}

class ExecuteHeroChainEvaluationContextBuilder : public IEvaluationContextBuilder
{
public:
	virtual void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::EXECUTE_HERO_CHAIN)
			return;

		Goals::ExecuteHeroChain & chain = dynamic_cast<Goals::ExecuteHeroChain &>(*task);
		const AIPath & path = chain.getPath();

		vstd::amax(evaluationContext.danger, path.getTotalDanger());
		evaluationContext.movementCost += path.movementCost();
		evaluationContext.closestWayRatio = chain.closestWayRatio;

		std::map<const CGHeroInstance *, float> costsPerHero;

		for(auto & node : path.nodes)
		{
			vstd::amax(costsPerHero[node.targetHero], node.cost);
		}

		for(auto pair : costsPerHero)
		{
			auto role = evaluationContext.evaluator.ai->heroManager->getHeroRole(pair.first);

			evaluationContext.movementCostByRole[role] += pair.second;
		}

		auto heroPtr = task->hero;
		const CGObjectInstance * target = cb->getObj((ObjectInstanceID)task->objid, false);
		auto day = cb->getDate(Date::DAY);
		auto hero = heroPtr.get();
		bool checkGold = evaluationContext.danger == 0;
		auto army = path.heroArmy;

		vstd::amax(evaluationContext.armyLossPersentage, path.getTotalArmyLoss() / (double)path.getHeroStrength());
		evaluationContext.heroRole = evaluationContext.evaluator.ai->heroManager->getHeroRole(heroPtr);
		evaluationContext.goldReward += evaluationContext.evaluator.getGoldReward(target, hero);
		evaluationContext.armyReward += evaluationContext.evaluator.getArmyReward(target, hero, army, checkGold);
		evaluationContext.skillReward += evaluationContext.evaluator.getSkillReward(target, hero, evaluationContext.heroRole);
		evaluationContext.strategicalValue += evaluationContext.evaluator.getStrategicalValue(target);
		evaluationContext.goldCost += evaluationContext.evaluator.getGoldCost(target, hero, army);
		vstd::amax(evaluationContext.enemyHeroDangerRatio, evaluationContext.evaluator.getEnemyHeroDanger(path) / (double)path.getHeroStrength());
		vstd::amax(evaluationContext.turn, path.turn());
	}
};

class ClusterEvaluationContextBuilder : public IEvaluationContextBuilder
{
public:
	virtual void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::UNLOCK_CLUSTER)
			return;

		Goals::UnlockCluster & clusterGoal = dynamic_cast<Goals::UnlockCluster &>(*task);
		std::shared_ptr<ObjectCluster> cluster = clusterGoal.getCluster();

		auto hero = clusterGoal.hero.get();
		auto role = evaluationContext.evaluator.ai->heroManager->getHeroRole(clusterGoal.hero);

		std::vector<std::pair<const CGObjectInstance *, ObjectInfo>> objects(cluster->objects.begin(), cluster->objects.end());

		std::sort(objects.begin(), objects.end(), [](std::pair<const CGObjectInstance *, ObjectInfo> o1, std::pair<const CGObjectInstance *, ObjectInfo> o2) -> bool
		{
			return o1.second.priority > o2.second.priority;
		});

		int boost = 1;

		for(auto objInfo : objects)
		{
			auto target = objInfo.first;
			auto day = cb->getDate(Date::DAY);
			bool checkGold = objInfo.second.danger == 0;
			auto army = hero;

			evaluationContext.goldReward += evaluationContext.evaluator.getGoldReward(target, hero) / boost;
			evaluationContext.armyReward += evaluationContext.evaluator.getArmyReward(target, hero, army, checkGold) / boost;
			evaluationContext.skillReward += evaluationContext.evaluator.getSkillReward(target, hero, role) / boost;
			evaluationContext.strategicalValue += evaluationContext.evaluator.getStrategicalValue(target) / boost;
			evaluationContext.goldCost += evaluationContext.evaluator.getGoldCost(target, hero, army) / boost;
			evaluationContext.movementCostByRole[role] += objInfo.second.movementCost / boost;
			evaluationContext.movementCost += objInfo.second.movementCost / boost;

			vstd::amax(evaluationContext.turn, objInfo.second.turn / boost);

			boost <<= 1;

			if(boost > 8)
				break;
		}

		const AIPath & pathToCenter = clusterGoal.getPathToCenter();

	}
};

class BuildThisEvaluationContextBuilder : public IEvaluationContextBuilder
{
public:
	virtual void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::BUILD_STRUCTURE)
			return;

		Goals::BuildThis & buildThis = dynamic_cast<Goals::BuildThis &>(*task);
		auto & bi = buildThis.buildingInfo;
		
		evaluationContext.goldReward += 7 * bi.dailyIncome[Res::GOLD] / 2; // 7 day income but half we already have
		evaluationContext.heroRole = HeroRole::MAIN;
		evaluationContext.movementCostByRole[evaluationContext.heroRole] += bi.prerequisitesCount;
		evaluationContext.strategicalValue += buildThis.townInfo.armyScore / 50000.0;
		evaluationContext.goldCost += bi.buildCostWithPrerequisits[Res::GOLD];

		if(bi.creatureID != CreatureID::NONE)
		{
			if(bi.baseCreatureID == bi.creatureID)
			{
				evaluationContext.strategicalValue += 0.5f + 0.1f * bi.creatureLevel / (float)bi.prerequisitesCount;
				evaluationContext.armyReward += evaluationContext.evaluator.ai->armyManager->evaluateStackPower(bi.creatureID.toCreature(), bi.creatureGrows);
			}
			else
			{
				evaluationContext.strategicalValue += 0.05f * bi.creatureLevel / (float)bi.prerequisitesCount;
				evaluationContext.armyReward += evaluationContext.evaluator.getUpgradeArmyReward(buildThis.town, bi);
			}
		}
		else
		{
			evaluationContext.strategicalValue += evaluationContext.evaluator.ai->buildAnalyzer->getGoldPreasure() * evaluationContext.goldReward / 2200.0f;
		}
	}
};

uint64_t RewardEvaluator::getUpgradeArmyReward(const CGTownInstance * town, const BuildingInfo & bi) const
{
	if(ai->buildAnalyzer->hasAnyBuilding(town->alignment, bi.id))
		return 0;

	auto creaturesToUpgrade = ai->armyManager->getTotalCreaturesAvailable(bi.baseCreatureID);
	auto upgradedPower = ai->armyManager->evaluateStackPower(bi.creatureID.toCreature(), creaturesToUpgrade.count);

	return upgradedPower - creaturesToUpgrade.power;
}

PriorityEvaluator::PriorityEvaluator(const Nullkiller * ai)
	:ai(ai)
{
	initVisitTile();
	evaluationContextBuilders.push_back(std::make_shared<ExecuteHeroChainEvaluationContextBuilder>());
	evaluationContextBuilders.push_back(std::make_shared<BuildThisEvaluationContextBuilder>());
	evaluationContextBuilders.push_back(std::make_shared<ClusterEvaluationContextBuilder>());
}

EvaluationContext PriorityEvaluator::buildEvaluationContext(Goals::TSubgoal goal) const
{
	Goals::TGoalVec parts;
	EvaluationContext context(ai);

	if(goal->goalType == Goals::COMPOSITION)
	{
		parts = goal->decompose();
	}
	else
	{
		parts.push_back(goal);
	}

	for(auto subgoal : parts)
	{
		context.strategicalValue += subgoal->strategicalValue;
		context.goldCost += subgoal->goldCost;

		for(auto builder : evaluationContextBuilders)
		{
			builder->buildEvaluationContext(context, subgoal);
		}
	}

	return context;
}

/// distance
/// nearest hero?
/// gold income
/// army income
/// hero strength - hero skills
/// danger
/// importance
float PriorityEvaluator::evaluate(Goals::TSubgoal task)
{
	auto evaluationContext = buildEvaluationContext(task);

	int rewardType = (evaluationContext.goldReward > 0 ? 1 : 0) 
		+ (evaluationContext.armyReward > 0 ? 1 : 0)
		+ (evaluationContext.skillReward > 0 ? 1 : 0)
		+ (evaluationContext.strategicalValue > 0 ? 1 : 0);
	
	double result = 0;

	try
	{
		armyLossPersentageVariable->setValue(evaluationContext.armyLossPersentage);
		heroRoleVariable->setValue(evaluationContext.heroRole);
		mainTurnDistanceVariable->setValue(evaluationContext.movementCostByRole[HeroRole::MAIN]);
		scoutTurnDistanceVariable->setValue(evaluationContext.movementCostByRole[HeroRole::SCOUT]);
		goldRewardVariable->setValue(evaluationContext.goldReward);
		armyRewardVariable->setValue(evaluationContext.armyReward);
		skillRewardVariable->setValue(evaluationContext.skillReward);
		dangerVariable->setValue(evaluationContext.danger);
		rewardTypeVariable->setValue(rewardType);
		closestHeroRatioVariable->setValue(evaluationContext.closestWayRatio);
		strategicalValueVariable->setValue(evaluationContext.strategicalValue);
		goldPreasureVariable->setValue(ai->buildAnalyzer->getGoldPreasure());
		goldCostVariable->setValue(evaluationContext.goldCost / ((float)cb->getResourceAmount(Res::GOLD) + (float)ai->buildAnalyzer->getDailyIncome()[Res::GOLD] + 1.0f));
		turnVariable->setValue(evaluationContext.turn);
		fearVariable->setValue(evaluationContext.enemyHeroDangerRatio);

		engine->process();
		//engine.process(VISIT_TILE); //TODO: Process only Visit_Tile
		result = value->getValue();
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate VisitTile: %s", fe.getWhat());
	}
	assert(result >= 0);

#ifdef AI_TRACE_LEVEL >= 1
	logAi->trace("Evaluated %s, loss: %f, turn: %d, turns main: %f, scout: %f, gold: %d, cost: %d, army gain: %d, danger: %d, role: %s, strategical value: %f, cwr: %f, fear: %f, result %f",
		task->toString(),
		evaluationContext.armyLossPersentage,
		(int)evaluationContext.turn,
		evaluationContext.movementCostByRole[HeroRole::MAIN],
		evaluationContext.movementCostByRole[HeroRole::SCOUT],
		evaluationContext.goldReward,
		evaluationContext.goldCost,
		evaluationContext.armyReward,
		evaluationContext.danger,
		evaluationContext.heroRole == HeroRole::MAIN ? "main" : "scout",
		evaluationContext.strategicalValue,
		evaluationContext.closestWayRatio,
		evaluationContext.enemyHeroDangerRatio,
		result);
#endif

	return result;
}
