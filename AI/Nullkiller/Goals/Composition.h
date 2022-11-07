/*
* BuildThis.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT Composition : public ElementarGoal<Composition>
	{
	private:
		TGoalVec subtasks;

	public:
		Composition()
			: ElementarGoal(Goals::COMPOSITION), subtasks()
		{
		}

		Composition(TGoalVec subtasks)
			: ElementarGoal(Goals::COMPOSITION), subtasks(subtasks)
		{
		}

		virtual bool operator==(const Composition & other) const override;
		virtual std::string toString() const override;
		void accept(AIGateway * ai) override;
		Composition & addNext(const AbstractGoal & goal);
		Composition & addNext(TSubgoal goal);
		virtual TGoalVec decompose() const override;
		virtual bool isElementar() const override;
		virtual int getHeroExchangeCount() const override;
	};
}

}
