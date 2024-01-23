/*
* CGoal.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "AbstractGoal.h"

namespace NKAI
{

struct HeroPtr;
class AIGateway;

namespace Goals
{
	template<typename T> class DLL_EXPORT CGoal : public AbstractGoal
	{
	public:
		CGoal<T>(EGoals goal = INVALID) : AbstractGoal(goal)
		{
			isAbstract = true;
			value = 0;
			aid = -1;
			objid = -1;
			resID = -1;
			tile = int3(-1, -1, -1);
			town = nullptr;
		}

		CGoal<T> * clone() const override
		{
			return new T(static_cast<T const &>(*this)); //casting enforces template instantiation
		}
		template<typename Handler> void serialize(Handler & h, const int version)
		{
			h & static_cast<AbstractGoal &>(*this);
			//h & goalType & isElementar & isAbstract & priority;
			//h & value & resID & objid & aid & tile & hero & town & bid;
		}

		virtual bool operator==(const AbstractGoal & g) const override
		{
			if(goalType != g.goalType)
				return false;

			return (*this) == (static_cast<const T &>(g));
		}

		virtual bool operator==(const T & other) const = 0;

		virtual TGoalVec decompose() const override
		{
			TSubgoal single = decomposeSingle();

			if(!single || single->invalid())
				return {};
			
			return {single};
		}

	protected:
		virtual TSubgoal decomposeSingle() const
		{
			return TSubgoal();
		}
	};

	template<typename T> class DLL_EXPORT ElementarGoal : public CGoal<T>, public ITask
	{
	public:
		ElementarGoal<T>(EGoals goal = INVALID) : CGoal<T>(goal), ITask()
		{
			AbstractGoal::isAbstract = false;
		}

		ElementarGoal<T>(const ElementarGoal<T> & other) : CGoal<T>(other), ITask(other)
		{
		}

		T & setpriority(float p)
		{
			ITask::priority = p;

			return *((T *)this);
		}

		virtual bool isElementar() const override { return true; }

		virtual HeroPtr getHero() const override { return AbstractGoal::hero; }

		virtual int getHeroExchangeCount() const override { return 0; }
	};
}

}
