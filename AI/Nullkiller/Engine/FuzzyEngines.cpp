/*
* FuzzyEngines.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "FuzzyEngines.h"
#include "../Goals/Goals.h"

#include "../../../lib/mapObjects/MapObjects.h"
#include "../AIGateway.h"

namespace NKAI
{

#define MIN_AI_STRENGTH (0.5f) //lower when combat AI gets smarter
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 100 times weaker than us

extern boost::thread_specific_ptr<AIGateway> ai;

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
			fl::Rectangle * none = new fl::Rectangle("NONE", CGTownInstance::NONE, CGTownInstance::NONE + (CGTownInstance::FORT - CGTownInstance::NONE) * 0.5f);
			castleWalls->addTerm(none);

			fl::Trapezoid * medium = new fl::Trapezoid("MEDIUM", (CGTownInstance::FORT - CGTownInstance::NONE) * 0.5f, CGTownInstance::FORT,
				CGTownInstance::CITADEL, CGTownInstance::CITADEL + (CGTownInstance::CASTLE - CGTownInstance::CITADEL) * 0.5f);
			castleWalls->addTerm(medium);

			fl::Ramp * high = new fl::Ramp("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE);
			castleWalls->addTerm(high);

			castleWalls->setRange(CGTownInstance::NONE, CGTownInstance::CASTLE);
		}


		bankPresent = new fl::InputVariable("Bank");
		engine.addInputVariable(bankPresent);
		{
			fl::Rectangle * termFalse = new fl::Rectangle("FALSE", 0.0, 0.5f);
			bankPresent->addTerm(termFalse);
			fl::Rectangle * termTrue = new fl::Rectangle("TRUE", 0.5f, 1);
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

}
