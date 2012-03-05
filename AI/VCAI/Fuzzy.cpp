#include "StdInc.h"
#include "Fuzzy.h"
#include "../../lib/CObjectHandler.h"
#include "..\FuzzyLite\AreaCentroidAlgorithm.cpp"
#include "..\FuzzyLite\CompoundTerm.cpp"
#include "..\FuzzyLite\DescriptiveAntecedent.cpp"
#include "..\FuzzyLite\FuzzyEngine.cpp"
#include "..\FuzzyLite\FuzzyAnd.cpp"
#include "..\FuzzyLite\FuzzyOr.cpp"
#include "..\FuzzyLite\InputLVar.cpp"
#include "..\FuzzyLite\OutputLVar.cpp"
#include "..\FuzzyLite\FuzzyAntecedent.cpp"
#include "..\FuzzyLite\FuzzyConsequent.cpp"
#include "..\FuzzyLite\FuzzyDefuzzifier.cpp"
#include "..\FuzzyLite\FuzzyModulation.cpp"
#include "..\FuzzyLite\FuzzyOperator.cpp"
#include "..\FuzzyLite\FuzzyOperation.cpp"
#include "..\FuzzyLite\FuzzyException.cpp"
#include "..\FuzzyLite\FuzzyExceptions.cpp"
#include "..\FuzzyLite\FuzzyRule.cpp"
#include "..\FuzzyLite\HedgeSet.cpp"
#include "..\FuzzyLite\Hedge.cpp"
#include "..\FuzzyLite\SingletonTerm.cpp"
#include "..\FuzzyLite\TrapezoidalTerm.cpp"
#include "..\FuzzyLite\TriangularTerm.cpp"
#include "..\FuzzyLite\LinguisticTerm.cpp"
#include "..\FuzzyLite\LinguisticVariable.cpp"
#include "..\FuzzyLite\RuleBlock.cpp"
#include "..\FuzzyLite\ShoulderTerm.cpp"
#include "..\FuzzyLite\StrOp.cpp"
#include "..\FuzzyLite\MamdaniRule.cpp"
#include "..\FuzzyLite\MamdaniConsequent.cpp"

/*
 * Fuzzy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/

#define MIN_AI_STRENGHT (0.5f) //lower when combat AI gets smarter

class BankConfig;
class FuzzyEngine;
class InputLVar;
class CGTownInstance;

using namespace boost::assign;
using namespace vstd;

FuzzyHelper fh;

struct armyStructure
{
	float walkers, shooters, flyers;
	ui32 maxSpeed;
};

ui64 evaluateBankConfig (BankConfig * bc)
{
	ui64 danger = 0;
	BOOST_FOREACH (auto opt, bc->guards)
	{
		danger += VLC->creh->creatures[opt.first]->fightValue * opt.second;
	}
	return danger;
}

armyStructure evaluateArmyStructure (const CArmedInstance * army)
{
	ui64 totalStrenght = army->getArmyStrength();
	double walkersStrenght = 0;
	double flyersStrenght = 0;
	double shootersStrenght = 0;
	ui32 maxSpeed = 0;

	BOOST_FOREACH(auto s, army->Slots())
	{
		bool walker = true;
		if (s.second->type->hasBonusOfType(Bonus::SHOOTER))
		{
			shootersStrenght += s.second->getPower();
			walker = false;
		}
		if (s.second->type->hasBonusOfType(Bonus::FLYING))
		{
			flyersStrenght += s.second->getPower();
			walker = false;
		}
		if (walker)
			walkersStrenght += s.second->getPower();

		amax (maxSpeed, s.second->type->speed);
	}
	armyStructure as;
	as.walkers = walkersStrenght / totalStrenght;
	as.shooters = shootersStrenght / totalStrenght;
	as.flyers = flyersStrenght / totalStrenght;
	as.maxSpeed = maxSpeed;
	return as;
}

FuzzyHelper::FuzzyHelper()
{
	initBank();
	initTacticalAdvantage();
}

void FuzzyHelper::initBank()
{
	try
	{
		//Trivial bank estimation
		bankInput = new fl::InputLVar("BankInput");
		bankDanger = new fl::OutputLVar("BankDanger");
		bankInput->addTerm(new fl::SingletonTerm ("SET"));

		engine.addRuleBlock (&bankBlock); //have to be added before the rules are parsed
		engine.addInputLVar (bankInput);
		engine.addOutputLVar (bankDanger);
		for (int i = 0; i < 4; ++i)
		{
			bankDanger->addTerm(new fl::TriangularTerm ("Bank" + boost::lexical_cast<std::string>(i), 0, 1));
			bankBlock.addRule(new fl::MamdaniRule("if BankInput is SET then BankDanger is Bank" + boost::lexical_cast<std::string>(i), engine));
		}
	}
	catch (fl::FuzzyException fe)
	{
		tlog1 << "initBank " << fe.name() << ": " << fe.message() << '\n';
	}
}

void FuzzyHelper::initTacticalAdvantage()
{
	try
	{
		//Tactical advantage calculation
		std::vector<fl::InputLVar*> helper;
		ourShooters = new fl::InputLVar("OurShooters");
		ourWalkers = new fl::InputLVar("OurWalkers");
		ourFlyers = new fl::InputLVar("OurFlyers");
		enemyShooters = new fl::InputLVar("EnemyShooters");
		enemyWalkers = new fl::InputLVar("EnemyWalkers");
		enemyFlyers = new fl::InputLVar("EnemyFlyers");

		helper += ourShooters, ourWalkers, ourFlyers, enemyShooters, enemyWalkers, enemyFlyers;

		BOOST_FOREACH (auto val, helper)
		{
			val->addTerm (new fl::ShoulderTerm("FEW", 0, 0.75, true));
			val->addTerm (new fl::ShoulderTerm("MANY", 0.25, 1, false));
			engine.addInputLVar(val);
		}
		helper.clear();

		ourSpeed =  new fl::InputLVar("OurSpeed");
		enemySpeed = new fl::InputLVar("EnemySpeed");

		helper += ourSpeed, enemySpeed;

		BOOST_FOREACH (auto val, helper)
		{
			val->addTerm (new fl::ShoulderTerm("LOW", 3, 8.1, true));
			val->addTerm (new fl::TriangularTerm("MEDIUM", 6.9, 13.1));
			val->addTerm (new fl::ShoulderTerm("HIGH", 10.5, 16, false));
			engine.addInputLVar(val);
		}
		castleWalls = new fl::InputLVar("CastleWalls");
		castleWalls->addTerm(new fl::SingletonTerm("NONE", CGTownInstance::NONE));
		castleWalls->addTerm(new fl::TrapezoidalTerm("MEDIUM", CGTownInstance::FORT, 2.5));
		castleWalls->addTerm(new fl::ShoulderTerm("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE));
		engine.addInputLVar(castleWalls);

		bankPresent = new fl::InputLVar("Bank");
		bankPresent->addTerm(new fl::SingletonTerm("FALSE", 0));
		bankPresent->addTerm(new fl::SingletonTerm("TRUE", 1));
		engine.addInputLVar(bankPresent);

		threat = new fl::OutputLVar("Threat");
		threat->addTerm (new fl::TriangularTerm("LOW", MIN_AI_STRENGHT, 1));
		threat->addTerm (new fl::TriangularTerm("MEDIUM", 0.8, 1.2));
		threat->addTerm (new fl::ShoulderTerm("HIGH", 1, 1.5, false));
		engine.addOutputLVar(threat);

		engine.hedgeSet().add(new fl::HedgeSomewhat());
		engine.hedgeSet().add(new fl::HedgeVery());

		tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is LOW then Threat is very LOW", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if OurSpeed is LOW and OurShooters is FEW and EnemyShooters is MANY then Threat is very HIGH", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if (OurShooters is MANY or OurFlyers is MANY) and EnemyShooters is MANY then Threat is LOW", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is HIGH then Threat is somewhat HIGH", engine));
		//tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemyShooters is MANY then Threat is MEDIUM", engine));

		tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and OurShooters is MANY then Threat is somewhat HIGH", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and EnemyShooters is MANY then Threat is LOW", engine));

		tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurWalkers is MANY then Threat is very HIGH", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurFlyers is MANY and OurShooters is MANY then Threat is MEDIUM", engine));
		tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is MEDIUM and OurShooters is MANY and EnemyWalkers is MANY then Threat is LOW", engine));

		engine.addRuleBlock (&tacticalAdvantage);
	}
	catch(fl::ParsingException pe)
	{
		tlog1 << "initTacticalAdvantage " << pe.name() << ": " << pe.message() << '\n';
	}
	catch (fl::FuzzyException fe)
	{
		tlog1 << "initTacticalAdvantage " << fe.name() << ": " << fe.message() << '\n';
	}
}

ui64 FuzzyHelper::estimateBankDanger (int ID)
{
	std::vector <ConstTransitivePtr<BankConfig>> & configs = VLC->objh->banksInfo[ID];
	ui64 val = INFINITY;
	try
	{
		switch (configs.size())
		{
			case 4:
				try
				{
					int bankVal;
					for (int i = 0; i < 4; ++i)
					{
						bankVal = evaluateBankConfig (VLC->objh->banksInfo[ID][i]);
						bankDanger->term("Bank" + boost::lexical_cast<std::string>(i))->setMinimum(bankVal * 0.5f);
						bankDanger->term("Bank" + boost::lexical_cast<std::string>(i))->setMaximum(bankVal * 1.5f);
					}
					//comparison purposes
					//int averageValue = (evaluateBankConfig (VLC->objh->banksInfo[ID][0]) + evaluateBankConfig (VLC->objh->banksInfo[ID][3])) * 0.5;
					dynamic_cast<fl::SingletonTerm*>(bankInput->term("SET"))->setValue(0.5);
					bankInput->setInput (0.5);
					engine.process (BANK_DANGER);
					val = bankDanger->output().defuzzify(); //some expected value of this bank
				}
				catch (fl::FuzzyException fe)
				{
					tlog1 << fe.name() << ": " << fe.message() << '\n';
				}
				break;
			case 1: //rare case - Pyramid
				val = evaluateBankConfig (VLC->objh->banksInfo[ID][0]);
				break;
			default:
				tlog3 << ("Uhnandled bank config!\n");
		}
	}
	catch (fl::FuzzyException fe)
	{
		tlog1 << "estimateBankDanger " << fe.name() << ": " << fe.message() << '\n';
	}
	return val;

}

float FuzzyHelper::getTacticalAdvantage (const CArmedInstance *we, const CArmedInstance *enemy)
{
	float output = 1;
	try
	{
		armyStructure ourStructure = evaluateArmyStructure(we);
		armyStructure enemyStructure = evaluateArmyStructure(enemy);

		ourWalkers->setInput(ourStructure.walkers);
		ourShooters->setInput(ourStructure.shooters);
		ourFlyers->setInput(ourStructure.flyers);
		ourSpeed->setInput(ourStructure.maxSpeed);

		enemyWalkers->setInput(enemyStructure.walkers);
		enemyShooters->setInput(enemyStructure.shooters);
		enemyFlyers->setInput(enemyStructure.flyers);
		enemySpeed->setInput(enemyStructure.maxSpeed);

		bool bank = dynamic_cast<const CBank*>(enemy);
		if (bank)
			bankPresent->setInput(1);
		else
			bankPresent->setInput(0);

		const CGTownInstance * fort = dynamic_cast<const CGTownInstance*>(enemy);
		if (fort)
		{
			castleWalls->setInput (fort->fortLevel());
		}
		else
			castleWalls->setInput(0);

		engine.process (TACTICAL_ADVANTAGE);
		output = threat->output().defuzzify();
	}
	catch (fl::FuzzyException fe)
	{
		tlog1 << "getTacticalAdvantage " << fe.name() << ": " << fe.message() << '\n';
	}
	return output;
}