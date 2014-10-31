#include "StdInc.h"
#include "Fuzzy.h"
#include <limits>

#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/CommonConstructors.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/VCMI_Lib.h"
#include "../../CCallback.h"
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
class Engine;
class InputVariable;
class CGTownInstance;

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
	initBank();
	initTacticalAdvantage();
	initVisitTile();
	
	engine.configure("AlgebraicProduct", "AlgebraicSum",
			"AlgebraicProduct", "AlgebraicSum", "Centroid");

	logAi->infoStream() << engine.toString();
}

void FuzzyHelper::initBank()
{
	try
	{
		//Trivial bank estimation
		bankInput = new fl::InputVariable("BankInput");
		bankDanger = new fl::OutputVariable("BankDanger");
		bankInput->addTerm(new fl::Rectangle("SET", 0.5 - 5 * fl::fuzzylite::macheps(),
				0.5 + 5 * fl::fuzzylite::macheps()));

		engine.addInputVariable(bankInput);
		engine.addOutputVariable(bankDanger);
		engine.addRuleBlock(&bankBlock);
		for (int i = 0; i < 4; ++i)
		{
			bankDanger->addTerm(new fl::Triangle("Bank" + boost::lexical_cast<std::string>(i), 0, 1));
			bankBlock.addRule(fl::Rule::parse("if BankInput is SET then BankDanger is Bank" + boost::lexical_cast<std::string>(i), &engine));
		}
	}
	catch (fl::Exception & fe)
	{
		logAi->errorStream() << "initBank : " << fe.getWhat();
	}
}

void FuzzyHelper::initTacticalAdvantage()
{
	try
	{

		ta.ourShooters = new fl::InputVariable("OurShooters");
		ta.ourWalkers = new fl::InputVariable("OurWalkers");
		ta.ourFlyers = new fl::InputVariable("OurFlyers");
		ta.enemyShooters = new fl::InputVariable("EnemyShooters");
		ta.enemyWalkers = new fl::InputVariable("EnemyWalkers");
		ta.enemyFlyers = new fl::InputVariable("EnemyFlyers");

		//Tactical advantage calculation
		std::vector<fl::InputVariable*> helper =
		{
			ta.ourShooters, ta.ourWalkers, ta.ourFlyers, ta.enemyShooters, ta.enemyWalkers, ta.enemyFlyers
		};

		for (auto val : helper)
		{
			engine.addInputVariable(val);
			val->addTerm(new fl::Ramp("FEW", 0.6, 0.0));
			val->addTerm(new fl::Ramp("MANY", 0.4, 1));
		}

		ta.ourSpeed = new fl::InputVariable("OurSpeed");
		ta.enemySpeed = new fl::InputVariable("EnemySpeed");

		helper = {ta.ourSpeed, ta.enemySpeed};

		for (auto val : helper)
		{
			engine.addInputVariable(val);
			val->addTerm(new fl::Ramp("LOW", 6.5, 3));
			val->addTerm(new fl::Triangle("MEDIUM", 5.5, 10.5));
			val->addTerm(new fl::Ramp("HIGH", 8.5, 16));
		}

		ta.castleWalls = new fl::InputVariable("CastleWalls");
		engine.addInputVariable(ta.castleWalls);
		ta.castleWalls->addTerm(new fl::Rectangle("NONE", CGTownInstance::NONE - 5.0 * fl::fuzzylite::macheps(),
				CGTownInstance::NONE + 5.0 * fl::fuzzylite::macheps()));
		{
			fl::scalar a = CGTownInstance::FORT, d = 2.5;
			fl::scalar b = a + (d - a) * 1.0 / 5.0;
			fl::scalar c = a + (d - a) * 4.0 / 5.0;
			ta.castleWalls->addTerm(new fl::Trapezoid("MEDIUM", a, b, c, d));
		}

		ta.castleWalls->addTerm(new fl::Ramp("HIGH", CGTownInstance::CITADEL - 0.1, CGTownInstance::CASTLE));

		ta.bankPresent = new fl::InputVariable("Bank");
		engine.addInputVariable(ta.bankPresent);
		ta.bankPresent->addTerm(new fl::Rectangle("FALSE", 0.0 - 5.0 * fl::fuzzylite::macheps(),
				0.0 + 5.0 * fl::fuzzylite::macheps()));
		ta.bankPresent->addTerm(new fl::Rectangle("TRUE", 1.0 - 5.0 * fl::fuzzylite::macheps(),
				0.0 + 5.0 * fl::fuzzylite::macheps()));

		ta.threat = new fl::OutputVariable("Threat");
		engine.addOutputVariable(ta.threat);
		ta.threat->addTerm(new fl::Ramp("LOW", 1, MIN_AI_STRENGHT));
		ta.threat->addTerm(new fl::Triangle("MEDIUM", 0.8, 1.2));
		ta.threat->addTerm(new fl::Ramp("HIGH", 1, 1.5));

		engine.addRuleBlock(&ta.tacticalAdvantage);

		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurShooters is MANY and EnemySpeed is LOW then Threat is LOW", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurShooters is MANY and EnemyShooters is FEW then Threat is LOW", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurSpeed is LOW and EnemyShooters is MANY then Threat is HIGH", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurSpeed is HIGH and EnemyShooters is MANY then Threat is LOW", &engine));

		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurWalkers is FEW and EnemyShooters is MANY then Threat is somewhat LOW", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if OurShooters is MANY and EnemySpeed is HIGH then Threat is somewhat HIGH", &engine));
		//just to cover all cases
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if EnemySpeed is MEDIUM then Threat is MEDIUM", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if EnemySpeed is LOW and OurShooters is FEW then Threat is MEDIUM", &engine));

		ta.tacticalAdvantage.addRule(fl::Rule::parse("if Bank is TRUE and OurShooters is MANY then Threat is somewhat HIGH", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if Bank is TRUE and EnemyShooters is MANY then Threat is LOW", &engine));

		ta.tacticalAdvantage.addRule(fl::Rule::parse("if CastleWalls is HIGH and OurWalkers is MANY then Threat is very HIGH", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if CastleWalls is HIGH and OurFlyers is MANY and OurShooters is MANY then Threat is MEDIUM", &engine));
		ta.tacticalAdvantage.addRule(fl::Rule::parse("if CastleWalls is MEDIUM and OurShooters is MANY and EnemyWalkers is MANY then Threat is LOW", &engine));

	}
	catch (fl::Exception & pe)
	{
		logAi->errorStream() << "initTacticalAdvantage " << ": " << pe.getWhat();
	}
}

ui64 FuzzyHelper::estimateBankDanger (const CBank * bank)
{
	auto info = VLC->objtypeh->getHandlerFor(bank->ID, bank->subID)->getObjectInfo(bank->appearance);

	ui64 val = std::numeric_limits<ui64>::max();
	try
	{
		fl::Triangle* bank0 = dynamic_cast<fl::Triangle*> (bankDanger->getTerm("Bank0"));
		fl::Triangle* bank1 = dynamic_cast<fl::Triangle*> (bankDanger->getTerm("Bank1"));
		if (bank0 && bank1)
		{
			bank0->setVertexA(info->minGuards().totalStrength * 0.5f);
			bank0->setVertexC(info->minGuards().totalStrength * 1.5f);
			bank1->setVertexA(info->maxGuards().totalStrength * 0.5f);
			bank1->setVertexC(info->maxGuards().totalStrength * 1.5f);
		}

		//comparison purposes
		//int averageValue = (evaluateBankConfig (VLC->objh->banksInfo[ID][0]) + evaluateBankConfig (VLC->objh->banksInfo[ID][3])) * 0.5;
		//dynamic_cast<fl::SingletonTerm*>(bankInput->term("SET"))->setValue(0.5);
		bankInput->setInputValue(0.5);
		//engine.process(BANK_DANGER);//TODO: Process Bank_Dange only
		engine.process(); //TODO: Process Bank_Dange only
		val = bankDanger->getOutputValue(); //some expected value of this bank
	}
	catch (fl::Exception & fe)
	{
		logAi->errorStream() << "estimateBankDanger " << ": " << fe.getWhat();
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

		ta.ourWalkers->setInputValue(ourStructure.walkers);
		ta.ourShooters->setInputValue(ourStructure.shooters);
		ta.ourFlyers->setInputValue(ourStructure.flyers);
		ta.ourSpeed->setInputValue(ourStructure.maxSpeed);

		ta.enemyWalkers->setInputValue(enemyStructure.walkers);
		ta.enemyShooters->setInputValue(enemyStructure.shooters);
		ta.enemyFlyers->setInputValue(enemyStructure.flyers);
		ta.enemySpeed->setInputValue(enemyStructure.maxSpeed);

		bool bank = dynamic_cast<const CBank*> (enemy);
		if (bank)
			ta.bankPresent->setInputValue(1);
		else
			ta.bankPresent->setInputValue(0);

		const CGTownInstance * fort = dynamic_cast<const CGTownInstance*> (enemy);
		if (fort)
		{
			ta.castleWalls->setInputValue(fort->fortLevel());
		}
		else
			ta.castleWalls->setInputValue(0);

		//engine.process(TACTICAL_ADVANTAGE);//TODO: Process only Tactical_Advantage
		engine.process();
		output = ta.threat->getOutputValue();
	}
	catch (fl::Exception & fe)
	{
		logAi->errorStream() << "getTacticalAdvantage " << ": " << fe.getWhat();
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

	//a trick to switch between heroes less often - calculatePaths is costly
	auto sortByHeroes = [](const Goals::TSubgoal & lhs, const Goals::TSubgoal & rhs) -> bool
	{
		return lhs->hero.h < rhs->hero.h;
	};
	boost::sort (vec, sortByHeroes);

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

		vt.strengthRatio = new fl::InputVariable("strengthRatio"); //hero must be strong enough to defeat guards
		vt.heroStrength = new fl::InputVariable("heroStrength"); //we want to use weakest possible hero
		vt.turnDistance = new fl::InputVariable("turnDistance"); //we want to use hero who is near
		vt.missionImportance = new fl::InputVariable("lockedMissionImportance"); //we may want to preempt hero with low-priority mission
		vt.value = new fl::OutputVariable("Value");

		std::vector<fl::InputVariable*> helper = {vt.strengthRatio, vt.heroStrength, vt.turnDistance, vt.missionImportance};
		for (auto val : helper)
		{
			engine.addInputVariable(val);
		}
		engine.addOutputVariable(vt.value);

		vt.strengthRatio->addTerm(new fl::Ramp("LOW", SAFE_ATTACK_CONSTANT, 0));
		vt.strengthRatio->addTerm(new fl::Ramp("HIGH", SAFE_ATTACK_CONSTANT, SAFE_ATTACK_CONSTANT * 3));

		//strength compared to our main hero
		vt.heroStrength->addTerm(new fl::Ramp("LOW", 0.2, 0));
		vt.heroStrength->addTerm(new fl::Triangle("MEDIUM", 0.2, 0.8));
		vt.heroStrength->addTerm(new fl::Ramp("HIGH", 0.5, 1));

		vt.turnDistance->addTerm(new fl::Ramp("SMALL", 0.5, 0));
		vt.turnDistance->addTerm(new fl::Triangle("MEDIUM", 0.1, 0.8));
		vt.turnDistance->addTerm(new fl::Ramp("LONG", 0.5, 3));

		vt.missionImportance->addTerm(new fl::Ramp("LOW", 2.5, 0));
		vt.missionImportance->addTerm(new fl::Triangle("MEDIUM", 2, 3));
		vt.missionImportance->addTerm(new fl::Ramp("HIGH", 2.5, 5));

		//an issue: in 99% cases this outputs center of mass (2.5) regardless of actual input :/
		 //should be same as "mission Importance" to keep consistency
		vt.value->addTerm(new fl::Ramp("LOW", 2.5, 0));
		vt.value->addTerm(new fl::Triangle("MEDIUM", 2, 3)); //can't be center of mass :/
		vt.value->addTerm(new fl::Ramp("HIGH", 2.5, 5));

		engine.addRuleBlock (&vt.rules);

		//use unarmed scouts if possible
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is HIGH and heroStrength is LOW then Value is very HIGH", &engine));
		//we may want to use secondary hero(es) rather than main hero
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is HIGH and heroStrength is MEDIUM then Value is somewhat HIGH", &engine));
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is HIGH and heroStrength is HIGH then Value is somewhat LOW", &engine));
		//don't assign targets to heroes who are too weak, but prefer targets of our main hero (in case we need to gather army)
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is LOW and heroStrength is LOW then Value is very LOW", &engine));
		//attempt to arm secondary heroes is not stupid
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is LOW and heroStrength is MEDIUM then Value is somewhat HIGH", &engine));
		vt.rules.addRule(fl::Rule::parse("if strengthRatio is LOW and heroStrength is HIGH then Value is LOW", &engine));

		//do not cancel important goals
		vt.rules.addRule(fl::Rule::parse("if lockedMissionImportance is HIGH then Value is very LOW", &engine));
		vt.rules.addRule(fl::Rule::parse("if lockedMissionImportance is MEDIUM then Value is somewhat LOW", &engine));
		vt.rules.addRule(fl::Rule::parse("if lockedMissionImportance is LOW then Value is HIGH", &engine));
		//pick nearby objects if it's easy, avoid long walks
		vt.rules.addRule(fl::Rule::parse("if turnDistance is SMALL then Value is HIGH", &engine));
		vt.rules.addRule(fl::Rule::parse("if turnDistance is MEDIUM then Value is MEDIUM", &engine));
		vt.rules.addRule(fl::Rule::parse("if turnDistance is LONG then Value is LOW", &engine));
	}
	catch (fl::Exception & fe)
	{
		logAi->errorStream() << "visitTile " << ": " << fe.getWhat();
	}
}

float FuzzyHelper::evaluate (Goals::VisitTile & g)
{
	//we assume that hero is already set and we want to choose most suitable one for the mission
	if (!g.hero)
		return 0;

	//assert(cb->isInTheMap(g.tile));
	float turns = 0;
	float distance = cb->getMovementCost(g.hero.h, g.tile);
	if (!distance) //we stand on that tile
		turns = 0;
	else
	{
		if (distance < g.hero->movement) //we can move there within one turn
			turns = (fl::scalar)distance / g.hero->movement;
		else
			turns = 1 + (fl::scalar)(distance - g.hero->movement) / g.hero->maxMovePoints(true); //bool on land?
	}

	float missionImportance = 0;
	if (vstd::contains(ai->lockedHeroes, g.hero))
		missionImportance = ai->lockedHeroes[g.hero]->priority;

	float strengthRatio = 10.0f; //we are much stronger than enemy
	ui64 danger = evaluateDanger (g.tile, g.hero.h);
	if (danger)
		strengthRatio = (fl::scalar)g.hero.h->getTotalStrength() / danger;

	try
	{
		vt.strengthRatio->setInputValue(strengthRatio);
		vt.heroStrength->setInputValue((fl::scalar)g.hero->getTotalStrength() / ai->primaryHero()->getTotalStrength());
		vt.turnDistance->setInputValue(turns);
		vt.missionImportance->setInputValue(missionImportance);

		engine.process();
		//engine.process(VISIT_TILE); //TODO: Process only Visit_Tile
		g.priority = vt.value->getOutputValue();
	}
	catch (fl::Exception & fe)
	{
		logAi->errorStream() << "evaluate VisitTile " << ": " << fe.getWhat();
	}
	return g.priority;

}
float FuzzyHelper::evaluate (Goals::VisitHero & g)
{
	auto obj = cb->getObj(ObjectInstanceID(g.objid)); //we assume for now that these goals are similar
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

float FuzzyHelper::evaluate (Goals::ClearWayTo & g)
{
	if (!g.hero.h)
		throw cannotFulfillGoalException("ClearWayTo called without hero!");

	SectorMap sm(g.hero);
	int3 t = sm.firstTileToGet(g.hero, g.tile);

	if (t.valid())
	{
		if (isSafeToVisit(g.hero, t))
		{
			g.setpriority(Goals::VisitTile(g.tile).sethero(g.hero).setisAbstract(g.isAbstract).accept(this));
		}
		else
		{
			g.setpriority (Goals::GatherArmy(evaluateDanger(t, g.hero.h)*SAFE_ATTACK_CONSTANT).
				sethero(g.hero).setisAbstract(true).accept(this));
		}
		return g.priority;
	}
	else
		return -1;
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
