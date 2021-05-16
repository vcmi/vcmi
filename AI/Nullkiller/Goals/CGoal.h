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
#include "../FuzzyHelper.h"
#include "../VCAI.h"

struct HeroPtr;
class VCAI;

namespace Goals
{
	template<typename T> class DLL_EXPORT CGoal : public AbstractGoal
	{
	public:
		CGoal<T>(EGoals goal = INVALID) : AbstractGoal(goal)
		{
			isElementar = false;
			isAbstract = true;
			value = 0;
			aid = -1;
			objid = -1;
			resID = -1;
			tile = int3(-1, -1, -1);
			town = nullptr;
		}

		OSETTER(bool, isElementar)
		OSETTER(bool, isAbstract)
		OSETTER(int, value)
		OSETTER(int, resID)
		OSETTER(int, objid)
		OSETTER(int, aid)
		OSETTER(int3, tile)
		OSETTER(HeroPtr, hero)
		OSETTER(CGTownInstance *, town)
		OSETTER(int, bid)

		CGoal<T> * clone() const override
		{
			return new T(static_cast<T const &>(*this)); //casting enforces template instantiation
		}
		TSubgoal iAmElementar() const
		{
			TSubgoal ptr;

			ptr.reset(clone());
			ptr->setisElementar(true);

			return ptr;
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
			return {decomposeSingle()};
		}

	protected:
		virtual TSubgoal decomposeSingle() const
		{
			return sptr(Invalid());
		}
	};

	template<typename T> class DLL_EXPORT ElementarGoal : public CGoal<T>, public ITask
	{
	public:
		ElementarGoal<T>(EGoals goal = INVALID) : CGoal(goal)
		{
			priority = 0;
			isElementar = true;
			isAbstract = false;
		}

		///Visitor pattern
		//TODO: make accept work for std::shared_ptr... somehow
		virtual void accept(VCAI * ai) override //unhandled goal will report standard error
		{
			ai->tryRealize(*this);
		}

		T & setpriority(float p)
		{
			priority = p;

			return *((T *)this);
		}
	};

	class DLL_EXPORT Invalid : public ElementarGoal<Invalid>
	{
	public:
		Invalid()
			: ElementarGoal(Goals::INVALID)
		{
			priority = -1;
		}
		TGoalVec decompose() const override
		{
			return TGoalVec();
		}

		virtual bool operator==(const Invalid & other) const override
		{
			return true;
		}

		virtual std::string toString() const override
		{
			return "Invalid";
		}
	};
}
