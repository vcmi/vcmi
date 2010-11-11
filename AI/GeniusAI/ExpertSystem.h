#include "../vcmi/global.h"
#include "../vcmi/CCallback.h"
#include "HeroBonus.h"
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

template <typename ruleType, typename facts> class ExpertSystemShell
{
private:
	ICallback* m_cb;
protected:
	template std::set<typename ruleType> knowledge;
public:
	template std::set<typename facts> facts;

	template <typename t1> void getKnowledge(const t1 &a1){};
	template <typename ruleType> void getNextRule(){};
	template <typename t2> void returnGoals(const t2 &a2){};
};

template <typename input> class condition
{
	enum conditionType {LESS_THAN, EQUAL, GREATER_THAN, UNEQUAL, PRESENT};
public:
	input object;
	ui16 value;
	conditionType conType;
};

template <typename &input, typename &output> class Rule
{
friend class ExpertSystemShell;
public:
	fired; //if conditions of rule were met and it produces some output
protected:
	std::set<typename input> conditions;
	virtual void canBeFired(); //if this data makes any sense for rule
	virtual void fireRule();
public:
	template <typename &givenInput> bool matchesInput() //if condition and data match type
		{return dynamic_cast<input*>(givenInput);};
};

template <typename &input, typename &output> class Goal : public Rule
{
public:
	void fireTule(){};
};

template <typename &input, typename &output> class Weight : public Rule
{
public:
	float value; //multiply input by value and return to output
	void fireTule(){};
};

template <typename RuleType> class BonusSystemExpert : public ExpertSystemShell <typename RuleType, Bonus>
{
	enum effectType {POSITIVE=1, NEGATIVE=2, EXCLUDING=4, ENEMY=8, ALLY=16}; //what is the influencce of bonus and for who
};