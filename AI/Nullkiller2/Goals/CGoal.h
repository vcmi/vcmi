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

namespace NK2AI
{

namespace Goals
{
	template<typename T>
	class DLL_EXPORT CGoal : public AbstractGoal
	{
	public:
		explicit CGoal(EGoals goal = INVALID) : AbstractGoal(goal)
		{
			// isAbstract = true;
			// value = 0;
			// aid = -1;
			// objid = -1;
			// resID = -1;
			// tile = int3(-1, -1, -1);
			// town = nullptr;
		}

		~CGoal() override = default;

		CGoal * clone() const override
		{
			return new T(static_cast<T const &>(*this)); //casting enforces template instantiation
		}

		bool operator==(const AbstractGoal & g) const override
		{
			if(goalType != g.goalType)
				return false;

			return (*this) == (static_cast<const T &>(g));
		}

		virtual bool operator==(const T & other) const = 0;

		TGoalVec decompose(const Nullkiller * aiNk) const override
		{
			TSubgoal single = decomposeSingle(aiNk);

			if(!single || single->invalid())
				return {};
			
			return {single};
		}

	protected:
		virtual TSubgoal decomposeSingle(const Nullkiller * aiNk) const
		{
			return TSubgoal();
		}
	};

	template<typename T>
	class DLL_EXPORT ElementarGoal : public CGoal<T>, public ITask
	{
	public:
		ElementarGoal(EGoals goal = INVALID) : CGoal<T>(goal), ITask()
		{
			AbstractGoal::isAbstract = false;
		}

		ElementarGoal(const ElementarGoal<T> & other) : CGoal<T>(other), ITask(other)
		{
		}

		T & setpriority(float p)
		{
			ITask::priority = p;

			return *((T *)this);
		}

		bool isElementar() const override { return true; }

		const CGHeroInstance * getHero() const override { return AbstractGoal::hero; }

		int getHeroExchangeCount() const override { return 0; }

		bool isObjectAffected(ObjectInstanceID id) const override
		{
			return (AbstractGoal::hero && AbstractGoal::hero->id == id)
				|| AbstractGoal::objid == id.getNum()
				|| (AbstractGoal::town && AbstractGoal::town->id == id);
		}

		std::vector<ObjectInstanceID> getAffectedObjects() const override
		{
			auto result = std::vector<ObjectInstanceID>();

			if(AbstractGoal::hero)
				result.push_back(AbstractGoal::hero->id);

			if(AbstractGoal::objid != -1)
				result.push_back(ObjectInstanceID(AbstractGoal::objid));

			if(AbstractGoal::town)
				result.push_back(AbstractGoal::town->id);

			return result;
		}

		ITask * asTask() override
		{
			return this;
		}
	};
}

}
