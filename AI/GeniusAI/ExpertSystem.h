#include "../vcmi/global.h"
#include "../vcmi/CCallback.h"
#include "../vcmi/lib/HeroBonus.h"
#include <boost/bind.hpp>
#include <set>

/*
 * ExpertSystem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
struct Bonus;
template <typename input, typename output> class Rule;
typedef Rule<Bonus, Bonus> BRule;

template <typename ruleType, typename facts> class ExpertSystemShell
{
	enum runType {ANY_GOAL, TRESHOLD, FULL};
private:
	ICallback* m_cb;
protected:
	std::set<ruleType> knowledge;
	ui16 goalCounter; //count / evaluate achieved goals for runType
public:
	ExpertSystemShell(){goalCounter = 0;};
	std::list<facts> factList;

	template <typename t1> void getKnowledge(const t1 &a1){};
	template <typename ruleType> void getNextRule(){};
	template <typename t2> void returnGoals(const t2 &a2){};
	virtual void DataDrivenReasoning(runType type);
};
template <typename ruleType, typename facts> class Blackboard : public ExpertSystemShell <ruleType, facts>
//handle Bonus info coming from different sections of the game
{
public:
	Blackboard(){this->goalCounter = 0;};
	std::vector<ExpertSystemShell*> experts; //modules responsible for different tasks
};

template <typename input> class condition
{
	enum conditionType {LESS_THAN, EQUAL, GREATER_THAN, UNEQUAL, PRESENT};
public:
	input object;
	ui16 value;
	conditionType conType;
};

template <typename input, typename output> class Rule
{
friend class ExpertSystemShell <input, output>;
public:
	bool fired; //if conditions of rule were met and it produces some output
protected:
	std::set<input*> conditions;
	output decision;
	virtual void canBeFired(); //if this data makes any sense for rule
	virtual void fireRule();
public:
	template <typename givenInput> bool matchesInput() //if condition and data match type
		{return dynamic_cast<input*>(&givenInput);};
};

template <typename input, typename output> class Goal : public Rule <input, output>
{
protected:
	boost::function<void(output)> decision(); //do something with AI eventually
public:
	void fireRule(){};
};

template <typename input, typename output> class Weight : public Rule <input, output>
{
public:
	float value; //multiply input by value and return to output
	void fireTule(){};
};

class BonusSystemExpert : public ExpertSystemShell <BRule, Bonus>
{ //TODO: less templates?
	enum effectType {POSITIVE=1, NEGATIVE=2, EXCLUDING=4, ENEMY=8, ALLY=16}; //what is the influencce of bonus and for who
};