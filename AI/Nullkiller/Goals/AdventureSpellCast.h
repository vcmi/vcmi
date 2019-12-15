/*
* AdventureSpellCast.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace Goals
{
	class DLL_EXPORT AdventureSpellCast : public CGoal<AdventureSpellCast>
	{
	private:
		SpellID spellID;

	public:
		AdventureSpellCast(HeroPtr hero, SpellID spellID)
			: CGoal(Goals::ADVENTURE_SPELL_CAST), spellID(spellID)
		{
			sethero(hero);
		}

		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}

		const CSpell * getSpell() const
		{ 
			return spellID.toSpell();
		}

		TSubgoal whatToDoToAchieve() override;
		void accept(VCAI * ai) override;
		std::string name() const override;
		virtual bool operator==(const AdventureSpellCast & other) const override;
	};
}
