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
#include "../VCAI.h"

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
	configure();
}

PriorityEvaluator::~PriorityEvaluator()
{
	engine.removeRuleBlock(0);
	engine.~Engine();
}

void PriorityEvaluator::initVisitTile()
{
	try
	{
		armyLossPersentageVariable = new fl::InputVariable("armyLoss"); //hero must be strong enough to defeat guards
		heroStrengthVariable = new fl::InputVariable("heroStrength"); //we want to use weakest possible hero
		turnDistanceVariable = new fl::InputVariable("turnDistance"); //we want to use hero who is near
		goldRewardVariable = new fl::InputVariable("goldReward"); //indicate AI that content of the file is important or it is probably bad
		armyRewardVariable = new fl::InputVariable("armyReward"); //indicate AI that content of the file is important or it is probably bad
		value = new fl::OutputVariable("Value");
		value->setMinimum(0);
		value->setMaximum(1);
		value->setAggregation(new fl::AlgebraicSum());
		value->setDefuzzifier(new fl::Centroid(100));
		value->setDefaultValue(0.500);

		rules.setConjunction(new fl::AlgebraicProduct());
		rules.setDisjunction(new fl::AlgebraicSum());
		rules.setImplication(new fl::AlgebraicProduct());
		rules.setActivation(new fl::General());

		std::vector<fl::InputVariable *> helper = {
			armyLossPersentageVariable,
			heroStrengthVariable,
			turnDistanceVariable,
			goldRewardVariable,
			armyRewardVariable };

		for(auto val : helper)
		{
			engine.addInputVariable(val);
		}
		engine.addOutputVariable(value);

		armyLossPersentageVariable->addTerm(new fl::Ramp("LOW", 0.200, 0.000));
		armyLossPersentageVariable->addTerm(new fl::Ramp("HIGH", 0.200, 0.500));
		armyLossPersentageVariable->setRange(0, 1);

		//strength compared to our main hero
		heroStrengthVariable->addTerm(new fl::Ramp("LOW", 0.2, 0));
		heroStrengthVariable->addTerm(new fl::Triangle("MEDIUM", 0.2, 0.8));
		heroStrengthVariable->addTerm(new fl::Ramp("HIGH", 0.5, 1));
		heroStrengthVariable->setRange(0.0, 1.0);

		turnDistanceVariable->addTerm(new fl::Ramp("SMALL", 1.000, 0.000));
		turnDistanceVariable->addTerm(new fl::Triangle("MEDIUM", 0.000, 1.000, 2.000));
		turnDistanceVariable->addTerm(new fl::Ramp("LONG", 1.000, 3.000));
		turnDistanceVariable->setRange(0, 3);

		goldRewardVariable->addTerm(new fl::Ramp("LOW", 2000.000, 0.000));
		goldRewardVariable->addTerm(new fl::Triangle("MEDIUM", 0.000, 2000.000, 3500.000));
		goldRewardVariable->addTerm(new fl::Ramp("HIGH", 2000.000, 5000.000));
		goldRewardVariable->setRange(0.0, 5000.0);

		armyRewardVariable->addTerm(new fl::Ramp("LOW", 0.300, 0.000));
		armyRewardVariable->addTerm(new fl::Triangle("MEDIUM", 0.100, 0.400, 0.800));
		armyRewardVariable->addTerm(new fl::Ramp("HIGH", 0.400, 1.000));
		armyRewardVariable->setRange(0.0, 1.0);

		value->addTerm(new fl::Ramp("LOWEST", 0.150, 0.000));
		value->addTerm(new fl::Triangle("LOW", 0.100, 0.100, 0.250, 0.500));
		value->addTerm(new fl::Triangle("BITLOW", 0.200, 0.200, 0.350, 0.250));
		value->addTerm(new fl::Triangle("MEDIUM", 0.300, 0.500, 0.700, 0.050));
		value->addTerm(new fl::Triangle("BITHIGH", 0.650, 0.800, 0.800, 0.250));
		value->addTerm(new fl::Triangle("HIGH", 0.750, 0.900, 0.900, 0.500));
		value->addTerm(new fl::Ramp("HIGHEST", 0.850, 1.000));
		value->setRange(0.0, 1.0);

		//we may want to use secondary hero(es) rather than main hero

		//do not cancel important goals
		//addRule("if lockedMissionImportance is HIGH then Value is very LOW");
		//addRule("if lockedMissionImportance is MEDIUM then Value is somewhat LOW");
		//addRule("if lockedMissionImportance is LOW then Value is HIGH");

		//pick nearby objects if it's easy, avoid long walks
		/*addRule("if turnDistance is SMALL then Value is somewhat HIGH");
		addRule("if turnDistance is MEDIUM then Value is MEDIUM");
		addRule("if turnDistance is LONG then Value is LOW");*/

		//some goals are more rewarding by definition f.e. capturing town is more important than collecting resource - experimental
		addRule("if turnDistance is MEDIUM then Value is MEDIUM");
		addRule("if turnDistance is SMALL then Value is BITHIGH");
		addRule("if turnDistance is LONG then Value is BITLOW");
		addRule("if turnDistance is very LONG then Value is LOW");
		addRule("if goldReward is LOW then Value is MEDIUM");
		addRule("if goldReward is MEDIUM and armyLoss is LOW then Value is BITHIGH");
		addRule("if goldReward is HIGH and armyLoss is LOW then Value is HIGH");
		addRule("if armyReward is LOW then Value is MEDIUM");
		addRule("if armyReward is MEDIUM and armyLoss is LOW then Value is BITHIGH");
		addRule("if armyReward is HIGH and armyLoss is LOW then Value is HIGHEST with 0.5");
		addRule("if armyReward is HIGH and goldReward is HIGH and armyLoss is LOW then Value is HIGHEST");
		addRule("if armyReward is HIGH and goldReward is MEDIUM and armyLoss is LOW then Value is HIGHEST with 0.8");
		addRule("if armyReward is MEDIUM and goldReward is HIGH and armyLoss is LOW then Value is HIGHEST with 0.5");
		addRule("if armyReward is MEDIUM and goldReward is MEDIUM and armyLoss is LOW then Value is HIGH");
		addRule("if armyReward is HIGH and turnDistance is SMALL and armyLoss is LOW then Value is HIGHEST");
		addRule("if goldReward is HIGH and turnDistance is SMALL and armyLoss is LOW then Value is HIGHEST");
		addRule("if armyReward is MEDIUM and turnDistance is SMALL and armyLoss is LOW then Value is HIGH");
		addRule("if goldReward is MEDIUM and turnDistance is SMALL and armyLoss is LOW then Value is BITHIGH");
		addRule("if goldReward is LOW and armyReward is LOW and turnDistance is not SMALL then Value is LOWEST");
		addRule("if armyLoss is HIGH then Value is LOWEST");
		addRule("if armyLoss is LOW then Value is MEDIUM");
		addRule("if armyReward is LOW and turnDistance is LONG then Value is LOWEST");
	}
	catch(fl::Exception & fe)
	{
		logAi->error("visitTile: %s", fe.getWhat());
	}
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

uint64_t getDwellingScore(const CGObjectInstance * target)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	uint64_t score = 0;
	
	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.first && creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();
			if(cb->getResourceAmount().canAfford(creature->cost * creLevel.first))
			{
				score += creature->AIValue * creLevel.first;
			}
		}
	}

	return score;
}

uint64_t getArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	if(!target)
		return 0;

	const int dailyIncomeMultiplier = 5;

	switch(target->ID)
	{
	case Obj::TOWN:
		return target->tempOwner == PlayerColor::NEUTRAL ? 5000 : 1000;
	case Obj::CREATURE_BANK:
		return getCreatureBankArmyReward(target, hero);
	case Obj::CREATURE_GENERATOR1:
		return getDwellingScore(target) * dailyIncomeMultiplier;
	case Obj::CRYPT:
	case Obj::SHIPWRECK:
	case Obj::SHIPWRECK_SURVIVOR:
		return 1500;
	case Obj::ARTIFACT:
		return dynamic_cast<const CGArtifact *>(target)->storedArtifact-> artType->getArtClassSerial() == CArtifact::ART_MAJOR ? 3000 : 1500;
	case Obj::DRAGON_UTOPIA:
		return 10000;
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
		return isGold ? 800 : 100;
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
		return 900;
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
	auto heroPtr = task->hero;

	if(!heroPtr.validAndSet())
		return 2; 

	int objId = task->objid;

	if(task->parent)
		objId = task->parent->objid;

	const CGObjectInstance * target = cb->getObj((ObjectInstanceID)objId, false);
	
	auto hero = heroPtr.get();
	auto armyTotal = task->evaluationContext.heroStrength;
	double armyLossPersentage = task->evaluationContext.armyLoss / (double)armyTotal;
	int32_t goldReward = getGoldReward(target, hero);
	uint64_t armyReward = getArmyReward(target, hero);
	double result = 0;

	try
	{
		armyLossPersentageVariable->setValue(armyLossPersentage);
		heroStrengthVariable->setValue((fl::scalar)hero->getTotalStrength() / ai->primaryHero()->getTotalStrength());
		turnDistanceVariable->setValue(task->evaluationContext.movementCost);
		goldRewardVariable->setValue(goldReward);
		armyRewardVariable->setValue(armyReward / 10000.0);

		engine.process();
		//engine.process(VISIT_TILE); //TODO: Process only Visit_Tile
		result = value->getValue() / task->evaluationContext.closestWayRatio;
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate VisitTile: %s", fe.getWhat());
	}
	assert(result >= 0);

#ifdef VCMI_TRACE_PATHFINDER
	logAi->trace("Evaluated %s, loss: %f, turns: %f, gold: %d, army gain: %d, result %f",
		task->name(),
		armyLossPersentage,
		task->evaluationContext.movementCost,
		goldReward,
		armyReward,
		result);
#endif

	return result;
}
