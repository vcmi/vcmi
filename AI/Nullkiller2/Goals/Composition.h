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

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT Composition : public ElementarGoal<Composition>
	{
	private:
		std::vector<TGoalVec> subtasks; // things we want to do now

	public:
		Composition()
			: ElementarGoal(Goals::COMPOSITION), subtasks()
		{
		}

		bool operator==(const Composition & other) const override;
		std::string toString() const override;
		void accept(AIGateway * aiGw) override;
		Composition & addNext(const AbstractGoal & goal);
		Composition & addNext(TSubgoal goal);
		Composition & addNextSequence(const TGoalVec & taskSequence);
		TGoalVec decompose(const Nullkiller * aiNk) const override;
		bool isElementar() const override;
		int getHeroExchangeCount() const override;

		std::vector<ObjectInstanceID> getAffectedObjects() const override;
		bool isObjectAffected(ObjectInstanceID id) const override;
	};
}

}
