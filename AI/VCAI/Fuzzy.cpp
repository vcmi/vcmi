#include "StdInc.h"
#include "Fuzzy.h"
#include <limits>

#include "../../lib/CObjectHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/VCMI_Lib.h"
#include "../../CCallback.h"
//#include "Goals.cpp"
#include "VCAI.h"

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

struct BankConfig;
class FuzzyEngine;
class InputLVar;
class CGTownInstance;

using namespace boost::assign;
using namespace vstd;
//using namespace Goals;

FuzzyHelper *fh;

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

struct armyStructure
{
	float walkers, shooters, flyers;
	ui32 maxSpeed;
};

ui64 evaluateBankConfig (BankConfig * bc)
{
	ui64 danger = 0;
	for (auto opt : bc->guards)
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

	for(auto s : army->Slots())
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

		amax (maxSpeed, s.second->type->valOfBonuses(Bonus::STACKS_SPEED));
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
	engine.hedgeSet().add(new fl::HedgeSomewhat());
	engine.hedgeSet().add(new fl::HedgeVery());

	initBank();
	initTacticalAdvantage();
	initVisitTile();
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
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "initBank " << fe.name() << ": " << fe.message();
	}
}

void FuzzyHelper::initTacticalAdvantage()
{
	try
	{
		//Tactical advantage calculation
		std::vector<fl::InputLVar*> helper;

		//TODO: delete all that stuff upon destruction
		ta.ourShooters = new fl::InputLVar("OurShooters");
		ta.ourWalkers = new fl::InputLVar("OurWalkers");
		ta.ourFlyers = new fl::InputLVar("OurFlyers");
		ta.enemyShooters = new fl::InputLVar("EnemyShooters");
		ta.enemyWalkers = new fl::InputLVar("EnemyWalkers");
		ta.enemyFlyers = new fl::InputLVar("EnemyFlyers");

		helper += ta.ourShooters, ta.ourWalkers, ta.ourFlyers, ta.enemyShooters, ta.enemyWalkers, ta.enemyFlyers;

		for (auto val : helper)
		{
			val->addTerm (new fl::ShoulderTerm("FEW", 0, 0.75, true));
			val->addTerm (new fl::ShoulderTerm("MANY", 0.25, 1, false));
			engine.addInputLVar(val);
		}
		helper.clear();

		ta.ourSpeed =  new fl::InputLVar("OurSpeed");
		ta.enemySpeed = new fl::InputLVar("EnemySpeed");

		helper += ta.ourSpeed, ta.enemySpeed;

		for (auto val : helper)
		{
			val->addTerm (new fl::ShoulderTerm("LOW", 3, 8.1, true));
			val->addTerm (new fl::TriangularTerm("MEDIUM", 6.9, 13.1));
			val->addTerm (new fl::ShoulderTerm("HIGH", 10.5, 16, false));
			engine.addInputLVar(val);
		}
		ta.castleWalls = new fl::InputLVar("CastleWalls");
		ta.castleWalls->addTerm(new fl::SingletonTerm("NONE", CGTownInstance::NONE));
		ta.castleWalls->addTerm(new fl::TrapezoidalTerm("MEDIUM", CGTownInstance::FORT, 2.5));
		ta.castleWalls->addTerm(new fl::ShoulderTerm("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE));
		engine.addInputLVar(ta.castleWalls);

		ta.bankPresent = new fl::InputLVar("Bank");
		ta.bankPresent->addTerm(new fl::SingletonTerm("FALSE", 0));
		ta.bankPresent->addTerm(new fl::SingletonTerm("TRUE", 1));
		engine.addInputLVar(ta.bankPresent);

		ta.threat = new fl::OutputLVar("Threat");
		ta.threat->addTerm (new fl::TriangularTerm("LOW", MIN_AI_STRENGHT, 1));
		ta.threat->addTerm (new fl::TriangularTerm("MEDIUM", 0.8, 1.2));
		ta.threat->addTerm (new fl::ShoulderTerm("HIGH", 1, 1.5, false));
		engine.addOutputLVar(ta.threat);

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is LOW then Threat is very LOW", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurSpeed is LOW and OurShooters is FEW and EnemyShooters is MANY then Threat is very HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if (OurShooters is MANY and OurFlyers is MANY) and EnemyShooters is MANY then Threat is LOW", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is HIGH then Threat is somewhat HIGH", engine));
		//tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemyShooters is MANY then Threat is MEDIUM", engine));

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and OurShooters is MANY then Threat is somewhat HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and EnemyShooters is MANY then Threat is LOW", engine));

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurWalkers is MANY then Threat is very HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurFlyers is MANY and OurShooters is MANY then Threat is MEDIUM", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is MEDIUM and OurShooters is MANY and EnemyWalkers is MANY then Threat is LOW", engine));

		engine.addRuleBlock (&ta.tacticalAdvantage);
	}
	catch(fl::ParsingException & pe)
	{
        logAi->errorStream() << "initTacticalAdvantage " << pe.name() << ": " << pe.message();
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "initTacticalAdvantage " << fe.name() << ": " << fe.message();
	}
}

ui64 FuzzyHelper::estimateBankDanger (int ID)
{
	std::vector <ConstTransitivePtr<BankConfig>> & configs = VLC->objh->banksInfo[ID];
	ui64 val = std::numeric_limits<ui64>::max();
	try
	{
		switch (configs.size())
		{
			case 4:
				try
				{
					for (int i = 0; i < 4; ++i)
					{
						int bankVal = evaluateBankConfig (VLC->objh->banksInfo[ID][i]);
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
				catch (fl::FuzzyException & fe)
				{
                    logAi->errorStream() << fe.name() << ": " << fe.message();
				}
				break;
			case 1: //rare case - Pyramid
				val = evaluateBankConfig (VLC->objh->banksInfo[ID][0]);
				break;
			default:
                logAi->warnStream() << ("Uhnandled bank config!");
		}
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "estimateBankDanger " << fe.name() << ": " << fe.message();
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

		ta.ourWalkers->setInput(ourStructure.walkers);
		ta.ourShooters->setInput(ourStructure.shooters);
		ta.ourFlyers->setInput(ourStructure.flyers);
		ta.ourSpeed->setInput(ourStructure.maxSpeed);

		ta.enemyWalkers->setInput(enemyStructure.walkers);
		ta.enemyShooters->setInput(enemyStructure.shooters);
		ta.enemyFlyers->setInput(enemyStructure.flyers);
		ta.enemySpeed->setInput(enemyStructure.maxSpeed);

		bool bank = dynamic_cast<const CBank*>(enemy);
		if (bank)
			ta.bankPresent->setInput(1);
		else
			ta.bankPresent->setInput(0);

		const CGTownInstance * fort = dynamic_cast<const CGTownInstance*>(enemy);
		if (fort)
		{
			ta.castleWalls->setInput (fort->fortLevel());
		}
		else
			ta.castleWalls->setInput(0);

		engine.process (TACTICAL_ADVANTAGE);
		output = ta.threat->output().defuzzify();
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "getTacticalAdvantage " << fe.name() << ": " << fe.message();
	}
	return output;
}

FuzzyHelper::TacticalAdvantage::~TacticalAdvantage()
{
	//TODO: smart pointers?
	delete ourWalkers;
	delete ourShooters;
	delete ourFlyers;
	delete enemyWalkers;
	delete enemyShooters;
	delete enemyFlyers;
	delete ourSpeed;
	delete enemySpeed;
	delete bankPresent;
	delete castleWalls;
	delete threat;
}

//shared_ptr<AbstractGoal> chooseSolution (std::vector<shared_ptr<AbstractGoal>> & vec)
Goals::TSubgoal FuzzyHelper::chooseSolution (Goals::TGoalVec vec)
{
	if (vec.empty()) //no possibilities found
		return sptr(Goals::Invalid());

	typedef std::pair<Goals::TSubgoal, float> goalValue;
	std::vector <goalValue> values;

	for (auto g : vec)
	{
		values.push_back (std::make_pair(g, g->accept(this)));
	}

	auto compareGoals = [&](const goalValue & lhs, const goalValue & rhs) -> bool
	{
		return lhs.second < rhs.second;
	};

	boost::sort (values, compareGoals);
	return values.back().first;
}

float FuzzyHelper::evaluate (Goals::Explore & g)
{
	return 1;
}
float FuzzyHelper::evaluate (Goals::RecruitHero & g)
{
	return 1; //just try to recruit hero as one of options
}
FuzzyHelper::EvalVisitTile::~EvalVisitTile()
{
	delete strengthRatio;
	delete heroStrength;
	delete tileDistance;
	delete missionImportance;
}

void FuzzyHelper::initVisitTile()
{
	std::vector<fl::InputLVar*> helper;

	vt.strengthRatio = new fl::InputLVar("strengthRatio"); //hero must be strong enough to defeat guards
	vt.heroStrength = new fl::InputLVar("heroStrength"); //we want to use weakest possible hero
	vt.tileDistance = new fl::InputLVar("tileDistance"); //we want to use hero who is near
	vt.missionImportance = new fl::InputLVar("lockedMissionImportance"); //we may want to preempt hero with low-priority mission
	vt.value = new fl::OutputLVar("Value");

	helper += vt.strengthRatio, vt.heroStrength, vt.tileDistance, vt.missionImportance;

	vt.strengthRatio->addTerm (new fl::ShoulderTerm("LOW", 0.3, SAFE_ATTACK_CONSTANT, true));
	vt.strengthRatio->addTerm (new fl::ShoulderTerm("HIGH", SAFE_ATTACK_CONSTANT, SAFE_ATTACK_CONSTANT * 3, false));

	vt.heroStrength->addTerm (new fl::ShoulderTerm("LOW", 1, 2500, true)); //assumed strength of new hero from tavern
	vt.heroStrength->addTerm (new fl::TriangularTerm("MEDIUM", 2500, 40000)); //assumed strength of hero able to defeat bank
	vt.heroStrength->addTerm (new fl::ShoulderTerm("HIGH", 40000, 1e5, false)); //assumed strength of hero able to clear utopia

	vt.tileDistance->addTerm (new fl::ShoulderTerm("SMALL", 0, 3.5, true));
	vt.tileDistance->addTerm (new fl::TriangularTerm("MEDIUM", 3, 10.5));
	vt.tileDistance->addTerm (new fl::ShoulderTerm("LONG", 10, 50, false));

	vt.missionImportance->addTerm (new fl::ShoulderTerm("LOW", 0, 3.1, true));
	vt.missionImportance->addTerm (new fl::TriangularTerm("MEDIUM", 2, 9.5));
	vt.missionImportance->addTerm (new fl::ShoulderTerm("HIGH", 4.5, 10, false));

	vt.value->addTerm (new fl::ShoulderTerm("LOW", 0, 1.1, true));
	vt.value->addTerm (new fl::ShoulderTerm("HIGH", 1, 5, false));
	
	for (auto val : helper)
	{
		engine.addInputLVar(val);
	}
	engine.addOutputLVar (vt.value);

	//vt.rules.addRule (new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is LOW then Threat is very LOW", engine));
	//use unarmed scouts if possible
	vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is HIGH and heroStrength is LOW then Value is very HIGH", engine));
	//if medium heroes can't scratch enemy, don't try to arm them
	vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is LOW and heroStrength is MEDIUM then Value is LOW", engine));
	//do not cancel important goals
	vt.rules.addRule (new fl::MamdaniRule("if lockedMissionImportance is HIGH then Value is very LOW", engine));
	//pick nearby objects if it's easy, avoid long walks
	vt.rules.addRule (new fl::MamdaniRule("if tileDistance is SMALL then Value is HIGH", engine));
	vt.rules.addRule (new fl::MamdaniRule("if tileDistance is LONG then Value is LOW", engine));

	engine.addRuleBlock (&vt.rules);
}
float FuzzyHelper::evaluate (Goals::VisitTile & g)
{
	//we assume that hero is already set and we want to choose most suitable one for the mission
	if (!g.hero)
		return 0;

	float output = 0;

	cb->setSelection (g.hero.h);
	int distance = cb->getDistance(g.tile); //at this point we already assume tile is reachable
	
	float missionImportance = 0;
	if (vstd::contains(ai->lockedHeroes, g.hero))
		missionImportance = ai->lockedHeroes[g.hero]->importanceWhenLocked();

	float strengthRatio = 100; //we are much stronger than enemy
	ui64 danger = evaluateDanger (g.tile, g.hero.h);
	if (danger)
		strengthRatio = g.hero.h->getTotalStrength() / danger;

	try
	{
		vt.strengthRatio->setInput (strengthRatio);
		vt.heroStrength->setInput (g.hero->getTotalStrength());
		vt.tileDistance->setInput (distance);
		vt.missionImportance->setInput (missionImportance);

		engine.process (VISIT_TILE);
		output = vt.value->output().defuzzify();
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "evaluate VisitTile " << fe.name() << ": " << fe.message();
	}
	return output;

}
float FuzzyHelper::evaluate (Goals::VisitHero & g)
{
	auto obj = cb->getObj(ObjectInstanceID(g.objid)); //we assume for now that these goals are similiar
	return Goals::VisitTile(obj->pos).sethero(g.hero).setisAbstract(g.isAbstract).accept(this);
	//TODO: consider direct copy (constructor?)
}
float FuzzyHelper::evaluate (Goals::BuildThis & g)
{
	return 0;
}
float FuzzyHelper::evaluate (Goals::DigAtTile & g)
{
	return 0;
}
float FuzzyHelper::evaluate (Goals::CollectRes & g)
{
	return 0;
}
float FuzzyHelper::evaluate (Goals::Build & g)
{
	return 0;
}
float FuzzyHelper::evaluate (Goals::Invalid & g)
{
	return -1e10;
}
float FuzzyHelper::evaluate (Goals::AbstractGoal & g)
{
	logAi->debugStream() << boost::format("Cannot evaluate goal %s") % g.name();
	return -1e10;
}
