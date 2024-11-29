/*
* AdventureSpellCast.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AdventureSpellCast.h"
#include "../AIGateway.h"

namespace NKAI
{

using namespace Goals;

bool AdventureSpellCast::operator==(const AdventureSpellCast & other) const
{
	return hero == other.hero;
}

void AdventureSpellCast::accept(AIGateway * ai)
{
	if(!hero)
		throw cannotFulfillGoalException("Invalid hero!");

	auto spell = getSpell();

	logAi->trace("Decomposing adventure spell cast of %s for hero %s", spell->getNameTranslated(), hero->getNameTranslated());

	if(!spell->isAdventure())
		throw cannotFulfillGoalException(spell->getNameTranslated() + " is not an adventure spell.");

	if(!hero->canCastThisSpell(spell))
		throw cannotFulfillGoalException("Hero can not cast " + spell->getNameTranslated());

	if(hero->mana < hero->getSpellCost(spell))
		throw cannotFulfillGoalException("Hero has not enough mana to cast " + spell->getNameTranslated());


	if(town && spellID == SpellID::TOWN_PORTAL)
	{
		ai->selectedObject = town->id;

		if(town->visitingHero && town->tempOwner == ai->playerID && !town->getUpperArmy()->stacksCount())
		{
			ai->myCb->swapGarrisonHero(town);
		}

		if(town->visitingHero)
			throw cannotFulfillGoalException("The town is already occupied by " + town->visitingHero->getNameTranslated());
	}

	if (hero->inTownGarrison)
		ai->myCb->swapGarrisonHero(hero->visitedTown);

	auto wait = cb->waitTillRealize;

	cb->waitTillRealize = true;
	cb->castSpell(hero, spellID, tile);

	if(town && spellID == SpellID::TOWN_PORTAL)
	{
		// visit town
		ai->moveHeroToTile(town->visitablePos(), hero);
	}

	cb->waitTillRealize = wait;

	throw goalFulfilledException(sptr(*this));
}

std::string AdventureSpellCast::toString() const
{
	return "AdventureSpellCast " + spellID.toSpell()->getNameTranslated();
}

}
