#include "../vcmi/global.h"
#include "../vcmi/CCallback.h"
#include "../vcmi/lib/HeroBonus.h"
#include <boost/bind.hpp>
#include <vector>

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
template <typename fact> class AIholder;
template <typename input, typename output> class Rule;
typedef Rule<Bonus, Bonus> BRule;
typedef boost::function<bool(int, si32)> TLogic;
bool greaterThan (int prop, si32 val);

enum conditionType {LESS_THAN, EQUAL, GREATER_THAN, UNEQUAL, PRESENT};

template <typename ruleType, typename fact> class ExpertSystemShell
{
	enum runType {ANY_GOAL, TRESHOLD, FULL}; //Treshold - stop when received decision has high AI value
private:
	ICallback* m_cb;
protected:
	std::vector<ruleType> knowledge;
	std::vector<ruleType*> rulesToErase;
	std::vector<ruleType*> rulesToAdd;
	std::vector<fact*> factsToErase;
	std::vector<fact*> factsToAdd;
	ui16 goalCounter; //count / evaluate achieved goals for runType
public:
	ExpertSystemShell(){goalCounter = 0;};
	std::list<fact> facts; //facts are AIholders with actual game objects, for eg. "my bonus is worthless"

	template <typename t1> void getKnowledge(const t1 &a1){};
	void getNextRule(){};
	void addRule(ruleType &rule){rulesToAdd.insert(rule);};
	void removeRule(ruleType &rule){rulesToErase.insert(rule);};
	template <typename t2> void returnGoals(const t2 &a2){};
	template <typename cond> void DataDrivenReasoning(runType type);
};
template <typename ruleType, typename fact> class Blackboard : public ExpertSystemShell <ruleType, fact>
//handle Bonus info coming from different sections of the game
{
public:
	Blackboard(){this->goalCounter = 0;}; //template magic, do not touch!
	std::vector<ExpertSystemShell*> experts; //modules responsible for different tasks
};

template <typename input> class condition
{//compares selected object parameter with value using functor. universal (?) logic handler
public:
	input object; //what the fact is, or what it's like (CSelector)
	si32 value;
	ui8 parameter;
	TLogic functor; //value of selected parameter, condition value

	condition(){object = NULL; value = 0; parameter = 0; functor = greaterThan;};

	bool matchesFact(input &fact){return false;};
};

template <typename input, typename conType> class Rule
{
friend class ExpertSystemShell <input, conType>;
public:
	bool fired; //if conditions of rule were met and it produces some output
	ui8 conditionCounter;
protected:
	std::vector<std::pair<conType, input*>> cons; //conditions and matching facts
	input decision;
	virtual void canBeFired(); //if this data makes any sense for rule - type check
	virtual bool checkCondition(); //if condition is true or false
	virtual bool checkCondition(std::vector<input*> &feed);
	virtual void fireRule(); //use paired conditions and facts by default
	virtual void fireRule(ExpertSystemShell<input, conType> &system);
	virtual void fireRule(std::vector<input*> &feed);
	virtual void refreshRule();
	virtual void refreshRule(std::vector<conType> &conditionSet); //in case conditions were erased
public:
	Rule(){fired = false; conditionCounter = 0; decision = NULL;};
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
	float multiplier; //multiply input by value and return to output
	void fireTule(){};
};

template <typename input> class AIholder
{ //stores certain condition and its temporal value
public:
	si32 aiValue;
	input *object;

	AIholder(){object = NULL; aiValue = 0;}
	AIholder(input &o){object = o; aiValue = 0;}
	AIholder(input &o, si32 val){object = 0; aiValue = val;}
};

class BonusSystemExpert : public ExpertSystemShell <BRule, Bonus>
{ //TODO: less templates?
	enum effectType {POSITIVE=1, NEGATIVE=2, EXCLUDING=4, ENEMY=8, ALLY=16}; //what is the influence of bonus and for who
};

class BonusCondition : public condition<CSelector> //used to test rule
{
public:
	enum Parameter
	{
		type, subtype, val, duration, source, id, valType, additionalInfo, effectRange, limiter //?
	};
	bool matchesFact(Bonus &fact);
};

class BonusHolder : public AIholder<Bonus>
{
public:
	BonusHolder(Bonus &bonus){object = &bonus; aiValue = bonus.val;}
	BonusHolder(Bonus &bonus, si32 val){object = &bonus; aiValue = val;}
};

class BonusRule : public Rule <BonusHolder, BonusCondition>
{
protected:
	void fireRule();
};

inline bool greaterThan (int prop, si32 val) //does it make any sense to keep functors inline?
{
	if ((si32)prop > val)
		return true;
	return false;
}
inline bool lessThan (int prop, si32 val)
{
	if ((si32)prop < val)
		return true;
	return false;
}
inline bool eqal (int prop, si32 val)
{
	if ((si32)prop == val)
		return true;
	return false;
}
inline bool unequal (int prop, si32 val)
{
	if ((si32)prop != val)
		return true;
	return false;
}
inline bool present (int prop, si32 val=0)
//inline bool present (int prop) //TODO: can we use function with less arguments?
{
	return(prop); //unfixable warning :(
}

class LogicConjunction
{
	const TLogic first, second; //TODO: universal argument list of functions?
public:
	LogicConjunction(const TLogic First, const TLogic Second)
		:first(First), second(Second)
	{
	}
	bool operator()(int prop, si32 val) const
	{
		return first(prop,val) && second(prop,val);
	}
};
TLogic operator&&(const TLogic &first, const TLogic &second);

class LogicAlternative
{
	const TLogic first, second;
public:
	LogicAlternative(const TLogic First, const TLogic Second)
		:first(First), second(Second)
	{
	}
	bool operator()(int prop, si32 val) const
	{
		return first(prop,val) || second(prop,val); 
	}
};
TLogic operator||(const TLogic &first, const TLogic &second);

class KnowledgeHandler///I'd opt for one omniscent knowledge manager, so no templates here
{
public:
	std::list<BonusRule> knowledge; //permanent storage of rules

	void parseKnowledge(std::string &filename){};
	void addKnowledge(ExpertSystemShell<BRule,Bonus> &expert);
	void addFacts(ExpertSystemShell<BRule,Bonus> &expert);
};