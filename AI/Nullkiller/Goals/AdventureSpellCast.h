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

namespace NKAI
{

namespace Goals
{
	class DLL_EXPORT AdventureSpellCast : public ElementarGoal<AdventureSpellCast>
	{
	private:
		SpellID spellID;

	public:
		AdventureSpellCast(HeroPtr hero, SpellID spellID)
			: ElementarGoal(Goals::ADVENTURE_SPELL_CAST), spellID(spellID)
		{
			sethero(hero);
		}

		const CSpell * getSpell() const
		{ 
			return spellID.toSpell();
		}

		void accept(AIGateway * ai) override;
		std::string toString() const override;
		virtual bool operator==(const AdventureSpellCast & other) const override;
	};
}

}
