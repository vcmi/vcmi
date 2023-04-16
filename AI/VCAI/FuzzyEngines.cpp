/*
* FuzzyEngines.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "FuzzyEngines.h"
#include "Goals/Goals.h"

#include "../../lib/mapObjects/MapObjects.h"
#include "VCAI.h"
#include "MapObjectsEvaluator.h"

#define MIN_AI_STRENGTH (0.5f) //lower when combat AI gets smarter
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 100 times weaker than us

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

engineBase::engineBase()
{
	rules = new fl::RuleBlock();
	engine.addRuleBlock(rules);
}

void engineBase::configure()
{
	engine.configure("Minimum", "Maximum", "Minimum", "AlgebraicSum", "Centroid", "Proportional");
	logAi->trace(engine.toString());
}

void engineBase::addRule(const std::string & txt)
{
	rules->addRule(fl::Rule::parse(txt, &engine));
}

struct armyStructure
{
	float walkers, shooters, flyers;
	ui32 maxSpeed;
};

armyStructure evaluateArmyStructure(const CArmedInstance * army)
{
	ui64 totalStrength = army->getArmyStrength();
	double walkersStrength = 0;
	double flyersStrength = 0;
	double shootersStrength = 0;
	ui32 maxSpeed = 0;

	static const CSelector selectorSHOOTER = Selector::type()(Bonus::SHOOTER);
	static const std::string keySHOOTER = "type_"+std::to_string((int32_t)Bonus::SHOOTER);

	static const CSelector selectorFLYING = Selector::type()(Bonus::FLYING);
	static const std::string keyFLYING = "type_"+std::to_string((int32_t)Bonus::FLYING);

	static const CSelector selectorSTACKS_SPEED = Selector::type()(Bonus::STACKS_SPEED);
	static const std::string keySTACKS_SPEED = "type_"+std::to_string((int32_t)Bonus::STACKS_SPEED);

	for(auto s : army->Slots())
	{
		bool walker = true;
		auto bearer = s.second->getType()->getBonusBearer();
		if(bearer->hasBonus(selectorSHOOTER, keySHOOTER))
		{
			shootersStrength += s.second->getPower();
			walker = false;
		}
		if(bearer->hasBonus(selectorFLYING, keyFLYING))
		{
			flyersStrength += s.second->getPower();
			walker = false;
		}
		if(walker)
			walkersStrength += s.second->getPower();

		vstd::amax(maxSpeed, bearer->valOfBonuses(selectorSTACKS_SPEED, keySTACKS_SPEED));
	}
	armyStructure as;
	as.walkers = static_cast<float>(walkersStrength / totalStrength);
	as.shooters = static_cast<float>(shootersStrength / totalStrength);
	as.flyers = static_cast<float>(flyersStrength / totalStrength);
	as.maxSpeed = maxSpeed;
	assert(as.walkers || as.flyers || as.shooters);
	return as;
}

float HeroMovementGoalEngineBase::calculateTurnDistanceInputValue(const Goals::AbstractGoal & goal) const
{
	if(goal.evaluationContext.movementCost > 0)
	{
		return goal.evaluationContext.movementCost;
	}
	else
	{
		auto pathInfo = ai->myCb->getPathsInfo(goal.hero.h)->getPathInfo(goal.tile);
		return pathInfo->getCost();
	}
}

TacticalAdvantageEngine::TacticalAdvantageEngine()
{
	try
	{
		ourShooters = new fl::InputVariable("OurShooters");
		ourWalkers = new fl::InputVariable("OurWalkers");
		ourFlyers = new fl::InputVariable("OurFlyers");
		enemyShooters = new fl::InputVariable("EnemyShooters");
		enemyWalkers = new fl::InputVariable("EnemyWalkers");
		enemyFlyers = new fl::InputVariable("EnemyFlyers");

		//Tactical advantage calculation
		std::vector<fl::InputVariable *> helper =
		{
			ourShooters, ourWalkers, ourFlyers, enemyShooters, enemyWalkers, enemyFlyers
		};

		for(auto val : helper)
		{
			engine.addInputVariable(val);
			val->addTerm(new fl::Ramp("FEW", 0.6, 0.0));
			val->addTerm(new fl::Ramp("MANY", 0.4, 1));
			val->setRange(0.0, 1.0);
		}

		ourSpeed = new fl::InputVariable("OurSpeed");
		enemySpeed = new fl::InputVariable("EnemySpeed");

		helper = { ourSpeed, enemySpeed };

		for(auto val : helper)
		{
			engine.addInputVariable(val);
			val->addTerm(new fl::Ramp("LOW", 6.5, 3));
			val->addTerm(new fl::Triangle("MEDIUM", 5.5, 10.5));
			val->addTerm(new fl::Ramp("HIGH", 8.5, 16));
			val->setRange(0, 25);
		}

		castleWalls = new fl::InputVariable("CastleWalls");
		engine.addInputVariable(castleWalls);
		{
			auto * none = new fl::Rectangle("NONE", CGTownInstance::NONE, CGTownInstance::NONE + (CGTownInstance::FORT - CGTownInstance::NONE) * 0.5f);
			castleWalls->addTerm(none);

			auto * medium = new fl::Trapezoid("MEDIUM", (CGTownInstance::FORT - CGTownInstance::NONE) * 0.5f, CGTownInstance::FORT,
				CGTownInstance::CITADEL, CGTownInstance::CITADEL + (CGTownInstance::CASTLE - CGTownInstance::CITADEL) * 0.5f);
			castleWalls->addTerm(medium);

			auto * high = new fl::Ramp("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE);
			castleWalls->addTerm(high);

			castleWalls->setRange(CGTownInstance::NONE, CGTownInstance::CASTLE);
		}


		bankPresent = new fl::InputVariable("Bank");
		engine.addInputVariable(bankPresent);
		{
			auto * termFalse = new fl::Rectangle("FALSE", 0.0, 0.5f);
			bankPresent->addTerm(termFalse);
			auto * termTrue = new fl::Rectangle("TRUE", 0.5f, 1);
			bankPresent->addTerm(termTrue);
			bankPresent->setRange(0, 1);
		}

		threat = new fl::OutputVariable("Threat");
		engine.addOutputVariable(threat);
		threat->addTerm(new fl::Ramp("LOW", 1, MIN_AI_STRENGTH));
		threat->addTerm(new fl::Triangle("MEDIUM", 0.8, 1.2));
		threat->addTerm(new fl::Ramp("HIGH", 1, 1.5));
		threat->setRange(MIN_AI_STRENGTH, 1.5);

		addRule("if OurShooters is MANY and EnemySpeed is LOW then Threat is LOW");
		addRule("if OurShooters is MANY and EnemyShooters is FEW then Threat is LOW");
		addRule("if OurSpeed is LOW and EnemyShooters is MANY then Threat is HIGH");
		addRule("if OurSpeed is HIGH and EnemyShooters is MANY then Threat is LOW");

		addRule("if OurWalkers is FEW and EnemyShooters is MANY then Threat is LOW");
		addRule("if OurShooters is MANY and EnemySpeed is HIGH then Threat is HIGH");
		//just to cover all cases
		addRule("if OurShooters is FEW and EnemySpeed is HIGH then Threat is MEDIUM");
		addRule("if EnemySpeed is MEDIUM then Threat is MEDIUM");
		addRule("if EnemySpeed is LOW and OurShooters is FEW then Threat is MEDIUM");

		addRule("if Bank is TRUE and OurShooters is MANY then Threat is HIGH");
		addRule("if Bank is TRUE and EnemyShooters is MANY then Threat is LOW");

		addRule("if CastleWalls is HIGH and OurWalkers is MANY then Threat is HIGH");
		addRule("if CastleWalls is HIGH and OurFlyers is MANY and OurShooters is MANY then Threat is MEDIUM");
		addRule("if CastleWalls is MEDIUM and OurShooters is MANY and EnemyWalkers is MANY then Threat is LOW");

	}
	catch(fl::Exception & pe)
	{
		logAi->error("initTacticalAdvantage: %s", pe.getWhat());
	}
	configure();
}

float TacticalAdvantageEngine::getTacticalAdvantage(const CArmedInstance * we, const CArmedInstance * enemy)
{
	float output = 1;
	/*try //TODO: rework this engine, it tends to produce nonsense output
	{
		armyStructure ourStructure = evaluateArmyStructure(we);
		armyStructure enemyStructure = evaluateArmyStructure(enemy);

		ourWalkers->setValue(ourStructure.walkers);
		ourShooters->setValue(ourStructure.shooters);
		ourFlyers->setValue(ourStructure.flyers);
		ourSpeed->setValue(ourStructure.maxSpeed);

		enemyWalkers->setValue(enemyStructure.walkers);
		enemyShooters->setValue(enemyStructure.shooters);
		enemyFlyers->setValue(enemyStructure.flyers);
		enemySpeed->setValue(enemyStructure.maxSpeed);

		bool bank = dynamic_cast<const CBank *>(enemy);
		if(bank)
			bankPresent->setValue(1);
		else
			bankPresent->setValue(0);

		const CGTownInstance * fort = dynamic_cast<const CGTownInstance *>(enemy);
		if(fort)
			castleWalls->setValue(fort->fortLevel());
		else
			castleWalls->setValue(0);

		engine.process();
		output = threat->getValue();
	}
	catch(fl::Exception & fe)
	{
		logAi->error("getTacticalAdvantage: %s ", fe.getWhat());
	}

	if(output < 0 || (output != output))
	{
		fl::InputVariable * tab[] = { bankPresent, castleWalls, ourWalkers, ourShooters, ourFlyers, ourSpeed, enemyWalkers, enemyShooters, enemyFlyers, enemySpeed };
		std::string names[] = { "bankPresent", "castleWalls", "ourWalkers", "ourShooters", "ourFlyers", "ourSpeed", "enemyWalkers", "enemyShooters", "enemyFlyers", "enemySpeed" };
		std::stringstream log("Warning! Fuzzy engine doesn't cover this set of parameters: ");

		for(int i = 0; i < boost::size(tab); i++)
			log << names[i] << ": " << tab[i]->getValue() << " ";
		logAi->error(log.str());
		assert(false);
	}*/

	return output;
}

//std::shared_ptr<AbstractGoal> chooseSolution (std::vector<std::shared_ptr<AbstractGoal>> & vec)

HeroMovementGoalEngineBase::HeroMovementGoalEngineBase()
{
	try
	{
		strengthRatio = new fl::InputVariable("strengthRatio"); //hero must be strong enough to defeat guards
		heroStrength = new fl::InputVariable("heroStrength"); //we want to use weakest possible hero
		turnDistance = new fl::InputVariable("turnDistance"); //we want to use hero who is near
		missionImportance = new fl::InputVariable("lockedMissionImportance"); //we may want to preempt hero with low-priority mission
		value = new fl::OutputVariable("Value");
		value->setMinimum(0);
		value->setMaximum(5);

		std::vector<fl::InputVariable *> helper = { strengthRatio, heroStrength, turnDistance, missionImportance };
		for(auto val : helper)
		{
			engine.addInputVariable(val);
		}
		engine.addOutputVariable(value);

		strengthRatio->addTerm(new fl::Ramp("LOW", SAFE_ATTACK_CONSTANT, 0));
		strengthRatio->addTerm(new fl::Ramp("HIGH", SAFE_ATTACK_CONSTANT, SAFE_ATTACK_CONSTANT * 3));
		strengthRatio->setRange(0, SAFE_ATTACK_CONSTANT * 3);

		//strength compared to our main hero
		heroStrength->addTerm(new fl::Ramp("LOW", 0.5, 0));
		heroStrength->addTerm(new fl::Triangle("MEDIUM", 0.2, 0.8));
		heroStrength->addTerm(new fl::Ramp("HIGH", 0.5, 1));
		heroStrength->setRange(0.0, 1.0);

		turnDistance->addTerm(new fl::Ramp("SHORT", 0.5, 0));
		turnDistance->addTerm(new fl::Triangle("MEDIUM", 0.1, 0.8));
		turnDistance->addTerm(new fl::Ramp("LONG", 0.5, 10));
		turnDistance->setRange(0.0, 10.0);

		missionImportance->addTerm(new fl::Ramp("LOW", 2.5, 0));
		missionImportance->addTerm(new fl::Triangle("MEDIUM", 2, 3));
		missionImportance->addTerm(new fl::Ramp("HIGH", 2.5, 5));
		missionImportance->setRange(0.0, 5.0);

		//an issue: in 99% cases this outputs center of mass (2.5) regardless of actual input :/
		//should be same as "mission Importance" to keep consistency
		value->addTerm(new fl::Ramp("LOW", 2.5, 0));
		value->addTerm(new fl::Triangle("MEDIUM", 2, 3)); //can't be center of mass :/
		value->addTerm(new fl::Ramp("HIGH", 2.5, 5));
		value->setRange(0.0, 5.0);

		//use unarmed scouts if possible
		addRule("if strengthRatio is HIGH and heroStrength is LOW then Value is HIGH");
		//we may want to use secondary hero(es) rather than main hero
		addRule("if strengthRatio is HIGH and heroStrength is MEDIUM then Value is MEDIUM");
		addRule("if strengthRatio is HIGH and heroStrength is HIGH then Value is LOW");
		//don't assign targets to heroes who are too weak, but prefer targets of our main hero (in case we need to gather army)
		addRule("if strengthRatio is LOW and heroStrength is LOW then Value is LOW");
		//attempt to arm secondary heroes is not stupid
		addRule("if strengthRatio is LOW and heroStrength is MEDIUM then Value is HIGH");
		addRule("if strengthRatio is LOW and heroStrength is HIGH then Value is LOW");

		//do not cancel important goals
		addRule("if lockedMissionImportance is HIGH then Value is LOW");
		addRule("if lockedMissionImportance is MEDIUM then Value is MEDIUM");
		addRule("if lockedMissionImportance is LOW then Value is HIGH");
		//pick nearby objects if it's easy, avoid long walks
		addRule("if turnDistance is SHORT then Value is HIGH");
		addRule("if turnDistance is MEDIUM then Value is MEDIUM");
		addRule("if turnDistance is LONG then Value is LOW");
	}
	catch(fl::Exception & fe)
	{
		logAi->error("HeroMovementGoalEngineBase: %s", fe.getWhat());
	}
}

void HeroMovementGoalEngineBase::setSharedFuzzyVariables(Goals::AbstractGoal & goal)
{
	float turns = calculateTurnDistanceInputValue(goal);
	float missionImportanceData = 0;

	if(vstd::contains(ai->lockedHeroes, goal.hero))
	{
		missionImportanceData = ai->lockedHeroes[goal.hero]->priority;
	}
	else if(goal.parent)
	{
		missionImportanceData = goal.parent->priority;
	}

	float strengthRatioData = 10.0f; //we are much stronger than enemy
	ui64 danger = fh->evaluateDanger(goal.tile, goal.hero.h);
	if(danger)
		strengthRatioData = static_cast<float>((fl::scalar)goal.hero.h->getTotalStrength() / danger);

	try
	{
		strengthRatio->setValue(strengthRatioData);
		heroStrength->setValue((fl::scalar)goal.hero->getTotalStrength() / ai->primaryHero()->getTotalStrength());
		turnDistance->setValue(turns);
		missionImportance->setValue(missionImportanceData);
	}
	catch(fl::Exception & fe)
	{
		logAi->error("HeroMovementGoalEngineBase::setSharedFuzzyVariables: %s", fe.getWhat());
	}
}

VisitObjEngine::VisitObjEngine()
{
	try
	{
		objectValue = new fl::InputVariable("objectValue"); //value of that object type known by AI

		engine.addInputVariable(objectValue);

		//objectValue ranges are based on checking RMG priorities of some objects and checking LOW/MID/HIGH proportions for various values in QtFuzzyLite
		objectValue->addTerm(new fl::Ramp("LOW", 3500.0, 0.0));
		objectValue->addTerm(new fl::Triangle("MEDIUM", 0.0, 8500.0));
		std::vector<fl::Discrete::Pair> multiRamp = { fl::Discrete::Pair(5000.0, 0.0), fl::Discrete::Pair(10000.0, 0.75), fl::Discrete::Pair(20000.0, 1.0) };
		objectValue->addTerm(new fl::Discrete("HIGH", multiRamp));
		objectValue->setRange(0.0, 20000.0); //relic artifact value is border value by design, even better things are scaled down.

		addRule("if objectValue is HIGH then Value is HIGH");
		addRule("if objectValue is MEDIUM then Value is MEDIUM");
		addRule("if objectValue is LOW then Value is LOW");
	}
	catch(fl::Exception & fe)
	{
		logAi->error("FindWanderTarget: %s", fe.getWhat());
	}
	configure();
}

float VisitObjEngine::evaluate(Goals::VisitObj & goal)
{
	if(!goal.hero)
		return 0;

	auto obj = ai->myCb->getObj(ObjectInstanceID(goal.objid));
	if(!obj)
	{
		logAi->error("Goals::VisitObj objid " + std::to_string(goal.objid) + " no longer visible, probably goal used for something it's not intended");
		return -100; // FIXME: Added check when goal was used for hero instead of VisitHero, but crashes are bad anyway
	}

	std::optional<int> objValueKnownByAI = MapObjectsEvaluator::getInstance().getObjectValue(obj);
	int objValue = 0;

	if(objValueKnownByAI != std::nullopt) //consider adding value manipulation based on object instances on map
	{
		objValue = std::min(std::max(objValueKnownByAI.value(), 0), 20000);
	}
	else
	{
		MapObjectsEvaluator::getInstance().addObjectData(obj->ID, obj->subID, 0);
		logGlobal->error("AI met object type it doesn't know - ID: " + std::to_string(obj->ID) + ", subID: " + std::to_string(obj->subID) + " - adding to database with value " + std::to_string(objValue));
	}

	setSharedFuzzyVariables(goal);

	float output = -1.0f;
	try
	{
		objectValue->setValue(objValue);
		engine.process();
		output = static_cast<float>(value->getValue());
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate getWanderTargetObjectValue: %s", fe.getWhat());
	}
	assert(output >= 0.0f);
	return output;
}

VisitTileEngine::VisitTileEngine() //so far no VisitTile-specific variables that are not shared with HeroMovementGoalEngineBase
{
	configure();
}

float VisitTileEngine::evaluate(Goals::VisitTile & goal)
{
	//we assume that hero is already set and we want to choose most suitable one for the mission
	if(!goal.hero)
		return 0;

	//assert(cb->isInTheMap(g.tile));

	setSharedFuzzyVariables(goal);

	try
	{
		engine.process();

		goal.priority = static_cast<float>(value->getValue());
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate VisitTile: %s", fe.getWhat());
	}
	assert(goal.priority >= 0);
	return goal.priority;
}
