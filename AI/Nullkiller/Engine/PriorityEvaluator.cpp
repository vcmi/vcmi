/*
 * Fuzzy.cpp, part of VCMI engine
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
#include "../VCAI.h"
#include "../AIhelper.h"

#define MIN_AI_STRENGHT (0.5f) //lower when combat AI gets smarter
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 100 times weaker than us

struct BankConfig;
class CBankInfo;
class Engine;
class InputVariable;
class CGTownInstance;

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

PriorityEvaluator::PriorityEvaluator()
{
	initVisitTile();
}

PriorityEvaluator::~PriorityEvaluator()
{
	delete engine;
}

void PriorityEvaluator::initVisitTile()
{
	auto file = CResourceHandler::get("initial")->load(ResourceID("config/ai-priorities.txt"))->readAll();
	std::string str = std::string((char *)file.first.get(), file.second);
	engine = fl::FllImporter().fromString(str);
	armyLossPersentageVariable = engine->getInputVariable("armyLoss");
	heroRoleVariable = engine->getInputVariable("heroRole");
	dangerVariable = engine->getInputVariable("danger");
	turnDistanceVariable = engine->getInputVariable("turnDistance");
	goldRewardVariable = engine->getInputVariable("goldReward");
	armyRewardVariable = engine->getInputVariable("armyReward");
	skillRewardVariable = engine->getInputVariable("skillReward");
	rewardTypeVariable = engine->getInputVariable("rewardType");
	closestHeroRatioVariable = engine->getInputVariable("closestHeroRatio");
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
			if(checkGold &&	!cb->getResourceAmount().canAfford(creature->cost * creLevel.first))
				continue;

			score += creature->AIValue * creLevel.first;
		}
	}

	return score;
}

uint64_t getArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero, bool checkGold)
{
	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::TOWN:
		return target->tempOwner == PlayerColor::NEUTRAL ? 1000 : 10000;
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
		return 1500;
	case Obj::ARTIFACT:
		return dynamic_cast<const CGArtifact *>(target)->storedArtifact-> artType->getArtClassSerial() * 300;
	case Obj::DRAGON_UTOPIA:
		return 10000;
	default:
		return 0;
	}
}

float evaluateWitchHutSkillScore(const CGWitchHut * hut, const CGHeroInstance * hero, HeroRole role)
{
	if(!hut->wasVisited(hero->tempOwner))
		return role == HeroRole::SCOUT ? 2 : 0;

	auto skill = SecondarySkill(hut->ability);

	if(hero->getSecSkillLevel(skill) != SecSkillLevel::NONE
		|| hero->secSkills.size() >= GameConstants::SKILL_PER_HERO)
		return 0;

	auto score = ai->ah->evaluateSecSkill(skill, hero);

	return score >= 2 ? (role == HeroRole::MAIN ? 10 : 4) : score;
}

float getSkillReward(const CGObjectInstance * target, const CGHeroInstance * hero, HeroRole role)
{
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
		return 1;
	case Obj::ARENA:
	case Obj::SHRINE_OF_MAGIC_THOUGHT:
		return 2;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		return 8;
	case Obj::WITCH_HUT:
		return evaluateWitchHutSkillScore(dynamic_cast<const CGWitchHut *>(target), hero, role);
	default:
		return 0;
	}
}

/// Gets aproximated reward in gold. Daily income is multiplied by 5
int32_t getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	if(!target)
		return 0;

	const int dailyIncomeMultiplier = 5;
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
	case Obj::CREATURE_BANK:
		return getCreatureBankResources(target, hero)[Res::GOLD];
	case Obj::CRYPT:
	case Obj::DERELICT_SHIP:
		return 3000;
	case Obj::DRAGON_UTOPIA:
		return 10000;
	case Obj::SEA_CHEST:
		return 1500;
	default:
		return 0;
	}
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
	if(task->priority > 0)
		return task->priority;

	auto heroPtr = task->hero;

	if(!heroPtr.validAndSet())
		return 2; 

	int objId = task->objid;

	if(task->parent)
		objId = task->parent->objid;

	const CGObjectInstance * target = cb->getObj((ObjectInstanceID)objId, false);
	
	auto day = cb->getDate(Date::DAY);
	auto hero = heroPtr.get();
	auto armyTotal = task->evaluationContext.heroStrength;
	double armyLossPersentage = task->evaluationContext.armyLoss / (double)armyTotal;
	uint64_t danger = task->evaluationContext.danger;
	HeroRole heroRole = ai->ah->getHeroRole(heroPtr);
	int32_t goldReward = getGoldReward(target, hero);
	bool checkGold = danger == 0;
	uint64_t armyReward = getArmyReward(target, hero, checkGold);
	float skillReward = getSkillReward(target, hero, heroRole);
	double result = 0;
	int rewardType = (goldReward > 0 ? 1 : 0) + (armyReward > 0 ? 1 : 0) + (skillReward > 0 ? 1 : 0);
	
	try
	{
		armyLossPersentageVariable->setValue(armyLossPersentage);
		heroRoleVariable->setValue(heroRole);
		turnDistanceVariable->setValue(task->evaluationContext.movementCost);
		goldRewardVariable->setValue(goldReward);
		armyRewardVariable->setValue(armyReward);
		skillRewardVariable->setValue(skillReward);
		dangerVariable->setValue(danger);
		rewardTypeVariable->setValue(rewardType);
		closestHeroRatioVariable->setValue(task->evaluationContext.closestWayRatio);

		engine->process();
		//engine.process(VISIT_TILE); //TODO: Process only Visit_Tile
		result = value->getValue();
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate VisitTile: %s", fe.getWhat());
	}
	assert(result >= 0);

#ifdef VCMI_TRACE_PATHFINDER
	logAi->trace("Evaluated %s, hero %s, loss: %f, turns: %f, gold: %d, army gain: %d, danger: %d, role: %s, result %f",
		task->name(),
		hero->name,
		armyLossPersentage,
		task->evaluationContext.movementCost,
		goldReward,
		armyReward,
		danger,
		heroRole ? "scout" : "main",
		result);
#endif

	return result;
}
