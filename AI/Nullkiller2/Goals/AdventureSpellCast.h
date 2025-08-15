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

namespace NK2AI
{

namespace Goals
{
	class DLL_EXPORT AdventureSpellCast : public ElementarGoal<AdventureSpellCast>
	{
	private:
		SpellID spellID;

	public:
		AdventureSpellCast(const CGHeroInstance * hero, SpellID spellID)
			: ElementarGoal(Goals::ADVENTURE_SPELL_CAST), spellID(spellID)
		{
			sethero(hero);
		}

		const CSpell * getSpell() const
		{ 
			return spellID.toSpell();
		}

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const AdventureSpellCast & other) const override;
	};
}

}
