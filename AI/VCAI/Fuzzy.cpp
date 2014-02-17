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
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 10 times weaker than us

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
	engine.fuzzyOperator().setAggregation(new fl::FuzzyOrSum()); //to consider all cases simultaneously

	initBank();
	initTacticalAdvantage();
	initVisitTile();
	
	logAi->infoStream() << engine.toString();
}

void FuzzyHelper::initBank()
{
	try
	{
		//Trivial bank estimation
		bankInput = new fl::InputLVar("BankInput");
		bankDanger = new fl::OutputLVar("BankDanger");
		bankInput->addTerm(new fl::SingletonTerm ("SET", 0.5));

		engine.addInputLVar (bankInput);
		engine.addOutputLVar (bankDanger);
		engine.addRuleBlock (&bankBlock);
		
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

		ta.ourShooters = new fl::InputLVar("OurShooters");
		ta.ourWalkers = new fl::InputLVar("OurWalkers");
		ta.ourFlyers = new fl::InputLVar("OurFlyers");
		ta.enemyShooters = new fl::InputLVar("EnemyShooters");
		ta.enemyWalkers = new fl::InputLVar("EnemyWalkers");
		ta.enemyFlyers = new fl::InputLVar("EnemyFlyers");

		helper += ta.ourShooters, ta.ourWalkers, ta.ourFlyers, ta.enemyShooters, ta.enemyWalkers, ta.enemyFlyers;

		for (auto val : helper)
		{
			engine.addInputLVar(val);
			val->addTerm (new fl::ShoulderTerm("FEW", 0, 0.6, true));
			val->addTerm (new fl::ShoulderTerm("MANY", 0.4, 1, false));
		}
		helper.clear();

		ta.ourSpeed =  new fl::InputLVar("OurSpeed");
		ta.enemySpeed = new fl::InputLVar("EnemySpeed");

		helper += ta.ourSpeed, ta.enemySpeed;

		for (auto val : helper)
		{
			engine.addInputLVar(val);
			val->addTerm (new fl::ShoulderTerm("LOW", 3, 6.5, true));
			val->addTerm (new fl::TriangularTerm("MEDIUM", 5.5, 10.5));
			val->addTerm (new fl::ShoulderTerm("HIGH", 8.5, 16, false));
		}
		
		ta.castleWalls = new fl::InputLVar("CastleWalls");
		engine.addInputLVar(ta.castleWalls);
		ta.castleWalls->addTerm(new fl::SingletonTerm("NONE", CGTownInstance::NONE));
		ta.castleWalls->addTerm(new fl::TrapezoidalTerm("MEDIUM", CGTownInstance::FORT, 2.5));
		ta.castleWalls->addTerm(new fl::ShoulderTerm("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE, false));

		ta.bankPresent = new fl::InputLVar("Bank");
		engine.addInputLVar(ta.bankPresent);
		ta.bankPresent->addTerm(new fl::SingletonTerm("FALSE", 0));
		ta.bankPresent->addTerm(new fl::SingletonTerm("TRUE", 1));

		ta.threat = new fl::OutputLVar("Threat");
		engine.addOutputLVar(ta.threat);
		ta.threat->addTerm (new fl::ShoulderTerm("LOW", MIN_AI_STRENGHT, 1, true));
		ta.threat->addTerm (new fl::TriangularTerm("MEDIUM", 0.8, 1.2));
		ta.threat->addTerm (new fl::ShoulderTerm("HIGH", 1, 1.5, false));

		engine.addRuleBlock (&ta.tacticalAdvantage);

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is LOW then Threat is LOW", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemyShooters is FEW then Threat is LOW", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurSpeed is LOW and EnemyShooters is MANY then Threat is HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurSpeed is HIGH and EnemyShooters is MANY then Threat is LOW", engine));

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurWalkers is FEW and EnemyShooters is MANY then Threat is somewhat LOW", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if OurShooters is MANY and EnemySpeed is HIGH then Threat is somewhat HIGH", engine));
		//just to cover all cases
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if EnemySpeed is MEDIUM then Threat is MEDIUM", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if EnemySpeed is LOW and OurShooters is FEW then Threat is MEDIUM", engine));

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and OurShooters is MANY then Threat is somewhat HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if Bank is TRUE and EnemyShooters is MANY then Threat is LOW", engine));

		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurWalkers is MANY then Threat is very HIGH", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is HIGH and OurFlyers is MANY and OurShooters is MANY then Threat is MEDIUM", engine));
		ta.tacticalAdvantage.addRule(new fl::MamdaniRule("if CastleWalls is MEDIUM and OurShooters is MANY and EnemyWalkers is MANY then Threat is LOW", engine));

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
					//dynamic_cast<fl::SingletonTerm*>(bankInput->term("SET"))->setValue(0.5);
					bankInput->setInput (0.5);
					engine.process (BANK_DANGER);
					//engine.process();
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
		//engine.process();
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

	for (auto g : vec)
	{
		setPriority(g);
	}

	auto compareGoals = [](const Goals::TSubgoal & lhs, const Goals::TSubgoal & rhs) -> bool
	{
		return lhs->priority < rhs->priority;
	};

	boost::sort (vec, compareGoals);
	return vec.back();
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
	delete turnDistance;
	delete missionImportance;
}

void FuzzyHelper::initVisitTile()
{
	try
	{
		std::vector<fl::InputLVar*> helper;

		vt.strengthRatio = new fl::InputLVar("strengthRatio"); //hero must be strong enough to defeat guards
		vt.heroStrength = new fl::InputLVar("heroStrength"); //we want to use weakest possible hero
		vt.turnDistance = new fl::InputLVar("turnDistance"); //we want to use hero who is near
		vt.missionImportance = new fl::InputLVar("lockedMissionImportance"); //we may want to preempt hero with low-priority mission
		vt.value = new fl::OutputLVar("Value");

		helper += vt.strengthRatio, vt.heroStrength, vt.turnDistance, vt.missionImportance;
		for (auto val : helper)
		{
			engine.addInputLVar(val);
		}
		engine.addOutputLVar (vt.value);

		vt.strengthRatio->addTerm (new fl::ShoulderTerm("LOW", 0, SAFE_ATTACK_CONSTANT, true));
		vt.strengthRatio->addTerm (new fl::ShoulderTerm("HIGH", SAFE_ATTACK_CONSTANT, SAFE_ATTACK_CONSTANT * 3, false));

		//strength compared to our main hero
		vt.heroStrength->addTerm (new fl::ShoulderTerm("LOW", 0, 0.2, true));
		vt.heroStrength->addTerm (new fl::TriangularTerm("MEDIUM", 0.2, 0.8));
		vt.heroStrength->addTerm (new fl::ShoulderTerm("HIGH", 0.5, 1, false));

		vt.turnDistance->addTerm (new fl::ShoulderTerm("SMALL", 0, 0.5, true));
		vt.turnDistance->addTerm (new fl::TriangularTerm("MEDIUM", 0.1, 0.8));
		vt.turnDistance->addTerm (new fl::ShoulderTerm("LONG", 0.5, 3, false));

		vt.missionImportance->addTerm (new fl::ShoulderTerm("LOW", 0, 2.5, true));
		vt.missionImportance->addTerm (new fl::TriangularTerm("MEDIUM", 2, 3));
		vt.missionImportance->addTerm (new fl::ShoulderTerm("HIGH", 2.5, 5, false));

		//an issue: in 99% cases this outputs center of mass (2.5) regardless of actual input :/
		 //should be same as "mission Importance" to keep consistency
		vt.value->addTerm (new fl::ShoulderTerm("LOW", 0, 2.5, true));
		vt.value->addTerm (new fl::TriangularTerm("MEDIUM", 2, 3)); //can't be center of mass :/
		vt.value->addTerm (new fl::ShoulderTerm("HIGH", 2.5, 5, false));

		engine.addRuleBlock (&vt.rules);

		//use unarmed scouts if possible
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is HIGH and heroStrength is LOW then Value is very HIGH", engine));
		//we may want to use secondary hero(es) rather than main hero
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is HIGH and heroStrength is MEDIUM then Value is somewhat HIGH", engine));
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is HIGH and heroStrength is HIGH then Value is somewhat LOW", engine));
		//don't assign targets to heroes who are too weak, but prefer targets of our main hero (in case we need to gather army)
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is LOW and heroStrength is LOW then Value is very LOW", engine));
		//attempt to arm secondary heroes is not stupid
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is LOW and heroStrength is MEDIUM then Value is somewhat HIGH", engine));
		vt.rules.addRule (new fl::MamdaniRule("if strengthRatio is LOW and heroStrength is HIGH then Value is LOW", engine));
		
		//do not cancel important goals
		vt.rules.addRule (new fl::MamdaniRule("if lockedMissionImportance is HIGH then Value is very LOW", engine));
		vt.rules.addRule (new fl::MamdaniRule("if lockedMissionImportance is MEDIUM then Value is somewhat LOW", engine));
		vt.rules.addRule (new fl::MamdaniRule("if lockedMissionImportance is LOW then Value is HIGH", engine));
		//pick nearby objects if it's easy, avoid long walks
		vt.rules.addRule (new fl::MamdaniRule("if turnDistance is SMALL then Value is HIGH", engine));
		vt.rules.addRule (new fl::MamdaniRule("if turnDistance is MEDIUM then Value is MEDIUM", engine));
		vt.rules.addRule (new fl::MamdaniRule("if turnDistance is LONG then Value is LOW", engine));
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "visitTile " << fe.name() << ": " << fe.message();
	}
}
float FuzzyHelper::evaluate (Goals::VisitTile & g)
{
	//we assume that hero is already set and we want to choose most suitable one for the mission
	if (!g.hero)
		return 0;

	//assert(cb->isInTheMap(g.tile));
	cb->setSelection (g.hero.h);
	float turns = 0;
	float distance = cb->getMovementCost(g.hero.h, g.tile);
	if (!distance) //we stand on that tile
		turns = 0;
	else
	{
		if (distance < g.hero->movement) //we can move there within one turn
			turns = (fl::flScalar)distance /g.hero->movement;
		else
			turns = 1 + (fl::flScalar)(distance - g.hero->movement)/g.hero->maxMovePoints(true); //bool on land?
	}
	
	float missionImportance = 0;
	if (vstd::contains(ai->lockedHeroes, g.hero))
		missionImportance = ai->lockedHeroes[g.hero]->priority;

	float strengthRatio = 10.0f; //we are much stronger than enemy
	ui64 danger = evaluateDanger (g.tile, g.hero.h);
	if (danger)
		strengthRatio = (fl::flScalar)g.hero.h->getTotalStrength() / danger;

	try
	{
		vt.strengthRatio->setInput (strengthRatio);
		vt.heroStrength->setInput ((fl::flScalar)g.hero->getTotalStrength()/ai->primaryHero()->getTotalStrength());
		vt.turnDistance->setInput (turns);
		vt.missionImportance->setInput (missionImportance);

		//engine.process();
		engine.process (VISIT_TILE);
		g.priority = vt.value->output().defuzzify();
	}
	catch (fl::FuzzyException & fe)
	{
        logAi->errorStream() << "evaluate VisitTile " << fe.name() << ": " << fe.message();
	}
	return g.priority;

}
float FuzzyHelper::evaluate (Goals::VisitHero & g)
{
	auto obj = cb->getObj(ObjectInstanceID(g.objid)); //we assume for now that these goals are similiar
	if (!obj)
		return -100; //hero died in the meantime
	//TODO: consider direct copy (constructor?)
	g.setpriority(Goals::VisitTile(obj->visitablePos()).sethero(g.hero).setisAbstract(g.isAbstract).accept(this));
	return g.priority;	
}
float FuzzyHelper::evaluate (Goals::GatherArmy & g)
{
	//the more army we need, the more important goal
	//the more army we lack, the less important goal
	float army = g.hero->getArmyStrength();
	return g.value / std::max(g.value - army, 1000.0f);
}
float FuzzyHelper::evaluate (Goals::BuildThis & g)
{
	return 1;
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
	logAi->warnStream() << boost::format("Cannot evaluate goal %s") % g.name();
	return g.priority;
}
void FuzzyHelper::setPriority (Goals::TSubgoal & g)
{
	g->setpriority(g->accept(this)); //this enforces returned value is set
}
