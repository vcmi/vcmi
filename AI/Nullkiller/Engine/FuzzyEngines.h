/*
* FuzzyEngines.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once
#include <fl/Headers.h>
#include "../Goals/AbstractGoal.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;

VCMI_LIB_NAMESPACE_END

class engineBase //subclasses create fuzzylite variables with "new" that are not freed - this is desired as fl::Engine wants to destroy these...
{
protected:
	fl::Engine engine;
	fl::RuleBlock * rules;
	virtual void configure();
	void addRule(const std::string & txt);
public:
	engineBase();
};

class TacticalAdvantageEngine : public engineBase //TODO: rework this engine, it does not work well (example: AI hero with 140 beholders attacked 150 beholders - engine lowered danger 50000 -> 35000)
{
public:
	TacticalAdvantageEngine();
	float getTacticalAdvantage(const CArmedInstance * we, const CArmedInstance * enemy); //returns factor how many times enemy is stronger than us
private:
	fl::InputVariable * ourWalkers, *ourShooters, *ourFlyers, *enemyWalkers, *enemyShooters, *enemyFlyers;
	fl::InputVariable * ourSpeed, *enemySpeed;
	fl::InputVariable * bankPresent;
	fl::InputVariable * castleWalls;
	fl::OutputVariable * threat;
};
